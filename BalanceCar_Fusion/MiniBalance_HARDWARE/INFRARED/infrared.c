#include "infrared.h"

u16 turn_dir = 0;

void Init_Infrared_Gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void tracking(void)
{
    /* black line tracing */
    if (RIN1 == 0 && RIN2 == 0 && RIN3 == 0 && RIN4 == 0)
        turn_dir = 0;
    else if (RIN1 == 1 && RIN3 == 0 && RIN4 == 0)
        turn_dir = 1;   /* turn right */
    else if (RIN4 == 1 && RIN1 == 0 && RIN2 == 0)
        turn_dir = 2;   /* turn left */
    else if ((RIN2 == 0 || RIN3 == 0) && RIN1 == 1 && RIN4 == 1)
        turn_dir = 3;   /* go straight */
    else if ((RIN2 == 1 || RIN3 == 1) && RIN1 == 0 && RIN4 == 0)
        turn_dir = 3;
    else
        turn_dir = 0;
}
