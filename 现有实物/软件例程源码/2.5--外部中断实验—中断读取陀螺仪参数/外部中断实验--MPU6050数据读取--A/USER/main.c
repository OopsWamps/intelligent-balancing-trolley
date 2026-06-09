//实验名称：	外部中断实验
//实验内容：	陀螺仪模块产生定时口线输出，主控板响应外部中断，得到精准定时信号，供定时功能使用。
//									主程序初始化指示灯、OLED、IIC、MPU6050、DMP、外部中断。
//									外部中断中读取陀螺仪温度值，并供主程序在OLED上显示温度值。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
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

void 		Set_Pwm(int moto1,int moto2);
void 		Xianfu_Pwm(void);
int 			myabs(int a);

u8   T_count = 0;

int main(void)
{ 
			delay_init();	    	            												//=====延时函数初始化	
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口

//ST-LINK V2 仿真器只支持SWD接口，仿真连接后不能在线切换	
//OLED 与JTAG接口公用PB3\PB4\PA15，OLED的使用需要关闭JTAG接口，释放出公用GPIO。
//即：仿真时注销下面两条指令，OLED不工作；只是下载时，打开下面两条指令，OLED正常工作
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试

			OLED_Init();                    												//=====OLED初始化	    
	
	    IIC_Init();                     														//=====IIC初始化
			MPU6050_initialize();           									//=====MPU6050初始化	
			DMP_Init();                     												//=====初始化DMP 
	
			MiniBalance_EXTI_Init();        								//=====MPU6050 5ms定时触发外部中断初始化
	
			delay_ms(1000);
			OLED_ShowString(30,0,"Si Valley");
			OLED_ShowString(0,30,"Temperature:");
			OLED_Refresh_Gram();												//刷新
		
			while(1)
			{
				
						//====第六行显示温度和距离======//	
						OLED_ShowNumber(0+60,50,Temperature/10,2,12);
						OLED_ShowNumber(23+60,50,Temperature%10,1,12);
						OLED_ShowString(13+60,50,".");
						OLED_ShowString(35+60,50,"`C");
			
						OLED_Refresh_Gram();									//刷新	
			
						delay_flag=1;	
						delay_50=0;
						while(delay_flag);	     		//通过MPU6050的INT中断实现的50ms精准延时		

						T_count++;
						if(T_count >=10)						//500ms翻转一次LED
						{
									T_count = 0;
									LED	=	!LED;
						}
			}
}  

/**************************************************************************
函数功能：所有的控制代码都在这里面
         5ms定时中断由MPU6050的INT引脚触发
         严格保证采样和数据处理的时间同步				 
**************************************************************************/
int EXTI15_10_IRQHandler(void) 
{    
	 if(INT==0)		
	 {   
			EXTI->PR=1<<12;                                                   //清除中断标志位   
		
		  if(delay_flag==1)
			{
				 if(++delay_50 >= 10)	 delay_50=0,delay_flag=0;    //给主函数提供50ms的精准延时
			}

			Temperature=Read_Temperature();      	//IIC 读取MPU6050内置温度传感器数据，近似表示主板温度。											
	 }   
	
	 return 0;	  
} 

