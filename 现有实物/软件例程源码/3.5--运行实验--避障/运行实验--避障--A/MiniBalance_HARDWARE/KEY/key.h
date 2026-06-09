#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
#define KEY PAin(5)
void KEY_Init(void);          				//按键初始化
u8 click_N_Double (u8 time);  	//单击按键扫描和双击按键扫描
u8 click(void);               						//单击按键扫描
u8 Long_Press(void);           		//长按扫描  
#endif  
