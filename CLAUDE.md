# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

STM32F103C8T6 两轮自平衡小车，六种运行模式。包含提起/着陆/倒地检测等自保护功能。PCB 开源在立创开源硬件平台。

| 模式 | 指示灯 | 说明 |
|------|--------|------|
| 0 平衡 | 白灯 | 原地平衡站立 |
| 1 蓝牙+避障 | 蓝灯 | 手机APP遥控 + 超声波避障 |
| 2 超声波跟随 | 红灯 | 超声波测距自动跟随 |
| 3 光电寻迹 | — | 红外对管循迹 |
| 4 纯蓝牙遥控 | — | 无避障的纯遥控 |
| 5 视觉跟随 | — | 树莓派 OpenCV/MediaPipe 视觉跟随（Phase 1 颜色 / Phase 2 ML人体） |

## 构建

Keil MDK 工程，使用 ARMCC V5 编译器。工程文件：`STM32_Balance_Car-standard/Project.uvprojx`。在 Keil 中打开构建，不支持命令行编译。烧录后需重新上电代码才生效。

## 架构

```
         EXTI0 中断（MPU6050 数据就绪）
                  │
                  ▼
         main loop ──► MPU_Get_Angle()
                  │
        ┌─────────┼──────────┐──────────┐
        ▼         ▼          ▼          ▼
    Balance()  CheckLift  CheckFall  ModeSelect()
    (3环PID)   DetectPut  Down()
```

### 代码分层

| 层 | 目录 | 说明 |
|---|---|---|
| 入口 | `User/` | `main.c` 主循环 + 中断处理；`headfile.h` 统一头文件 |
| 应用 | `App/` | 控制逻辑：`control/`(PID参数与计算)、`mode/`(模式切换+保护检测)、`pid/`(通用PID算法)、`utils/`(系统初始化+声光报警) |
| 模块驱动 | `Modules/` | 各硬件模块驱动：`mpu6050/`(含DMP)、`motor/`、`encoder/`、`hcsr04/`、`hc06/`、`oled/`、`led/`、`beep/`、`key/` |
| BSP | `Bsp/` | 底层外设驱动：`gpio/`、`iic/`、`pwm/`、`tim/`、`usart/`、`exti/`、`delay/`、`sys/` |
| 标准库 | `Library/` | STM32标准外设库 |
| 启动 | `Start/` | CMSIS Cortex-M3 内核 + 启动文件 |

### 控制系统（核心）

三环串级 PID，在 `App/control/control.c` 中定义参数和计算：

1. **直立环**（PD）：`AnglePidCtrl()` — 角度外环，kp=240, kd=0.93，目标角 0.8°
2. **速度环**（PI）：`SpeedPidCtrl()` — 速度内环，kp=-0.62, ki=-0.0031，带低通滤波
3. **转向环**（PD）：`TurnPidCtrl()` — 偏航控制，kd=0.35, kp=-30

输出合成：`pwm = upright_out - upright_kp × speed_out ± turn_out`

模式 2（跟随模式）额外使用 `DistPidCtrl()` 通过超声波距离控制速度环目标值。

### 关键约束

- **初始化顺序不可更改**：IMU 必须最先初始化，否则无法平衡（`utils.c` 中的 `System_Init()`）
- `stop_flag` 在倒地/偏角过大时置位，用于清零速度环积分，防止重启时电机乱转
- 模式切换时调用 `DataClear()` 重置 PID 状态

## EasyEDA 集成

`create_schematic.mjs` 通过 EasyEDA API 自动创建平衡车底板原理图。配套 EasyEDA 技能包在 `.claude/skills/easyeda-api/`，提供 120+ 类的 API 参考和 WebSocket 桥接服务器。

技能触发词："嘉立创EDA，启动！"、"EasyEDA"、"PCB"、"原理图"等。激活后首行回复 `📋 EasyEDA Session`。

## 硬件参考资料

`现有实物/小车硬件原理图/` 包含：STM32 最小系统、MPU6050 模块、TB6612 驱动、12V→5V 稳压、精密电阻电池电压测量、平衡车底板原理图。

## 视觉跟随系统

树莓派 4B + 摄像头通过 USART2 (PA2/PA3, 9600 8N1) 与 STM32 通信，两阶段实现：

### Phase 1: 颜色追踪 (`pi_vision/color_tracker.py`)
- **原理**：HSV 颜色空间掩码 → 形态学开闭运算去噪 → 最大轮廓质心计算
- **操作**：鼠标点击画面拾取目标颜色 → SPACE 开启追踪 → 实时发送偏移指令
- **优点**：轻量（仅 OpenCV）、无需 GPU、色块锁定快
- **劣势**：受光照影响大、相似颜色干扰、目标丢失后无法恢复

### Phase 2: ML 追踪 (`pi_vision/ml_tracker.py`)
- **原理**：MediaPipe Pose Landmarker 检测 33 个人体骨骼关键点 → 躯干质心 + 肩宽三角测距
- **操作**：启动即自动检测人体 → SPACE 开启追踪；也可按 `o` 切换到 CSRT 通用对象追踪（鼠标框选 ROI）
- **优点**：光照鲁棒、自动检测人体无需手动选目标、3D 距离估算准
- **劣势**：需下载模型文件（5.5MB）、CPU 占用较高（Pi 4B ~15fps）

### 通信协议

**Pi → STM32（6 字节帧）：**
```
[0xA5, x_offset(int8), y_offset(int8), distance(uint8), confidence(uint8), XOR_checksum]
```

**STM32 → Pi（反向指令，2 字节）：**
```
[0xB0, cmd]   cmd: 0x01=LEARN(开始追踪) 0x02=CLEAR(清除) 0x03=STOP(停止)
```

### 树莓派部署
```bash
pip install opencv-python pyserial numpy mediapipe
python pi_vision/ml_tracker.py --mode person
# 或测试模式（不开串口）
python pi_vision/ml_tracker.py --no-serial --mode person
```
