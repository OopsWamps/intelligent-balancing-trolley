#ifndef __INFRARED_H
#define __INFRARED_H	 
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
extern 	u16 	turn_dir;
extern void Init_Infrared_Gpio(void);
extern void tracking(void);

#define 	RIN1 	PAin(7) 
#define 	RIN2 	PCin(15) 
#define 	RIN3 	PCin(14) 
#define 	RIN4 	PCin(13) 

//extern float Angle_Balance,Gyro_Balance,Gyro_Turn;

#endif  
