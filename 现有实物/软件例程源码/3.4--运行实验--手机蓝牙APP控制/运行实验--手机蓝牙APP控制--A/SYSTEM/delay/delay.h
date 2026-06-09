#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"  
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
	 
void delay_init(void);
void delay_ms(u16 nms);
void delay_us(u32 nus);

void delay_s(u16 ns);
#endif





























