//实验名称：	定时改变LED亮灭
//实验内容：	本实验主程序初始化指示灯IO口、定时器2，
//							  主程序中周期性检测外部变量时间标志，时间到，LED指示灯取反一次。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/

extern   u8		TIME_10MS_FLAG;									//申明外部变量
extern   u8		TIME_20MS_FLAG;
extern   u8		TIME_50MS_FLAG;	
extern   u8		TIME_500MS_FLAG;
extern   u8		TIME_2000MS_FLAG;

int main(void)
{ 
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口

			TIM2_Int_Init(99,7199);			 										//10Khz的计数频率,计数到100为10ms  72000/(7199+1)=10(KHZ)
	
			while(1)
			{

								if(TIME_500MS_FLAG == 1)				//定时500ms标志
								{
											TIME_500MS_FLAG  = 0;
									
											LED	=	!LED;
								}
								
//								if(TIME_2000MS_FLAG == 1)
//								{
//											TIME_2000MS_FLAG  = 0;
//									
//											LED	=	!LED;
//								}
								
//								if(TIME_50MS_FLAG == 1)
//								{
//											TIME_50MS_FLAG  = 0;
//									
//											LED	=	!LED;
//								}
																
			}
}  
