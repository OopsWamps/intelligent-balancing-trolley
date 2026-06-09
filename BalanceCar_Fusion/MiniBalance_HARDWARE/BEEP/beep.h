#ifndef _BEEP_H
#define _BEEP_H

#include "stm32f10x.h"

#define BEEP_GPIO_Port GPIOA
#define BEEP_Pin GPIO_Pin_4

typedef enum
{
    BEEP_OFF = 0,
    BEEP_ON
} BEEP_Mode_e;

typedef enum
{
    BEEP_SYSTEM = 0,
} BEEP_Type_e;

void BeepDeviceInit(void);
void SetBeepMode(BEEP_Type_e BeepType, BEEP_Mode_e Mode);

#endif
