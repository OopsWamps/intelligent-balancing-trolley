#include "beep.h"

static BEEP_Mode_e beep_state = BEEP_OFF;

void BeepDeviceInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = BEEP_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(BEEP_GPIO_Port, BEEP_Pin);
    beep_state = BEEP_OFF;
}

void SetBeepMode(BEEP_Type_e BeepType, BEEP_Mode_e Mode)
{
    if (BeepType >= 1) return;
    beep_state = Mode;
    if (Mode == BEEP_ON)
        GPIO_ResetBits(BEEP_GPIO_Port, BEEP_Pin);
    else
        GPIO_SetBits(BEEP_GPIO_Port, BEEP_Pin);
}
