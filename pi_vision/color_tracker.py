#!/usr/bin/env python3
"""
Color-based object tracking for balancing car visual following.
Phase 1: HSV color blob tracking via OpenCV, sends position data to
STM32 over UART (USART2, 9600 8N1).

Protocol (Pi -> STM32, 6 bytes):
  [0xA5, x_offset(int8), y_offset(int8), distance(uint8),
   confidence(uint8), xor_checksum]

Controls:
  Click image  -> select target color at that pixel
  SPACE        -> toggle tracking on/off
  'c'          -> clear target color
  'q' / ESC    -> quit
"""

import argparse
import os
import struct
import sys
import time

import cv2
import numpy as np

# ── serial (graceful fallback if not on Pi) ──────────────────
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False


# ── protocol constants ────────────────────────────────────────
FRAME_HEADER = 0xA5
FRAME_LEN = 6
REVERSE_HEADER = 0xB0
CMD_LEARN = 0x01
CMD_CLEAR = 0x02
CMD_STOP = 0x03

# ── HSV range for color detection ─────────────────────────────
HSV_RANGE = 15   # +/- hue tolerance
SAT_MIN = 80     # minimum saturation
VAL_MIN = 50     # minimum value
MIN_CONTOUR_AREA = 200


def make_frame(x_offset, y_offset, distance, confidence):
    """Build and return a 6-byte frame with XOR checksum."""
    x = max(-128, min(127, int(x_offset)))
    y = max(-128, min(127, int(y_offset)))
    d = max(0, min(255, int(distance)))
    c = max(0, min(100, int(confidence)))
    payload = bytes([FRAME_HEADER, x & 0xFF, y & 0xFF, d, c])
    checksum = 0
    for b in payload:
        checksum ^= b
    return payload + bytes([checksum])


