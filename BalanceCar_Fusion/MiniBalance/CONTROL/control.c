#include "control.h"
#include "filter.h"

int Balance_Pwm, Velocity_Pwm, Turn_Pwm;
u8 Flag_Target;
u32 Flash_R_Count;
int Voltage_Temp, Voltage_Count, Voltage_All;

PIDParam_t upright_pid = { .kp = 300, .kd = 1, .out = 0, .tar = 0 };
PIDParam_t speed_pid   = { .kp = 80, .ki = 0.4, .out = 0, .filter = 0.8, .tar = 0 };
PIDParam_t turn_pid    = { .kd = 0,  .kp = 42, .out = 0, .tar = 0 };
PIDParam_t dist_pid    = { .kp = -0.35, .ki = -0.35/200, .out = 0, .tar = 40 };

PID_t dist;
BalanceState_t balance_state = {
    .lifted_flag = 0,
    .putdown_counter = 0,
    .lifted_counter = 0,
    .balance_enable = 1,
    .mode = 0
};

float pwm_out, PWMA_float, PWMB_float;
static uint8_t obstacle_blocked = 0;

static float Encoder_Err, filtered_Err, last_filtered_Err, Encoder_S;

/* ===============  safety detection =============== */

void CheckLiftState(void)
{
    if (fabs(Angle_Balance) > 40.0f && abs((int)Gyro_Balance) > 100 && (Encoder_Left + Encoder_Right) > 50)
    {
        balance_state.lifted_counter++;
        if (balance_state.lifted_counter > 30)
        {
            balance_state.lifted_flag = 1;
            balance_state.balance_enable = 0;
            balance_state.lifted_counter = 0;
            Moto1 = 0; Moto2 = 0;
            AIN1 = 0; AIN2 = 0;
            BIN1 = 0; BIN2 = 0;
        }
    }
}

void DetectPutDown(void)
{
    if (balance_state.lifted_flag || Flag_Stop)
    {
        if (fabs(Angle_Balance) < 20.0f && abs((int)Gyro_Balance) < 150 && abs(Encoder_Left + Encoder_Right) < 120)
        {
            if (balance_state.putdown_counter++ > 30)
            {
                balance_state.lifted_flag = 0;
                balance_state.putdown_counter = 0;
                balance_state.balance_enable = 1;
                Flag_Stop = 0;
            }
        }
        else
        {
            balance_state.putdown_counter = 0;
        }
    }
}

void CheckFallDown(void)
{
    if (fabs(upright_pid.tar - Angle_Balance) > 70.0f && Flag_Stop == 0)
    {
        balance_state.balance_enable = 0;
        AIN1 = 0; AIN2 = 0;
        BIN1 = 0; BIN2 = 0;
    }
}

void ObstacleAvoid(void)
{
    Read_Distane();
    if (Distance > 0 && Distance <= 60)
    {
        if (!obstacle_blocked && Distance < 25)
        {
            obstacle_blocked = 1;
        }
        else if (obstacle_blocked && Distance > 40)
        {
            obstacle_blocked = 0;
        }
    }
}

/* ===============  mode & balance wrapper =============== */

