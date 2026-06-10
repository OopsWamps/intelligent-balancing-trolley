#!/usr/bin/env python3
"""
ML-based object tracking for balancing car visual following.
Phase 2: Person detection (MediaPipe Pose) + custom object tracking (CSRT).

Protocol (Pi -> STM32, 6 bytes):
  [0xA5, x_offset(int8), y_offset(int8), distance(uint8),
   confidence(uint8), xor_checksum]

Modes:
  'p' → person following (MediaPipe Pose, auto-detect)
  'o' → object following (click-drag ROI, CSRT tracker)
  SPACE → toggle tracking on/off
  'c' → clear selection
  'q' / ESC → quit
"""

import argparse
import os
import sys
import time
from collections import deque

import cv2
import numpy as np

# ── serial ────────────────────────────────────────────────────
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

# ── ML imports (graceful fallback) ────────────────────────────
try:
    import mediapipe as mp
    from mediapipe.tasks.python import vision, BaseOptions
    from mediapipe.tasks.python.vision import PoseLandmark
    HAS_MEDIAPIPE = True
except ImportError:
    HAS_MEDIAPIPE = False
    print("[WARN] mediapipe not installed — person mode disabled")

# ── protocol ──────────────────────────────────────────────────
FRAME_HEADER = 0xA5
REVERSE_HEADER = 0xB0
CMD_LEARN, CMD_CLEAR, CMD_STOP = 0x01, 0x02, 0x03

# ── distance estimation ───────────────────────────────────────
AVG_SHOULDER_WIDTH_CM = 42.0     # average adult shoulder width
FOCAL_LENGTH_PX = 600.0          # calibrated focal length (tune per camera)


def make_frame(x_offset, y_offset, distance, confidence):
    x = max(-128, min(127, int(x_offset)))
    y = max(-128, min(127, int(y_offset)))
    d = max(0, min(255, int(distance)))
    c = max(0, min(100, int(confidence)))
    payload = bytes([FRAME_HEADER, x & 0xFF, y & 0xFF, d, c])
    cs = 0
    for b in payload:
        cs ^= b
    return payload + bytes([cs])


# ═══════════════════════════════════════════════════════════════
#  Person Detector  (MediaPipe Pose Landmarker)
# ═══════════════════════════════════════════════════════════════

MODEL_PATH = os.path.join(os.path.dirname(__file__),
                          "models", "pose_landmarker_lite.task")


class PersonDetector:
    """Detect the closest person using MediaPipe Pose Landmarker (0.10.x API)."""

    # Landmark indices for torso center (compatible with both old & new API)
    TORSO_IDS = [11, 12, 23, 24]   # L/R shoulder, L/R hip

    def __init__(self, num_poses=1, min_detection_confidence=0.5):
        if not HAS_MEDIAPIPE:
            raise RuntimeError("MediaPipe not available")
        if not os.path.exists(MODEL_PATH):
            raise RuntimeError(
                f"Model not found: {MODEL_PATH}\n"
                "Download from: https://ai.google.dev/edge/mediapipe/solutions/vision/pose_landmarker")

        options = vision.PoseLandmarkerOptions(
            base_options=BaseOptions(model_asset_path=MODEL_PATH),
            running_mode=vision.RunningMode.VIDEO,
            num_poses=num_poses,
            min_pose_detection_confidence=min_detection_confidence,
            min_pose_presence_confidence=0.5,
            min_tracking_confidence=0.4,
            output_segmentation_masks=False,
        )
        self.landmarker = vision.PoseLandmarker.create_from_options(options)
        self._ts = 0

    def detect(self, rgb_frame):
        """Return (cx, cy, shoulder_width_px, confidence) or None."""
        h, w = rgb_frame.shape[:2]
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        self._ts += 33   # simulate ~30fps timing
        result = self.landmarker.detect_for_video(mp_image, self._ts)

        if not result.pose_landmarks:
            return None

        # Use the first detected person
        landmarks = result.pose_landmarks[0]

        # Check visibility of key landmarks
        vis = [landmarks[i].visibility for i in self.TORSO_IDS]
        if sum(vis) < 1.0:
            return None

        # Gather torso landmark pixel coordinates (landmarks are 0..1 normalized)
        xs = [int(landmarks[i].x * w) for i in self.TORSO_IDS]
        ys = [int(landmarks[i].y * h) for i in self.TORSO_IDS]
        cx = int(np.mean(xs))
        cy = int(np.mean(ys))

        # Shoulder width in pixels
        shoulder_px = abs(int(landmarks[11].x * w) - int(landmarks[12].x * w))

        avg_vis = sum(vis) / len(vis)
        return cx, cy, shoulder_px, avg_vis

    def release(self):
        self.landmarker.close()


