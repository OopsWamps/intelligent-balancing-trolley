//实验名称：	串口通信实验
//实验内容：	本实验接收串口助手发送的命令码，并将命令码转换成功能名称在OLED显示屏上显示。
//									主程序初始化指示灯、按键、OLED和串口2，主循环中处理接收的功能码，按键反转LED状态，
//									OLED显示命令码功能和LED状态。

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
			delay_init();	    	            												//=====延时函数初始化	
	
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			KEY_Init();                     												//=====按键初始化
	
			OLED_Init();                    												//=====OLED初始化	    

			delay_ms(1000);
			OLED_ShowString(30,10,"Si Valley");
			OLED_Refresh_Gram();												//刷新
	
			uart2_init(9600);               											//=====串口2初始化 ，使用串口助手测试实验 
		
			while(1)
			{
						delay_ms(50);
						if(click() == 1)                                   //===扫描按键状态 按键一次可以改变LED指示灯状态一次
						{
									LED	=	!LED;
						}
						
						if( Flag_Qian == 1)						OLED_ShowString(28-4,50,"Run forward");	
						else if(Flag_Hou == 1)  		OLED_ShowString(28-4,50,"Run behind ");		//增加空格为了显示整齐与切换时有效覆盖
						else if(Flag_Left == 1)  		OLED_ShowString(28-4,50," Turn left   ");			//
						else if(Flag_Right == 1)  	OLED_ShowString(28-4,50,"Turn right  ");			//
						else OLED_ShowString(28-4,50,"   Stop    ");
						OLED_Refresh_Gram();									//刷新	
			}
}  
