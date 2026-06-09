#ifndef __FILTER_H
#define __FILTER_H
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
extern float angle, angle_dot; 	
void Kalman_Filter(float Accel,float Gyro);		
void Yijielvbo(float angle_m, float gyro_m);
#endif