void ModeSelect(void)
{
    static uint8_t last_mode = 0xFF;

    if (balance_state.mode != last_mode)
    {
        DataClear();
        last_mode = balance_state.mode;
    }

    switch (balance_state.mode)
    {
    case MODE_BALANCE:
        speed_pid.ki = 0.4;
        Led_Flash(100);
        break;
    case MODE_BT_AVOID:
        speed_pid.ki = 0;
        Led_Flash(0);
        Bi_zhang = 1;
        Read_Distane();
        if (Distance > 0 && Distance < 25)
        {
            if (Flag_Qian == 1)
            {
                Flag_Qian = 0; Flag_Hou = 0; Flag_Left = 0; Flag_Right = 1;
            }
            else if (Flag_Qian == 0 && Flag_Hou == 0 && Flag_Left == 0 && Flag_Right == 0)
            {
                Flag_Hou = 1;
            }
            SetBeepMode(BEEP_SYSTEM, BEEP_ON);
        }
        else
        {
            SetBeepMode(BEEP_SYSTEM, BEEP_OFF);
        }
        break;
    case MODE_FOLLOW:
        speed_pid.ki = 0;
        Led_Flash(0);
        if (Distance > 0 && Distance <= 120) DistPidCtrl();
        else speed_pid.tar = 0;
        Read_Distane();
        break;
    case MODE_TRACE:
        speed_pid.ki = 0.4;
        Led_Flash(100);
        tracking();
        break;
    case MODE_BT_ONLY:
        speed_pid.ki = 0;
        Led_Flash(0);
        Bi_zhang = 0;
        break;
    case MODE_VISION:
        speed_pid.ki = 0;
        Led_Flash(0);
        if (pi_vision.fresh && pi_vision.confidence > 30)
        {
            turn_pid.tar = (float)pi_vision.x_offset * 0.3f;
            if (pi_vision.distance > 0)
                dist.target = pi_vision.distance;
            pi_vision.fresh = 0;
        }
        else
        {
            turn_pid.tar = 0;
        }
        break;
    default:
        break;
    }
}

void Balance(void)
{
    if (balance_state.balance_enable)
    {
        ModeSelect();

        Balance_Pwm = balance(Angle_Balance, Gyro_Balance);
        Velocity_Pwm = velocity(Encoder_Left, Encoder_Right);
        Turn_Pwm = turn(Encoder_Left, Encoder_Right, Gyro_Turn);

        Moto1 = -Balance_Pwm + Velocity_Pwm - Turn_Pwm;
        Moto2 = -Balance_Pwm + Velocity_Pwm + Turn_Pwm;

        Xianfu_Pwm();

        if (Turn_Off(Angle_Balance, Voltage) == 0)
            Set_Pwm(Moto1, Moto2);
    }
}

/* ===============  distance PID =============== */

void DistPidCtrl(void)
{
    dist.target = dist_pid.tar;
    dist.now = (float)Distance;
    PidCalucate(&dist);
    speed_pid.tar = dist.out;
}

void PWMLimit(float PWMA, float PWMB)
{
    if (PWMA > MAXPWM) PWMA = MAXPWM;
    if (PWMA < -MAXPWM) PWMA = -MAXPWM;
    if (PWMB > MAXPWM) PWMB = MAXPWM;
    if (PWMB < -MAXPWM) PWMB = -MAXPWM;
}

void DataClear(void)
{
    Encoder_Err = 0; filtered_Err = 0; last_filtered_Err = 0; Encoder_S = 0;
    dist.iout = 0; dist.out = 0;
}

/* ===============  main control interrupt =============== */

int EXTI15_10_IRQHandler(void)
{
    if (INT == 0)
    {
        EXTI->PR = 1 << 12;
        Flag_Target = !Flag_Target;

        if (delay_flag == 1)
        {
            if (++delay_50 >= 10) delay_50 = 0, delay_flag = 0;
        }

        Get_Angle(Way_Angle);

        if (Flag_Target == 1) return 0;   /* 10ms once */

        Encoder_Left = Read_Encoder(2);
        Encoder_Right = Read_Encoder(4);

        /* key for mode switching */
        Key();

        /* safety checks */
        CheckLiftState();
        DetectPutDown();
        CheckFallDown();

        /* main balance control with mode */
        Balance();
    }
    return 0;
}

/* ===============  PID loops (original 3.5) =============== */

int balance(float Angle, float Gyro)
{
    float Bias;
    int balance_val;
    Bias = Angle - ZHONGZHI;
    balance_val = Balance_Kp * Bias + Gyro * Balance_Kd;
    return balance_val;
}

