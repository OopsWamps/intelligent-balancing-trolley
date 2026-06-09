#ifndef __CONTROL_H
#define __CONTROL_H
#include "sys.h"
#include "pid.h"

#define PI 3.14159265
#define ZHONGZHI 0
#define DIFFERENCE 100
#define MAXPWM 6900

typedef struct {
    float kp, kd, ki;
    float tar, current;
    float filter;
    float out;
} PIDParam_t;

/* mode definitions */
#define MODE_BALANCE   0
#define MODE_BT_AVOID  1
#define MODE_FOLLOW    2
#define MODE_TRACE     3
#define MODE_BT_ONLY   4
#define MODE_VISION    5

/* state management */
typedef struct {
    uint8_t lifted_flag;
    uint16_t putdown_counter;
    uint16_t lifted_counter;
    uint8_t balance_enable;
    uint8_t mode;
} BalanceState_t;

extern BalanceState_t balance_state;

extern int Balance_Pwm, Velocity_Pwm, Turn_Pwm;
extern PIDParam_t upright_pid;
extern PIDParam_t speed_pid;
extern PIDParam_t turn_pid;
extern PIDParam_t dist_pid;
extern PID_t dist;

extern u8 Flag_Stop;

int EXTI15_10_IRQHandler(void);
int balance(float angle, float gyro);
int velocity(int encoder_left, int encoder_right);
int turn(int encoder_left, int encoder_right, float gyro);
void Set_Pwm(int moto1, int moto2);
void Key(void);
void Xianfu_Pwm(void);
u8 Turn_Off(float angle, int voltage);
void Get_Angle(u8 way);
int myabs(int a);
int Pick_Up(float Acceleration, float Angle, int encoder_left, int encoder_right);
int Put_Down(float Angle, int encoder_left, int encoder_right);

void ModeSelect(void);
void Balance(void);
void CheckLiftState(void);
void DetectPutDown(void);
void CheckFallDown(void);
void ObstacleAvoid(void);

void DistPidCtrl(void);
void PWMLimit(float PWMA, float PWMB);
void DataClear(void);

#endif
