# STM32 两轮自平衡小车

基于 STM32F103C8T6 的两轮自平衡小车，共六种运行模式，支持蓝牙遥控、超声波跟随、红外循迹和树莓派视觉跟随（颜色追踪 / 人体检测）。内置提起、着陆、倒地检测等自保护逻辑。PCB 开源在立创开源硬件平台。

## 运行模式

| 模式 | 指示灯 | 功能 |
|------|--------|------|
| 0 平衡 | 白灯 | 原地平衡站立 |
| 1 蓝牙+避障 | 蓝灯 | HC-06 蓝牙遥控 + 超声波避障 |
| 2 超声波跟随 | 红灯 | 超声波测距自动跟随 |
| 3 光电寻迹 | — | 红外对管循迹 |
| 4 纯蓝牙遥控 | — | 无避障的纯遥控 |
| 5 视觉跟随 | — | 树莓派视觉跟随（Phase 1 颜色 / Phase 2 人体骨骼） |

## 仓库结构

```
BalanceCar_Fusion/    Keil MDK 固件工程（STM32F103C8T6）
pi_vision/            树莓派视觉端（Python + OpenCV/MediaPipe）
pcb/                  EasyEDA 融合底板原理图自动化脚本
create_schematic.mjs  通过 EasyEDA API 生成平衡车底板原理图
docs 相关文档见 BalanceCar_Fusion/docs/软件设计说明.docx
```

## 固件（BalanceCar_Fusion）

### 构建

Keil MDK 工程，工程文件 `BalanceCar_Fusion/USER/MiniBalance.uvprojx`。在 Keil 中打开构建，烧录后重新上电生效。

### 架构

```
         EXTI 中断（MPU6050 5ms 数据就绪）
                  │
                  ▼
         main loop ──► 姿态解算（卡尔曼 / DMP / 互补，默认卡尔曼）
                  │
        ┌─────────┼──────────┐
        ▼         ▼          ▼
    balance()  提起/倒地  bluetooth/vision
    (三环PID)   保护检测    指令处理
```

初始化顺序固定：IMU 必须最先初始化，MPU6050 EXTI 中断最后开启。

### 控制系统

三环串级 PID，位于 `BalanceCar_Fusion/MiniBalance/CONTROL/control.c`：

1. **直立环**（PD）：角度外环，Kp=300, Kd=1
2. **速度环**（PI）：速度内环，Kp=80, Ki=0.4，编码器积分带低通滤波
3. **转向环**（PD）：偏航控制，转向输出叠加在差速上

输出合成：`pwm = upright_out - upright_kp × speed_out ± turn_out`。

跟随模式下距离环把超声波读数换算为速度环目标；视觉跟随模式下树莓派发来的 `distance` 与 `x_offset` 直接改写速度环与转向环目标（`confidence > 30` 才生效）。

### 保护逻辑

- 偏角超过 ±40° 或电池电压过低：关闭电机并停止积分
- 提起检测：车轮离空（编码器速度与姿态不符）时切断输出
- 倒地/停止时清零速度环积分，防止重启瞬间电机乱转

### 代码分层

| 层 | 目录 | 说明 |
|---|---|---|
| 入口 | `USER/` | `MiniBalance.c` 主程序 + Keil 工程文件 |
| 应用 | `MiniBalance/` | `CONTROL/` 三环PID、`pid/` 通用PID、`filter/` 姿态滤波、`show/` OLED显示 |
| 模块驱动 | `MiniBalance_HARDWARE/` | mpu6050+DMP、motor、encoder、usart2(树莓派)、usart3(HC-06)、infrared、oled、beep、key、adc 等 |
| BSP | `MiniBalance_COER/` | GPIO/定时器/延时等底层驱动 + CMSIS 启动文件 |
| 标准库 | `STM32F10x_FWLib/`、`SYSTEM/` | ST 标准外设库 |

## 视觉跟随（pi_vision）

树莓派 4B + USB 摄像头，经 USART2（PA2/PA3，9600 8N1）与 STM32 通信：

- **Phase 1 `color_tracker.py`**：HSV 颜色掩码追踪色块，鼠标点击拾取目标色，轻量但受光照影响
- **Phase 2 `ml_tracker.py`**：MediaPipe Pose 检测 33 个人体关键点，躯干质心定位 + 肩宽估算距离，光照鲁棒、自动识别人体

通信协议：

```
Pi → STM32（6字节帧）：0xA5, x_offset(i8), y_offset(i8), distance(u8), confidence(u8), XOR校验
STM32 → Pi（2字节）：  0xB0, cmd（0x01=LEARN 0x02=CLEAR 0x03=STOP）
```

树莓派端部署：

```bash
pip install opencv-python pyserial numpy mediapipe
python ml_tracker.py --mode person
```

详细说明见 [`pi_vision/README.md`](pi_vision/README.md)。
