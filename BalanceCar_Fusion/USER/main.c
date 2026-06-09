//实验名称：	运行实验--避障
//实验内容：	本实验接收手机蓝牙APP运行控制：前进、后退、左转、右转，叠加超声波测距避障功能。
//									主程序初始化指示灯、OLED、电机1编码器、电机2编码器、IIC、MPU6050、PWM、DMP、外部中断、UART3。
//									主循环中周期性显示小车运行模式和参数。
//									定时外部中断中实现编码器读取、IIC读取陀螺仪参数、电机运行参数的处理、LED闪烁控制。
//									串口3中断接收APP控制命令，修改电机运行标志，供定时外部中断处理电机运行使用。
//									定时器3捕获功能实现超声波捕获测距，定时处理用来小车运行中避障。
//操作方法：1，上电后使用“USER”按键开启和关闭小车平衡运行；
//									2，打开手机APP连接蓝牙，在APP上操作控制小车运行；
//									3，长按USER按键2秒以上，在“普通”和“避障”模式之间切换一次；
//									4，避障模式前进时，遇到障碍向右转弯；静止时遇到障碍，向后运行至超出障碍范围。

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
int Voltage =1110 + 110;                            //电池电压采样相关的变量
float Angle_Balance,Gyro_Balance,Gyro_Turn; //平衡倾角 平衡陀螺仪 转向陀螺仪
float Show_Data_Mb;                         				//全局显示变量，用于显示需要查看的数据
u32 Distance;                               							//超声波测距
u8 delay_50,delay_flag,Bi_zhang=0,PID_Send,Flash_Send; 		//延时和调参等变量
float Acceleration_Z;                       						//Z轴加速度计  
float Balance_Kp=300,Balance_Kd=1,Velocity_Kp=80,Velocity_Ki=0.4;	//PID参数
u16 PID_Parameter[10],Flash_Parameter[10];  	//Flash相关数组

u8	Uart_Ctrl_Flag = 0;
u8	UartCount = 0;

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
			MY_NVIC_PriorityGroupConfig(2);					//=====设置中断分组
			MiniBalance_PWM_Init(7199,0);   				//=====初始化PWM 10KHZ，用于驱动电机 如需初始化电调接口 

			uart3_init(9600);               											//=====串口3初始化  ---蓝牙
	
			OLED_Init();                    												//=====OLED初始化	    
	
			Encoder_Init_TIM2();            									//=====编码器接口
			Encoder_Init_TIM4();            									//=====初始化编码器
	
			IIC_Init();                     														//=====IIC初始化
			MPU6050_initialize();           									//=====MPU6050初始化	
			DMP_Init();                     												//=====初始化DMP 
	
			MiniBalance_EXTI_Init();        								//=====MPU6050 5ms定时中断初始化
			
			TIM3_Cap_Init(0XFFFF,72-1);	    					//=====超声波初始化
		
			while(1)
			{
						delay_flag=1;	
						delay_50=0;
						while(delay_flag);	     		//通过MPU6050的INT中断实现的50ms精准延时		
		
						Read_Distane();                              //===获取超声波测量距离值
				
						if(UartCount++ > 5)		UartCount = 0,	Uart_Ctrl_Flag = 0;							//超过 5*50ms 没有收到控制信号，清除控制标志。
						if(Bi_zhang==1&&Distance>BIZHANG_DIST && Uart_Ctrl_Flag==0&&Flag_Hou == 1)		Flag_Hou = 0;			//避免静止有障碍时一直向后。	
				
				
								//=============第一行显示LOGO=======================//	
																			OLED_ShowString(30,0,"Si Valley");
								//=============第二行显示温度和距离===============//	
																			OLED_ShowNumber(0,10,Temperature/10,2,12);
																			OLED_ShowNumber(23,10,Temperature%10,1,12);
																			OLED_ShowString(13,10,".");
																			OLED_ShowString(35,10,"`C");
																			OLED_ShowNumber(70,10,(u16)Distance,5,12);
																			OLED_ShowString(105,10,"mm");
								//=============第三行显示编码器1=======================//	
																			OLED_ShowString(00,20,"EncoLEFT");
								if( Encoder_Left<0)		OLED_ShowString(80,20,"-"),
																			OLED_ShowNumber(95,20,-Encoder_Left,3,12);
								else                 	OLED_ShowString(80,20,"+"),
																			OLED_ShowNumber(95,20, Encoder_Left,3,12);
								//=============第四行显示编码器2=======================//		
																			OLED_ShowString(00,30,"EncoRIGHT");
								if(Encoder_Right<0)		  OLED_ShowString(80,30,"-"),
																			OLED_ShowNumber(95,30,-Encoder_Right,3,12);
								else               		OLED_ShowString(80,30,"+"),
																			OLED_ShowNumber(95,30,Encoder_Right,3,12);	
								//=============第五行显示模式=======================//
																		  if(Bi_zhang==1)	OLED_ShowString(40,40,"Bizhang");
																	 	  else             OLED_ShowString(40,40,"Putong ");								
								//=============第六行显示角度=======================//
		                      OLED_ShowString(0,50,"Angle");
								if(Angle_Balance<0)		
														OLED_ShowString(45,50,"-"),	
														OLED_ShowNumber(45+10,50,-Angle_Balance,3,12);
								else					        
														OLED_ShowString(45,50,"+"),	
														OLED_ShowNumber(45+10,50,Angle_Balance,3,12);
								//=============刷新=======================//
								OLED_Refresh_Gram();	
			}
}  

