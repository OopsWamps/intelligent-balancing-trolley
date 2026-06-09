#ifndef __USRAT2_H
#define __USRAT2_H 
#include "sys.h"	  	
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
extern u8 Usart2_Receive;
void uart2_init(u32 bound);
void USART2_IRQHandler(void);
#endif

