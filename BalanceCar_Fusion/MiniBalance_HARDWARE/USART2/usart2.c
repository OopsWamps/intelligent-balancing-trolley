#include "usart2.h"

u8 Usart2_Receive;
PiVision_t pi_vision;

void uart2_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

void uart2_send_cmd(u8 cmd)
{
    USART_SendData(USART2, PI_CMD_FRAME_HEADER);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    USART_SendData(USART2, cmd);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        static uint8_t frame_buf[6];
        static uint8_t frame_idx = 0;
        static uint8_t frame_state = 0;   /* 0=wait header, 1-5=data */
        uint8_t byte, checksum, i;
        int uart_receive;

        uart_receive = USART_ReceiveData(USART2);
        Usart2_Receive = uart_receive;
        byte = (uint8_t)uart_receive;

        if (frame_state == 0)
        {
            if (byte == 0xA5)
            {
                frame_buf[0] = byte;
                frame_idx = 1;
                frame_state = 1;
            }
        }
        else
        {
            frame_buf[frame_idx++] = byte;
            if (frame_idx >= 6)
            {
                /* verify checksum */
                checksum = 0;
                for (i = 0; i < 5; i++)
                    checksum ^= frame_buf[i];
                if (checksum == frame_buf[5])
                {
                    pi_vision.x_offset   = (int8_t)frame_buf[1];
                    pi_vision.y_offset   = (int8_t)frame_buf[2];
                    pi_vision.distance   = frame_buf[3];
                    pi_vision.confidence = frame_buf[4];
                    pi_vision.fresh      = 1;
                }
                frame_state = 0;
                frame_idx = 0;
            }
        }
    }
}
