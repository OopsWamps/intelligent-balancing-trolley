# 视觉跟随系统

树莓派 4B 端视觉跟随程序，通过 OpenCV/MediaPipe 实现目标检测，经 UART 发送跟踪指令给 STM32 平衡车主控。

## 硬件连接

```
树莓派 4B                   STM32F103C8T6 (Forest S1)
  GPIO14 (TXD)  ──────────►  PA3 (USART2 RX)
  GPIO15 (RXD)  ◄──────────  PA2 (USART2 TX)
  GND           ───────────  GND
```

- 波特率: 9600 bps, 8N1
- 摄像头: USB 摄像头 或 乐视三合一体感摄像头

## 安装

```bash
pip install opencv-python pyserial numpy

# Phase 2 额外依赖 (人体检测)
pip install mediapipe
```

MediaPipe 人体模型文件已预下载到 `models/pose_landmarker_lite.task` (5.5MB)。

## Phase 1: 颜色追踪

```bash
python color_tracker.py
```

**原理：**
1. 将摄像头画面转为 HSV 色彩空间
2. 根据用户点击选中的目标颜色，生成 HSV 掩码 (目标 Hue ±15, Saturation >80, Value >50)
3. 形态学操作（开运算 + 闭运算）消除噪点
4. 查找最大轮廓，计算质心坐标
5. 根据质心偏离画面中心的距离，换算为 X/Y 偏移量
6. 通过包围盒面积估算距离 (distance ∝ 1/√area)
7. 以 ~12Hz 频率通过串口发送 6 字节跟踪帧

**操作：**

| 按键 | 功能 |
|------|------|
| 鼠标左键 | 点击画面中的有色目标，拾取 HSV 颜色 |
| SPACE | 开启/关闭追踪 |
| c | 清除目标颜色 |
| q / ESC | 退出 |

**优劣：**
- ✅ 轻量级，纯 OpenCV 实现，树莓派可稳定跑 30fps
- ✅ 色块锁定快，适合单一颜色目标（如红色球、蓝色外套）
- ❌ 光照变化敏感（HSV 在暗光下分割效果差）
- ❌ 相似颜色物体干扰（背景中出现同色物体会导致误跟踪）
- ❌ 目标丢失后无法自动恢复（需重新点击选择）

**调参：** 修改脚本内常量
- `HSV_RANGE` — 色相容差（默认 15，增大可容忍更大颜色偏差）
- `SAT_MIN` / `VAL_MIN` — 最小饱和度/亮度（降低可在暗光下工作）
- `MIN_CONTOUR_AREA` — 最小轮廓面积（防止噪点误检）

---

## Phase 2: ML 人体跟随

```bash
python ml_tracker.py --mode person
```

**原理：**
1. 使用 Google MediaPipe Pose Landmarker (轻量 TFLite 模型) 检测 33 个人体骨骼关键点
2. 取双肩 (11/12) + 双髋 (23/24) 四个关键点计算躯干质心作为跟踪目标
3. 根据双肩像素宽度，利用三角测距原理估算距离: `distance = 肩宽(cm) × 焦距(px) / 肩宽(px)`
4. 默认肩宽 42cm（成人均值），焦距 600px（需根据实际摄像头标定）
5. 对 X/Y/距离做 5 帧滑动平滑，减少抖动
6. 目标丢失时降置信度，连续丢失 10 帧以上停止发送指令
7. 也可切换到 **CSRT 对象追踪模式**：按 `o` 键，鼠标框选 ROI，使用 OpenCV CSRT 跟踪器锁定任意物体

**操作：**

| 按键 | 功能 |
|------|------|
| p | 切换到人体跟随模式（自动检测人体） |
| o | 切换到对象追踪模式（鼠标框选 ROI） |
| SPACE | 开启/关闭追踪 |
| c | 清除目标 |
| q / ESC | 退出 |

**优劣：**
- ✅ 光照鲁棒性强，不受环境光线影响
- ✅ 人体自动检测，无需手动选目标
- ✅ 33 点骨骼关键点可扩展（例如后续可支持手势控制）
- ✅ 距离估算比包围盒面积法更准确（基于人体先验尺寸）
- ❌ MediaPipe 模型在 Pi 4B 上约 10-15fps，不如颜色追踪流畅
- ❌ 模型文件 5.5MB，需额外下载
- ❌ 多人场景只跟踪第一个检测到的人

**调参：** 修改脚本内常量
- `AVG_SHOULDER_WIDTH_CM` — 平均肩宽（默认 42.0，儿童需调小）
- `FOCAL_LENGTH_PX` — 相机焦距（默认 600，需标定）

---

## 通信协议

### Pi → STM32 (6 字节)

| 偏移 | 字段 | 类型 | 说明 |
|------|------|------|------|
| 0 | header | uint8 | 帧头 0xA5 |
| 1 | x_offset | int8 | 水平偏移，-128(最左) ~ 0(居中) ~ +127(最右) |
| 2 | y_offset | int8 | 垂直偏移 |
| 3 | distance | uint8 | 估算距离 (cm)，0~255 |
| 4 | confidence | uint8 | 置信度，0~100% |
| 5 | checksum | uint8 | 前 5 字节 XOR 校验和 |

### STM32 → Pi (2 字节)

| 偏移 | 字段 | 说明 |
|------|------|------|
| 0 | header | 帧头 0xB0 |
| 1 | cmd | 0x01=LEARN (进入视觉模式) / 0x02=CLEAR / 0x03=STOP |

## 测试

在没有树莓派和串口的环境下，可用 `--no-serial` 在 PC 上验证追踪效果：

```bash
# 测试颜色追踪
python color_tracker.py --no-serial

# 测试人体追踪
python ml_tracker.py --no-serial --mode person

# 测试对象追踪 (不需要 MediaPipe)
python ml_tracker.py --no-serial --mode object

# 预设目标颜色
python color_tracker.py --no-serial --hsv 120 200 180
```
