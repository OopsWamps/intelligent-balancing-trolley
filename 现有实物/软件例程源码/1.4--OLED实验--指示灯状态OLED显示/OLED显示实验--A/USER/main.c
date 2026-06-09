//实验名称：	OLED显示指示灯状态
//实验内容：	本实验重点是OLED显示屏的使用.主程序初始化指示灯、按键和OLED，
//							  主循环中周期性延时测试按键，触发取反指示灯，并将指示灯状态显示在OLED上。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
int main(void)
{ 
			delay_init();	    	            												//=====延时函数初始化	
	
//ST-LINK V2 仿真器只支持SWD接口，仿真连接后不能在线切换	
//OLED 与JTAG接口公用PB3\PB4\PA15，OLED的使用需要关闭JTAG接口，释放出公用GPIO。
//即：仿真时注销下面两条指令，OLED不工作；只是下载时，打开下面两条指令，OLED正常工作
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			KEY_Init();                     												//=====按键初始化
	
			OLED_Init();                    												//=====OLED初始化	    

			OLED_ShowString(30,10,"Si Valley");		//显示公司LOGO
			OLED_Refresh_Gram();												//刷新
	
			while(1)
			{
						delay_ms(100);
						if(click() == 1)                                   //===扫描按键状态 按键一次可以改变LED指示灯状态一次
						{
									LED	=	!LED;															//指示灯状态反转
						}
						
						if(LED == 0)		OLED_ShowString(35,40,"LED on ");			//显示灯亮
						else 								OLED_ShowString(35,40,"LED off");			//显示灯灭
						OLED_Refresh_Gram();									//刷新						
			}
}  
