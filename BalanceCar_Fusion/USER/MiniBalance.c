#include "stm32f10x.h"
#include "sys.h"
#include "control.h"
#include "show.h"
#include "filter.h"

/* ── Global variables ─────────────────────────────────── */
u8  Way_Angle = 2;           /* 1=DMP, 2=Kalman, 3=Complementary */
u8  Flag_Qian, Flag_Hou, Flag_Left, Flag_Right;
u8  Flag_sudu = 2;
u8  Flag_Stop = 1;
u8  Flag_Show = 0;
int Encoder_Left, Encoder_Right;
int Moto1, Moto2;
int Temperature;
int Voltage;
float Angle_Balance, Gyro_Balance, Gyro_Turn;
float Show_Data_Mb;
u32  Distance;
u8   delay_50, delay_flag, Bi_zhang = 0;
u8   PID_Send, Flash_Send;
float Acceleration_Z;

/* PID parameters used by balance()/velocity() in control.c */
float Balance_Kp  = 300;
float Balance_Kd  = 1;
float Velocity_Kp = 80;
float Velocity_Ki = 0.4;
float Turn_Kd     = 0;
float Dist_Kp     = -0.35;
float Dist_Ki     = -0.35 / 200;

u8   Uart_Ctrl_Flag = 0;
u8   UartCount = 0;
u16  PID_Parameter[10], Flash_Parameter[10];

/* ── main ──────────────────────────────────────────────── */
int main(void)
{
    /* 1. system clock + NVIC */
    delay_init();
    MY_NVIC_PriorityGroupConfig(2);

    /* 2. JTAG remap (free PB3/PB4/PA15 for OLED SPI) */
    JTAG_Set(JTAG_SWD_DISABLE);
    JTAG_Set(SWD_ENABLE);

    /* 3. I2C → MPU6050 → DMP (IMU must be first!) */
    IIC_Init();
    MPU6050_initialize();
    DMP_Init();

    /* 4. OLED (SPI: PB3/PB4/PB5/PA15) */
    OLED_Init();

    /* 5. motor PWM (TIM1, 10kHz) */
    MiniBalance_PWM_Init(7199, 0);

    /* 6. encoder (TIM2/TIM4) */
    Encoder_Init_TIM2();
    Encoder_Init_TIM4();

    /* 7. ultrasonic (TIM3 capture) */
    TIM3_Cap_Init(0xFFFF, 72 - 1);

    /* 8. UART2 → Raspberry Pi */
    uart2_init(9600);

    /* 9. UART3 → Bluetooth HC-06 */
    uart3_init(9600);

    /* 10. infrared tracking GPIO */
    Init_Infrared_Gpio();

    /* 11. beeper (PA4, shared with LED) */
    BeepDeviceInit();

    /* 12. key + LED */
    KEY_Init();
    LED_Init();

    /* 13. ADC (battery voltage) */
    Adc_Init();

    /* 14. PID params init + Flash restore */
    InitPIDParams();
    PID_Init(&dist, POSITION_PID, dist_pid.kp, dist_pid.ki, 0);
    Flash_Read();

    /* 15. USART1 (debug/printf, 128kbaud) */
    uart_init(128000);

    /* 16. MPU6050 EXTI (5ms periodic interrupt) — last! */
    MiniBalance_EXTI_Init();

    /* ── main loop ──────────────────────────────────── */
    while (1)
    {
        if (Flash_Send == 1)
        {
            Flash_Write();
            Flash_Send = 0;
        }

        if (Flag_Show == 0)
        {
            APP_Show();
            oled_show();
        }
        else
        {
            DataScope();
        }

        delay_flag = 1;
        delay_50 = 0;
        while (delay_flag);   /* wait for MPU6050 INT 50ms tick */
    }
}