class ColorTracker:
    """Track the largest blob matching a calibrated HSV color."""

    def __init__(self):
        self.target_hsv = None     # (H, S, V) calibrated center
        self.tracking = False
        self.cap = None
        self.ser = None
        self.frame_w = 640
        self.frame_h = 480
        self._last_send = 0        # throttle serial writes

    # ── setup ──────────────────────────────────────────────
    def open_camera(self, index=0):
        self.cap = cv2.VideoCapture(index)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.frame_w)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.frame_h)
        if not self.cap.isOpened():
            raise RuntimeError(f"Cannot open camera index {index}")

    def open_serial(self, port="/dev/serial0", baud=9600):
        if not HAS_SERIAL:
            print("[WARN] pyserial not installed — serial disabled")
            return
        self.ser = serial.Serial(port, baud, timeout=0.05,
                                 bytesize=serial.EIGHTBITS,
                                 parity=serial.PARITY_NONE,
                                 stopbits=serial.STOPBITS_ONE)
        print(f"[SERIAL] {port} @ {baud} 8N1")

    # ── color calibration ──────────────────────────────────
    def calibrate(self, hsv_img, px, py):
        """Set target color from a pixel in the HSV image."""
        h, s, v = hsv_img[py, px]
        if s < 30 or v < 30:
            print(f"  Pixel too desaturated/dark (S={s}, V={v}), ignoring")
            return
        self.target_hsv = (int(h), int(s), int(v))
        self.tracking = False  # restart tracking with new color
        print(f"  Target HSV = ({h:.0f}, {s:.0f}, {v:.0f})")

    # ── tracking core ───────────────────────────────────────
    def find_object(self, hsv_img):
        """Return (cx, cy, area, mask) or (0,0,0,None) if not found."""
        if self.target_hsv is None:
            return 0, 0, 0, None

        h0, s0, v0 = self.target_hsv
        lower = np.array([
            max(0, h0 - HSV_RANGE),
            max(0, s0 - 30),
            max(0, v0 - 40),
        ], dtype=np.uint8)
        upper = np.array([
            min(179, h0 + HSV_RANGE),
            min(255, s0 + 30),
            min(255, v0 + 40),
        ], dtype=np.uint8)

        # Handle hue wrap-around for red-ish colors near 0/180
        if lower[0] < 0:
            lower1 = lower.copy(); lower1[0] = 0
            upper1 = upper.copy()
            lower2 = lower.copy(); lower2[0] = 179 + lower[0]
            upper2 = upper.copy(); upper2[0] = 179
            mask = cv2.inRange(hsv_img, lower1, upper1)
            mask |= cv2.inRange(hsv_img, lower2, upper2)
        elif upper[0] > 179:
            lower1 = lower.copy()
            upper1 = upper.copy(); upper1[0] = 179
            mask = cv2.inRange(hsv_img, lower1, upper1)
            lower2 = lower.copy(); lower2[0] = 0
            upper2 = upper.copy(); upper2[0] = upper[0] - 179
            mask |= cv2.inRange(hsv_img, lower2, upper2)
        else:
            mask = cv2.inRange(hsv_img, lower, upper)

        # Morphological open to reduce noise
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel, iterations=1)
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=2)

        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL,
                                       cv2.CHAIN_APPROX_SIMPLE)
        if not contours:
            return 0, 0, 0, mask

        largest = max(contours, key=cv2.contourArea)
        area = cv2.contourArea(largest)
        if area < MIN_CONTOUR_AREA:
            return 0, 0, 0, mask

        M = cv2.moments(largest)
        if M["m00"] <= 0:
            return 0, 0, 0, mask
        cx = int(M["m10"] / M["m00"])
        cy = int(M["m01"] / M["m00"])
        return cx, cy, area, mask

    def compute_frame(self, cx, cy, area):
        """Convert tracking result to protocol fields."""
        if area < MIN_CONTOUR_AREA:
            return 0, 0, 0, 0

        # X offset: -128 (far left) to +127 (far right)
        x_center = self.frame_w / 2.0
        x_offset = (cx - x_center) / x_center * 127.0

        # Y offset: track vertical deviation
        y_center = self.frame_h / 2.0
        y_offset = (cy - y_center) / y_center * 127.0

        # Distance estimate from bounding box area (tuned empirically)
        # area ∝ 1/distance²  →  distance = k / sqrt(area)
        distance = 8000.0 / max(np.sqrt(area), 1.0)
        distance = max(5, min(255, distance))

        # Confidence from area relative to full frame
        frame_area = self.frame_w * self.frame_h
        confidence = min(100, int(area / frame_area * 400))

        return x_offset, y_offset, distance, confidence

    def send_frame(self, x_offset, y_offset, distance, confidence):
        """Send tracking frame over serial, throttled to ~10 Hz."""
        now = time.time()
        if now - self._last_send < 0.08:
            return
        self._last_send = now

        frame = make_frame(x_offset, y_offset, distance, confidence)
        if self.ser and self.ser.is_open:
            try:
                self.ser.write(frame)
            except serial.SerialException:
                pass

    def check_serial_rx(self):
        """Check for reverse commands from STM32 (non-blocking)."""
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

    # ── UI drawing ─────────────────────────────────────────
    def draw_hud(self, frame, cx, cy, area, mask, x_off, y_off, dist, conf):
        """Overlay tracking information on the frame."""
        h, w = frame.shape[:2]

        # Crosshair at center
        cv2.line(frame, (w // 2 - 15, h // 2), (w // 2 + 15, h // 2),
                 (128, 128, 128), 1)
        cv2.line(frame, (w // 2, h // 2 - 15), (w // 2, h // 2 + 15),
                 (128, 128, 128), 1)

        # Show mask as inset
        if mask is not None:
            mask_rgb = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR)
            small = cv2.resize(mask_rgb, (120, 90))
            frame[10:10 + 90, 10:10 + 120] = small
            cv2.rectangle(frame, (8, 8), (132, 102), (80, 80, 80), 1)

        # Status bar
        if self.target_hsv is None:
            status = "Click to select target color"
            color = (80, 80, 255)
        elif self.tracking:
            status = "TRACKING"
            color = (0, 255, 0)
        else:
            status = "PAUSED (SPACE to resume)"
            color = (0, 200, 255)
        cv2.putText(frame, status, (10, h - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

        # Target indicator and telemetry
        if area >= MIN_CONTOUR_AREA:
            cv2.circle(frame, (cx, cy), 10, (0, 255, 0), 2)
            cv2.circle(frame, (cx, cy), 2, (0, 255, 0), -1)
            tele = f"X={x_off:+.0f} D={dist:.0f}cm C={conf}%"
            cv2.putText(frame, tele, (cx - 60, cy - 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 0), 1)

        # Target HSV preview
        if self.target_hsv is not None:
            hsv_preview = np.full((20, 30, 3), self.target_hsv, dtype=np.uint8)
            preview_bgr = cv2.cvtColor(hsv_preview, cv2.COLOR_HSV2BGR)
            frame[h - 40:h - 20, 10:40] = preview_bgr

    # ── main loop ──────────────────────────────────────────
    def run(self):
        if self.cap is None:
            self.open_camera()

        print("Controls: click=select color  SPACE=toggle track  c=clear  q=quit")
        cv2.namedWindow("Color Tracker")
        cv2.setMouseCallback("Color Tracker", self._on_click)

        try:
            while True:
                ret, frame = self.cap.read()
                if not ret:
                    print("Camera read error, retrying...")
                    time.sleep(0.1)
                    continue

                # Flip horizontally for more intuitive following
                frame = cv2.flip(frame, 1)
                hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

                # Always try to detect (even when paused, for visual feedback)
                cx, cy, area, mask = self.find_object(hsv)
                x_off, y_off, dist, conf = self.compute_frame(cx, cy, area)

                if self.tracking and area >= MIN_CONTOUR_AREA:
                    self.send_frame(x_off, y_off, dist, conf)
                elif self.tracking:
                    # Send zero-confidence frame when target lost
                    self.send_frame(0, 0, 0, 0)

                self.check_serial_rx()
                self.draw_hud(frame, cx, cy, area, mask,
                              x_off, y_off, dist, conf)
                cv2.imshow("Color Tracker", frame)

                key = cv2.waitKey(1) & 0xFF
                if key == ord('q') or key == 27:          # ESC
                    break
                elif key == ord(' '):
                    self.tracking = not self.tracking
                    status = "ON" if self.tracking else "OFF"
                    print(f"Tracking {status}")
                elif key == ord('c'):
                    self.target_hsv = None
                    self.tracking = False
                    print("Target cleared")

        except KeyboardInterrupt:
            pass
        finally:
            self.shutdown()

    def _on_click(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            if self.cap is None:
                return
            ret, frame = self.cap.read()
            if not ret:
                return
            frame = cv2.flip(frame, 1)
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            self.calibrate(hsv, x, y)

    def shutdown(self):
        print("\nShutting down...")
        if self.ser and self.ser.is_open:
            self.ser.close()
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()


# ── entry point ───────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="Color-based tracking for balancing car vision following")
    parser.add_argument("--port", default="/dev/serial0",
                        help="Serial port (default: /dev/serial0)")
    parser.add_argument("--baud", type=int, default=9600,
                        help="Serial baud rate (default: 9600)")
    parser.add_argument("--camera", type=int, default=0,
                        help="Camera index (default: 0)")
    parser.add_argument("--width", type=int, default=640,
                        help="Camera width")
    parser.add_argument("--height", type=int, default=480,
                        help="Camera height")
    parser.add_argument("--no-serial", action="store_true",
                        help="Disable serial (test mode)")
    parser.add_argument("--hsv", type=int, nargs=3, metavar=("H", "S", "V"),
                        help="Preset target HSV color")
    args = parser.parse_args()

    tracker = ColorTracker()
    tracker.frame_w = args.width
    tracker.frame_h = args.height
    tracker.open_camera(args.camera)

    if not args.no_serial:
        tracker.open_serial(args.port, args.baud)

    if args.hsv:
        tracker.target_hsv = tuple(args.hsv)
        print(f"Preset target HSV: {tracker.target_hsv}")

    tracker.run()


if __name__ == "__main__":
    main()
