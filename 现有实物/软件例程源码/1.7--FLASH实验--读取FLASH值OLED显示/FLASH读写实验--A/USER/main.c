//实验名称：	FLASH读写实验
//实验内容：	本实验实现读取FLASH和写入FLASH参数，并在OLED上实时显示参数值。
//									主程序初始化指示灯、按键、OLED，并从FLASH中读取参数值显示，主循环中执行按键修改参数功能，
//									并立即存储，随即读出，OLED显示参数实时数值。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
//int Voltage;                                										//电源电压采样相关的变量
float Balance_Kp=300,Balance_Kd=1,Velocity_Kp=80,Velocity_Ki=0.4;	//PID参数
u16 PID_Parameter[10],Flash_Parameter[10];  	//Flash相关数组

int main(void)
{ 
			delay_init();	    	            												//=====延时函数初始化	
	
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			KEY_Init();                     												//=====按键初始化
	
			OLED_Init();                    												//=====OLED初始化	  
  
			Flash_Write();						//每一次上电或者复位，重新将初始值写入FLASH中，避免连续操作参数一直累加下去。
			delay_ms(100);

			Flash_Read();      			//=====读取Flash的PID参数		
	
			delay_ms(500);
			OLED_ShowString(30,10,"Si Valley");
			
			OLED_ShowString(00,40,"Balance_Kp");
			OLED_ShowNumber(80,40,(u16)Balance_Kp,5,12);
						
			OLED_ShowString(00,50,"Balance_Kd");
			OLED_ShowNumber(80,50,(u16)Balance_Kd,5,12);
			
			OLED_Refresh_Gram();										 //刷新
				
			while(1)
			{
						delay_ms(100);
				
						if(click() == 1)                                //===扫描按键状态 按键一次可以改变LED指示灯状态一次
						{
									LED	=	!LED;
							
									Balance_Kp  += 10;									//修改参数值，并写入FLASH中，直观感受参数存取值的变化。
									Balance_Kd  += 20;									//
							
									Flash_Write();
						}

						Flash_Read(); 																//读取FLASH中最新值供修改、显示。
										
						OLED_ShowNumber(80,40,(u16)Balance_Kp,5,12);									
						OLED_ShowNumber(80,50,(u16)Balance_Kd,5,12);						
						OLED_Refresh_Gram();								//刷新						
			}
}  
