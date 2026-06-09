#include "control.h"
#include "filter.h"

int Balance_Pwm, Velocity_Pwm, Turn_Pwm;
u8 Flag_Target;
u32 Flash_R_Count;
int Voltage_Temp, Voltage_Count, Voltage_All;

PIDParam_t upright_pid;
PIDParam_t speed_pid;
PIDParam_t turn_pid;
PIDParam_t dist_pid;
PID_t dist;
BalanceState_t balance_state;

void InitPIDParams(void)
{
    upright_pid.kp = 300; upright_pid.kd = 1; upright_pid.out = 0; upright_pid.tar = 0;
    speed_pid.kp = 80; speed_pid.ki = 0.4; speed_pid.out = 0; speed_pid.filter = 0.8; speed_pid.tar = 0;
    turn_pid.kd = 0; turn_pid.kp = 42; turn_pid.out = 0; turn_pid.tar = 0;
    dist_pid.kp = -0.35; dist_pid.ki = -0.35f/200.0f; dist_pid.out = 0; dist_pid.tar = 40;
    balance_state.lifted_flag = 0;
    balance_state.putdown_counter = 0;
    balance_state.lifted_counter = 0;
    balance_state.balance_enable = 1;
    balance_state.mode = 0;
}

float pwm_out_val;
static uint8_t obstacle_blocked = 0;

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


