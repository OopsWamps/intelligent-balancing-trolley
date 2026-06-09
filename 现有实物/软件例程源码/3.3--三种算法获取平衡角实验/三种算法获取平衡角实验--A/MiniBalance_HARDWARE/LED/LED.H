#ifndef __LED_H
#define __LED_H
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
//LED 端口定义
#define LED PAout(4)  
void LED_Init(void);  //初始化
void Led_Flash(u16 time);
#endif