int velocity(int encoder_left, int encoder_right)
{
    static float Velocity, Encoder_Least, Encoder, Movement;
    static float Encoder_Integral, Target_Velocity;

    if (Bi_zhang == 1 && Flag_sudu == 1) Target_Velocity = 55;
    else Target_Velocity = 110;
    if (1 == Flag_Qian) Movement = Target_Velocity / Flag_sudu;
    else if (1 == Flag_Hou) Movement = -Target_Velocity / Flag_sudu;
    else Movement = 0;

    Encoder_Least = (encoder_left + encoder_right) - 0;
    Encoder *= 0.8;
    Encoder += Encoder_Least * 0.2;
    Encoder_Integral += Encoder;
    Encoder_Integral = Encoder_Integral - Movement;
    if (Encoder_Integral > 10000) Encoder_Integral = 10000;
    if (Encoder_Integral < -10000) Encoder_Integral = -10000;
    Velocity = Encoder * Velocity_Kp + Encoder_Integral * Velocity_Ki;
    if (Turn_Off(Angle_Balance, Voltage) == 1 || Flag_Stop == 1) Encoder_Integral = 0;
    return Velocity;
}

int turn(int encoder_left, int encoder_right, float gyro)
{
    static float Turn_Target, Turn, Encoder_temp, Turn_Convert = 0.9, Turn_Count;
    float Turn_Amplitude = 88 / Flag_sudu, Kp = 42, Kd = 0;

    if (balance_state.mode == MODE_VISION)
    {
        Turn_Target = turn_pid.tar;
        Kp = fabs(turn_pid.kp);
    }
    else if (balance_state.mode == MODE_TRACE)
    {
        Turn_Convert = 1.2;
        Kp = 30;
        if (1 == turn_dir)
            Turn_Target += Turn_Convert;
        else if (2 == turn_dir)
            Turn_Target -= Turn_Convert;
        else
            Turn_Target = 0;
    }
    else
    {
        if (1 == Flag_Left || 1 == Flag_Right)
        {
            if (++Turn_Count == 1)
                Encoder_temp = myabs(encoder_left + encoder_right);
            Turn_Convert = 50 / Encoder_temp;
            if (Turn_Convert < 0.6) Turn_Convert = 0.6;
            if (Turn_Convert > 3) Turn_Convert = 3;
        }
        else
        {
            Turn_Convert = 0.9;
            Turn_Count = 0;
            Encoder_temp = 0;
        }
        if (1 == Flag_Left) Turn_Target -= Turn_Convert;
        else if (1 == Flag_Right) Turn_Target += Turn_Convert;
        else Turn_Target = 0;
    }

    if (Turn_Target > Turn_Amplitude) Turn_Target = Turn_Amplitude;
    if (Turn_Target < -Turn_Amplitude) Turn_Target = -Turn_Amplitude;
    if (Flag_Qian == 1 || Flag_Hou == 1) Kd = 0.5;
    else Kd = 0;
    Turn = -Turn_Target * Kp - gyro * Kd;
    return Turn;
}

void Set_Pwm(int moto1, int moto2)
{
    if (moto1 > 0) AIN2 = 0, AIN1 = 1;
    else AIN2 = 1, AIN1 = 0;
    PWMA = myabs(moto1);
    if (moto2 > 0) BIN1 = 0, BIN2 = 1;
    else BIN1 = 1, BIN2 = 0;
    PWMB = myabs(moto2);
}

void Xianfu_Pwm(void)
{
    int Amplitude = 6900;
    if (Moto1 < -Amplitude) Moto1 = -Amplitude;
    if (Moto1 > Amplitude) Moto1 = Amplitude;
    if (Moto2 < -Amplitude) Moto2 = -Amplitude;
    if (Moto2 > Amplitude) Moto2 = Amplitude;
}

u8 Turn_Off(float angle, int voltage)
{
    u8 temp;
    if (angle < -40 || angle > 40 || 1 == Flag_Stop || voltage < 1110)
    {
        temp = 1;
        AIN1 = 0; AIN2 = 0;
        BIN1 = 0; BIN2 = 0;
    }
    else
        temp = 0;
    return temp;
}

