//实验名称：	ADC电压采集
//实验内容：	本实验采集供电电源电压，并将电压值在OLED显示屏上显示。主程序初始化指示灯、按键、OLED和ADC，
//							  主循环中采集ADC值并滤波，按键反转LED状态，OLED显示电源电压值和LED状态。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
int Voltage;                                										//电池电压采样相关的变量

int main(void)
{ 
			int 	Voltage_Temp,Voltage_Count=0,Voltage_All =0;
			u8	T_Count =0;
	
			#define 	Offset   25																//显示电压各参数公用偏移量

			delay_init();	    	            												//=====延时函数初始化	
	
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			KEY_Init();                     												//=====按键初始化
	
			OLED_Init();                    												//=====OLED初始化	  
  
			Adc_Init();                     													//=====adc初始化

			delay_ms(500);
			OLED_ShowString(30,10,"Si Valley");		//显示LOGO
			OLED_Refresh_Gram();												//刷新
	
			Voltage	= Get_battery_volt();								//避免上电时显示1秒零值。
			
			while(1)
			{
						delay_ms(10);
				
						Voltage_Temp=Get_battery_volt();		 //=====读取电源电压		
						Voltage_Count++;                             //=====平均值计数器
						Voltage_All+=Voltage_Temp;             //=====多次采样累积
						if(Voltage_Count >=100) 		Voltage =Voltage_All/100,Voltage_All=0,Voltage_Count=0;	//=====求平均值		
						
						T_Count++;
						if(T_Count >10)		T_Count = 0;				 //100ms处理一次
						{
								if(click() == 1)                                //===扫描按键状态 按键一次可以改变LED指示灯状态一次
								{
											LED	=	!LED;
								}
																
								if(LED == 0)		OLED_ShowString(35,30,"LED on ");				//第30行第35列开始显示“LED on”
								else 								OLED_ShowString(35,30,"LED off");
								
								//========第六行显示电压=======================//
								OLED_ShowString(00+Offset,50,"Volta");
								OLED_ShowString(58+Offset,50,".");
								OLED_ShowString(80+Offset,50,"V");
								OLED_ShowNumber(45+Offset,50,Voltage/100,2,12);
								OLED_ShowNumber(68+Offset,50,Voltage%100,2,12);
								if(Voltage%100<10) 	OLED_ShowNumber(62+Offset,50,0,2,12);
								
								OLED_Refresh_Gram();									//刷新	
						}
			}
}  