# ═══════════════════════════════════════════════════════════════
#  ML Tracker  (unified interface)
# ═══════════════════════════════════════════════════════════════

class MLTracker:
    def __init__(self):
        self.mode = "person"      # "person" | "object"
        self.tracking = False
        self.person_det = None    # PersonDetector instance
        self.roi = None           # (x, y, w, h) for CSRT
        self.csrt = None          # cv2.TrackerCSRT
        self.lost_count = 0
        self.smooth_xy = deque(maxlen=5)
        self.smooth_dist = deque(maxlen=5)

        self.cap = None
        self.ser = None
        self.frame_w = 640
        self.frame_h = 480
        self._last_send = 0

    # ── setup ──────────────────────────────────────────────
    def open_camera(self, index=0):
        self.cap = cv2.VideoCapture(index)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.frame_w)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.frame_h)
        if not self.cap.isOpened():
            raise RuntimeError(f"Cannot open camera {index}")

    def open_serial(self, port="/dev/serial0", baud=9600):
        if not HAS_SERIAL:
            print("[WARN] pyserial not installed")
            return
        self.ser = serial.Serial(port, baud, timeout=0.05,
                                 bytesize=serial.EIGHTBITS,
                                 parity=serial.PARITY_NONE,
                                 stopbits=serial.STOPBITS_ONE)
        print(f"[SERIAL] {port} @ {baud}")

    # ── person detection ───────────────────────────────────
    def init_person_mode(self):
        if not HAS_MEDIAPIPE:
            print("[ERROR] mediapipe required for person mode")
            return
        if self.person_det is None:
            self.person_det = PersonDetector()
            print("[MODE] Person following (MediaPipe Pose)")

    def detect_person(self, rgb_frame):
        result = self.person_det.detect(rgb_frame)
        if result is None:
            self.lost_count += 1
            return None
        cx, cy, shoulder_px, conf = result
        self.lost_count = 0

        # Distance from shoulder width triangulation
        if shoulder_px > 5:
            distance = AVG_SHOULDER_WIDTH_CM * FOCAL_LENGTH_PX / shoulder_px
        else:
            distance = 200

        # Confidence decays if we had recent losses
        effective_conf = conf * 100.0 * (0.5 if self.lost_count > 0 else 1.0)

        # Smooth outputs
        self.smooth_xy.append((cx, cy))
        self.smooth_dist.append(distance)
        sx = int(np.mean([v[0] for v in self.smooth_xy]))
        sy = int(np.mean([v[1] for v in self.smooth_xy]))
        sd = np.mean(self.smooth_dist)

        return sx, sy, sd, effective_conf

    # ── object tracking (CSRT) ─────────────────────────────
    def init_object_tracker(self, frame, roi):
        """Initialize CSRT tracker with selected ROI."""
        if self.csrt is not None:
            del self.csrt
        self.csrt = cv2.TrackerCSRT_create()
        self.csrt.init(frame, tuple(roi))
        self.roi = roi
        self.lost_count = 0
        print(f"[MODE] Object following (CSRT), ROI={roi}")

    def track_object(self, frame):
        """Return (cx, cy, distance, confidence) or None."""
        if self.csrt is None:
            return None
        ok, bbox = self.csrt.update(frame)
        if not ok:
            self.lost_count += 1
            return None

        self.lost_count = max(0, self.lost_count - 1)
        x, y, w, h = [int(v) for v in bbox]
        cx, cy = x + w // 2, y + h // 2

        # Distance from bounding box area
        area = w * h
        distance = 12000.0 / max(np.sqrt(area), 1.0)
        distance = max(10, min(255, distance))
        confidence = 70.0 if self.lost_count == 0 else 40.0

        self.smooth_xy.append((cx, cy))
        sx = int(np.mean([v[0] for v in self.smooth_xy]))
        sy = int(np.mean([v[1] for v in self.smooth_xy]))

        return sx, sy, distance, confidence

    # ── frame computation ──────────────────────────────────
    def compute_frame(self, cx, cy, distance, confidence):
        x_center = self.frame_w / 2.0
        x_offset = (cx - x_center) / x_center * 127.0

        y_center = self.frame_h / 2.0
        y_offset = (cy - y_center) / y_center * 127.0

        return x_offset, y_offset, distance, confidence

    def send_frame(self, x_off, y_off, dist, conf):
        now = time.time()
        if now - self._last_send < 0.08:
            return
        self._last_send = now
        frame = make_frame(x_off, y_off, dist, conf)
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(frame)
            except serial.SerialException:
                pass

    def check_serial_rx(self):
        if not self.ser or not self.ser.is_open:
            return
        try:
            while self.ser.in_waiting >= 2:
                b0 = self.ser.read(1)[0]
                if b0 == REVERSE_HEADER:
                    cmd = self.ser.read(1)[0]
                    if cmd == CMD_LEARN:
                        self.tracking = True
                    elif cmd in (CMD_CLEAR, CMD_STOP):
                        self.tracking = False
        except (serial.SerialException, IndexError):
            pass

    # ── UI ──────────────────────────────────────────────────
    def draw_hud(self, frame, cx=None, cy=None, dist=0, conf=0,
                 shoulder_px=0):
        h, w = frame.shape[:2]

        # Crosshair
        cv2.line(frame, (w // 2 - 12, h // 2), (w // 2 + 12, h // 2),
                 (80, 80, 80), 1)
        cv2.line(frame, (w // 2, h // 2 - 12), (w // 2, h // 2 + 12),
                 (80, 80, 80), 1)

        # Status
        if self.mode == "person":
            label = f"PERSON  {'TRACK' if self.tracking else 'PAUSED'}"
            color = (0, 255, 0) if self.tracking else (0, 200, 255)
        else:
            label = f"OBJECT  {'TRACK' if self.tracking else 'PAUSED'}"
            color = (200, 150, 0) if self.tracking else (0, 200, 255)
        cv2.putText(frame, label, (10, h - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

        # Tracking marker
        if cx and cy and self.tracking:
            cv2.circle(frame, (cx, cy), 12, (0, 255, 0), 2)
            cv2.circle(frame, (cx, cy), 3, (0, 255, 0), -1)
            xc = w // 2
            x_off = (cx - xc) / xc * 127
            tele = f"X={x_off:+.0f}  D={dist:.0f}cm  C={conf:.0f}%"
            cv2.putText(frame, tele, (cx - 55, cy - 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)

        # ROI selection rectangle (object mode)
        if self.mode == "object" and self.roi is not None:
            rx, ry, rw, rh = self.roi
            cv2.rectangle(frame, (rx, ry), (rx + rw, ry + rh),
                          (200, 150, 0), 2)

        # Lost warning
        if self.lost_count > 5:
            cv2.putText(frame, "LOST", (w // 2 - 30, h // 2 + 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

    # ── ROI selection callback ─────────────────────────────
    def _on_mouse(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            self._drag_start = (x, y)
        elif event == cv2.EVENT_LBUTTONUP and hasattr(self, '_drag_start'):
            sx, sy = self._drag_start
            w_abs, h_abs = abs(x - sx), abs(y - sy)
            if w_abs > 15 and h_abs > 15:
                rx = min(sx, x)
                ry = min(sy, y)
                self.roi = (rx, ry, w_abs, h_abs)
                # Re-init CSRT on next frame
                self._roi_pending = True

    # ── main loop ──────────────────────────────────────────
    def run(self):
        if self.cap is None:
            self.open_camera()

        self._drag_start = None
        self._roi_pending = False

        print("p=person  o=object  SPACE=toggle  c=clear  q=quit")
        print("In object mode: click-drag to select ROI")
        cv2.namedWindow("ML Tracker")
        cv2.setMouseCallback("ML Tracker", self._on_mouse)

        try:
            while True:
                ret, frame_bgr = self.cap.read()
                if not ret:
                    time.sleep(0.1)
                    continue

                frame_bgr = cv2.flip(frame_bgr, 1)
                frame_rgb = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB)

                cx, cy, dist, conf = None, None, 0, 0

                if self.mode == "person":
                    # ── Person mode ─────────────────────
                    if self.person_det is None:
                        self.init_person_mode()
                    result = self.detect_person(frame_rgb)
                    if result:
                        cx, cy, dist, conf = result

                elif self.mode == "object":
                    # ── Object mode ─────────────────────
                    if self._roi_pending and self.roi:
                        self.init_object_tracker(frame_bgr, self.roi)
                        self._roi_pending = False

                    result = self.track_object(frame_bgr)
                    if result:
                        cx, cy, dist, conf = result

                # Send frame if tracking
                if (self.tracking and cx is not None
                        and conf > 20 and self.lost_count < 10):
                    x_off, y_off, dist2, conf2 = self.compute_frame(
                        cx, cy, dist, conf)
                    self.send_frame(x_off, y_off, dist2, conf2)

                self.check_serial_rx()
                self.draw_hud(frame_bgr, cx, cy, dist, conf)
                cv2.imshow("ML Tracker", frame_bgr)

                key = cv2.waitKey(1) & 0xFF
                if key == ord('q') or key == 27:
                    break
                elif key == ord(' '):
                    self.tracking = not self.tracking
                    print(f"Tracking {'ON' if self.tracking else 'OFF'}")
                elif key == ord('p'):
                    self.mode = "person"
                    self.smooth_xy.clear()
                    self.smooth_dist.clear()
                    print("[MODE] Person following")
                elif key == ord('o'):
                    self.mode = "object"
                    self.csrt = None
                    self.roi = None
                    self.smooth_xy.clear()
                    print("[MODE] Object following (click-drag ROI)")
                elif key == ord('c'):
                    self.roi = None
                    self.csrt = None
                    self.smooth_xy.clear()
                    self.smooth_dist.clear()
                    self.lost_count = 0
                    print("Cleared")

        except KeyboardInterrupt:
            pass
        finally:
            self.shutdown()

    def shutdown(self):
        print("\nShutting down...")
        if self.person_det:
            self.person_det.release()
        if self.ser and self.ser.is_open:
            self.ser.close()
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()


# ── CLI ────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="ML tracker: person (MediaPipe) + object (CSRT)")
    parser.add_argument("--port", default="/dev/serial0")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--no-serial", action="store_true")
    parser.add_argument("--mode", default="person",
                        choices=["person", "object"])
    parser.add_argument("--mp-complexity", type=int, default=0,
                        help="MediaPipe model complexity (0=lite, 1=full, 2=heavy)")
    args = parser.parse_args()

    tracker = MLTracker()
    tracker.frame_w = args.width
    tracker.frame_h = args.height
    tracker.mode = args.mode
    tracker.open_camera(args.camera)

    if not args.no_serial:
        tracker.open_serial(args.port, args.baud)

    if args.mode == "person":
        tracker.init_person_mode()

    tracker.run()


if __name__ == "__main__":
    main()
