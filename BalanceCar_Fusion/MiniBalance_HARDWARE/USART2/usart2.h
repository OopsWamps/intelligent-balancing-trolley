#ifndef __USRAT2_H
#define __USRAT2_H
#include "sys.h"

extern u8 Usart2_Receive;

/* Pi vision following data frame */
typedef struct {
    int8_t x_offset;      /* -128~127, horizontal offset */
    int8_t y_offset;      /* -128~127, vertical offset */
    uint8_t distance;     /* 0~255 cm */
    uint8_t confidence;   /* 0~100 % */
    uint8_t fresh;        /* 1 = new frame received */
} PiVision_t;

extern PiVision_t pi_vision;

void uart2_init(u32 bound);
void USART2_IRQHandler(void);

#endif