void Key(void)
{
    u8 tmp, tmp2;
    tmp = click_N_Double(50);
    if (tmp == 1) Flag_Stop = !Flag_Stop;
    tmp2 = Long_Press();
    if (tmp2 == 1)
    {
        balance_state.mode++;
        balance_state.mode %= 6;
        SetBeepMode(BEEP_SYSTEM, BEEP_ON);
    }
}

void Get_Angle(u8 way)
{
    float Accel_Y, Accel_Angle, Accel_Z, Gyro_X, Gyro_Z;
    Temperature = Read_Temperature();

    if (way == 1)
    {
        Read_DMP();
        Angle_Balance = -Roll;
        Gyro_Balance = -gyro[0];
        Gyro_Turn = gyro[2];
        Acceleration_Z = accel[2];
    }
    else
    {
        Gyro_X = (I2C_ReadOneByte(devAddr, MPU6050_RA_GYRO_XOUT_H) << 8) + I2C_ReadOneByte(devAddr, MPU6050_RA_GYRO_XOUT_L);
        Gyro_Z = (I2C_ReadOneByte(devAddr, MPU6050_RA_GYRO_ZOUT_H) << 8) + I2C_ReadOneByte(devAddr, MPU6050_RA_GYRO_ZOUT_L);
        Accel_Y = (I2C_ReadOneByte(devAddr, MPU6050_RA_ACCEL_YOUT_H) << 8) + I2C_ReadOneByte(devAddr, MPU6050_RA_ACCEL_YOUT_L);
        Accel_Z = (I2C_ReadOneByte(devAddr, MPU6050_RA_ACCEL_ZOUT_H) << 8) + I2C_ReadOneByte(devAddr, MPU6050_RA_ACCEL_ZOUT_L);
        if (Gyro_X > 32768) Gyro_X -= 65536;
        if (Gyro_Z > 32768) Gyro_Z -= 65536;
        if (Accel_Y > 32768) Accel_Y -= 65536;
        if (Accel_Z > 32768) Accel_Z -= 65536;
        Gyro_Balance = Gyro_X;
        Accel_Angle = atan2(Accel_Y, Accel_Z) * 180 / PI;
        Gyro_X = Gyro_X / 16.4;

        if (way == 2) Kalman_Filter(Accel_Angle, Gyro_X);
        else if (way == 3) Yijielvbo(Accel_Angle, Gyro_X);
        Angle_Balance = angle;
        Gyro_Turn = Gyro_Z;
        Acceleration_Z = Accel_Z;
    }
}

int myabs(int a)
{
    int temp;
    if (a < 0) temp = -a;
    else temp = a;
    return temp;
}

int Pick_Up(float Acceleration, float Angle, int encoder_left, int encoder_right)
{
    static u16 flag, count0, count1, count2;

    if (flag == 0)
    {
        if (myabs(encoder_left) + myabs(encoder_right) < 30)
            count0++;
        else
            count0 = 0;
        if (count0 > 10) flag = 1, count0 = 0;
    }
    if (flag == 1)
    {
        if (++count1 > 200) count1 = 0, flag = 0;
        if (Acceleration > 26000 && (Angle > (-20 + ZHONGZHI)) && (Angle < (20 + ZHONGZHI)))
            flag = 2;
    }
    if (flag == 2)
    {
        if (++count2 > 100) count2 = 0, flag = 0;
        if (myabs(encoder_left + encoder_right) > 135)
        {
            flag = 0;
            return 1;
        }
    }
    return 0;
}

int Put_Down(float Angle, int encoder_left, int encoder_right)
{
    static u16 flag, count;
    if (Flag_Stop == 0) return 0;
    if (flag == 0)
    {
        if (Angle > (-10 + ZHONGZHI) && Angle < (10 + ZHONGZHI) && encoder_left == 0 && encoder_right == 0)
            flag = 1;
    }
    if (flag == 1)
    {
        if (++count > 50)
        {
            count = 0; flag = 0;
        }
        if (encoder_left < -3 && encoder_right < -3 && encoder_left > -60 && encoder_right > -60)
        {
            flag = 0;
            return 1;
        }
    }
    return 0;
}
