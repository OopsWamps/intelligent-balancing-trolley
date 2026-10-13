# STM32 两轮自平衡小车

基于 STM32F103C8T6 的两轮自平衡小车，三种运行模式：

| 模式 | 指示灯 | 功能 |
|------|--------|------|
| 平衡模式 | 白灯 | 静态自平衡 |
| 蓝牙避障 | 蓝灯 | HC-06 蓝牙遥控 + HCSR04 超声波避障 |
| 超声波跟随 | 红灯 | 超声波测距跟随 |

内置提起 / 着陆 / 倒地检测等自保护逻辑。PCB 开源在立创开源硬件平台。

## 构建

Keil MDK 工程，工程文件：`BalanceCar_Fusion/USER/MiniBalance.uvprojx`。在 Keil 中打开构建，烧录后重新上电生效。

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

### 控制系统

三环串级 PID（`BalanceCar_Fusion/MiniBalance/CONTROL/control.c`）：

1. **直立环**（PD）：角度外环，kp=240, kd=0.93，目标角 0.8°
2. **速度环**（PI）：速度内环，带低通滤波
3. **转向环**（PD）：偏航控制

输出合成：`pwm = upright_out - upright_kp × speed_out ± turn_out`。跟随模式额外用距离 PID 修改速度环目标值。

### 代码分层

| 层 | 目录 | 说明 |
|---|---|---|
| 入口 | `USER/` | main.c 主循环 + 中断处理 |
| 应用 | `MiniBalance/CONTROL/` 等 | PID 计算、模式切换、保护检测 |
| 模块驱动 | `MiniBalance_HARDWARE/` | mpu6050(DMP)、motor、encoder、hcsr04、hc06、oled 等 |
| BSP | `MiniBalance_COER/` | 底层外设驱动 + CMSIS 启动文件 |

## 目录结构

```
BalanceCar_Fusion/    Keil MDK 主工程（固件源码）
pi_vision/            Python 视觉端（颜色追踪 / 机器学习追踪）
create_schematic.mjs  通过 EasyEDA API 生成平衡车底板原理图
```
