//实验名称：	编码器实验--速度信号采集
//实验内容：	本实验通过单位时间读取编码器计数值，实现采集电机速度的功能，并在OLED上显示左右电机速度。
//									主程序初始化指示灯、OLED、电机1编码器、电机2编码器。
//									主循环中周期性调用读取编码器函数，并在OLED显示编码器计数值。
//操作方法：一手拿起小车，另一手分别转动两只车轮。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
u8 Way_Angle=2;                             					//获取角度的算法，1：四元数  2：卡尔曼  3：互补滤波 

u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu=2; 	//蓝牙遥控相关的变量
u8 Flag_Stop=1,Flag_Show=0;                 	//停止标志位和 显示标志位 默认停止 显示打开
int Encoder_Left,Encoder_Right;             	//左右编码器的脉冲计数
int Moto1,Moto2;                            						//电机PWM变量 应是Motor
int Temperature;                            							//显示温度
int Voltage;                                									//电池电压采样相关的变量
float Angle_Balance,Gyro_Balance,Gyro_Turn; //平衡倾角 平衡陀螺仪 转向陀螺仪
float Show_Data_Mb;                         				//全局显示变量，用于显示需要查看的数据
u32 Distance;                               							//超声波测距
u8 delay_50,delay_flag,Bi_zhang=0,PID_Send,Flash_Send; 		//延时和调参等变量
float Acceleration_Z;                       						//Z轴加速度计  
float Balance_Kp=300,Balance_Kd=1,Velocity_Kp=80,Velocity_Ki=0.4;	//PID参数
u16 PID_Parameter[10],Flash_Parameter[10];  	//Flash相关数组

int main(void)
{ 
			u8 T_count1= 0,T_count2=0;
				
			delay_init();	    	            												//=====延时函数初始化	
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口

//ST-LINK V2 仿真器只支持SWD接口，仿真连接后不能在线切换	
//OLED 与JTAG接口公用PB3\PB4\PA15，OLED的使用需要关闭JTAG接口，释放出公用GPIO。
//即：仿真时注销下面两条指令，OLED不工作；只是下载时，打开下面两条指令，OLED正常工作
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试

			OLED_Init();                    												//=====OLED初始化	    
	
			Encoder_Init_TIM2();            									//=====编码器接口
			Encoder_Init_TIM4();            									//=====初始化编码器

			delay_ms(1000);
			OLED_ShowString(30,10,"Si Valley");
			OLED_Refresh_Gram();												//刷新
	
			while(1)
			{
						delay_ms(5);
						
						if(T_count1++ >10)				//50ms 参数显示刷新一次
						{
									T_count1= 0;
						
									Encoder_Left=Read_Encoder(4);                            //===读取编码器的值
									Encoder_Right=Read_Encoder(2);                          //===读取编码器的值
							
									//=============第五行显示编码器1=======================//	
																				OLED_ShowString(00,40,"EncoLEFT");
									if( Encoder_Left<0)		OLED_ShowString(80,40,"-"),
																				OLED_ShowNumber(95,40,-Encoder_Left,3,12);
									else                 	OLED_ShowString(80,40,"+"),
																				OLED_ShowNumber(95,40, Encoder_Left,3,12);
									//=============第六行显示编码器2=======================//		
																				OLED_ShowString(00,50,"EncoRIGHT");
									if(Encoder_Right<0)		  OLED_ShowString(80,50,"-"),
																				OLED_ShowNumber(95,50,-Encoder_Right,3,12);
									else               		OLED_ShowString(80,50,"+"),
																				OLED_ShowNumber(95,50,Encoder_Right,3,12);	
		
									OLED_Refresh_Gram();									//刷新	
						}
						
						if(T_count2++ >100)				//500ms LED翻转一次
						{
									T_count2= 0;
								
									LED = !LED;
						}
			}
}  
