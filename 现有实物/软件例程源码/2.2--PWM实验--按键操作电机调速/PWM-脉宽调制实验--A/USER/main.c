//实验名称：	电机调速实验
//实验内容：	本实验提供电机运行参数，实现电机运行。
//									主程序初始化指示灯、按键、通用时间定时器、电机驱动模块。
//									主循环中执行按键修改电机运行状态、指示灯闪亮，并限制快速切换电机运行状态。

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

u8	WAIT_FLAG = 0;

extern   u8		TIME_10MS_FLAG;	
extern   u8		TIME_20MS_FLAG;
extern   u8		TIME_50MS_FLAG;	
extern   u8		TIME_500MS_FLAG;
extern   u8		TIME_2000MS_FLAG;
extern	u16	time_2000ms_ram;

void 		Set_Pwm(int moto1,int moto2);
void 		Xianfu_Pwm(void);
int 			myabs(int a);
	
int main(void)
{ 
			delay_init();	    	            												//=====延时函数初始化	
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			KEY_Init();                     												//=====按键初始化

			TIM2_Int_Init(99,7199);			 //10Khz的计数频率,计数到100为10ms  72000/(7199+1)=10(KHZ)
	
			MiniBalance_PWM_Init(7199,0);   				//=====初始化PWM 72MHZ，不分频，周期0.1ms，用于驱动电机 
	
			while(1)
			{
								if(TIME_10MS_FLAG == 1)
								{
											if((click() == 1)&&(WAIT_FLAG ==0))   //===扫描按键状态 按键一次可以改变电机运行状态一次
											{
														WAIT_FLAG =1;
														Flag_Stop++;	 							//按键被按下改变一次电机运行状态，在两秒内只能改变一次。
														if(Flag_Stop >3)			Flag_Stop = 1;			//1--停车，2--慢速，3--快速。

														time_2000ms_ram = 0;						
														TIME_2000MS_FLAG	=	0;
											}											
											if(Flag_Stop == 2)
											{
														Moto1 = -1000;                 //===左轮电机最终PWM
														Moto2 = 1000;                 //===右轮电机最终PWM
											}
											else if(Flag_Stop == 3)
											{
														Moto1 = -3000;                 //===左轮电机最终PWM
														Moto2 = 3000;                 //===右轮电机最终PWM
											}
											else
											{
														Moto1 = 0;                        //===左轮电机最终PWM
														Moto2 = 0;                        //===右轮电机最终PWM
											}											
											Xianfu_Pwm();                        //===PWM限幅

											Set_Pwm(Moto1,Moto2);        //===赋值给PWM寄存器  
								}
				
								if(TIME_500MS_FLAG == 1)
								{
											TIME_500MS_FLAG  = 0;
									
											LED	=	!LED;													//LED 1秒周期闪亮
								}
								
								if(TIME_2000MS_FLAG == 1)
								{
											TIME_2000MS_FLAG = 0;
											
											if(WAIT_FLAG == 1)
											{
														WAIT_FLAG = 0;								//按键后延迟两秒按键再次有效，限制快速切换。
											}
								}
								
			}
}  

/**************************************************************************
函数功能：赋值给PWM寄存器
入口参数：左轮PWM、右轮PWM
返回  值：无
**************************************************************************/
void Set_Pwm(int moto1,int moto2)
{
    	if(moto1>0)			AIN2=0,			AIN1=1;
			else 	          AIN2=1,			AIN1=0;
			PWMA=myabs(moto1);
		  if(moto2>0)	BIN1=0,			BIN2=1;
			else        BIN1=1,			BIN2=0;
			PWMB=myabs(moto2);	
}
/**************************************************************************
函数功能：限制PWM赋值 
入口参数：无
返回  值：无
**************************************************************************/
void Xianfu_Pwm(void)
{	
	  int Amplitude		=	6900;    						//===PWM满幅是7200 限制在6900
//		if(Flag_Qian==1)  Moto1+=DIFFERENCE;  //DIFFERENCE是一个衡量平衡小车电机和机械安装差异的一个变量。直接作用于输出，让小车具有更好的一致性。
//	  if(Flag_Hou==1)   Moto2-=DIFFERENCE;
    if(Moto1<-Amplitude) 		Moto1	=	-Amplitude;	
		if(Moto1>Amplitude)  		Moto1	=	Amplitude;	
	  if(Moto2<-Amplitude) 		Moto2	=	-Amplitude;	
		if(Moto2>Amplitude)  		Moto2	=	Amplitude;		
}

/**************************************************************************
函数功能：绝对值函数
入口参数：int
返回  值：unsigned int
**************************************************************************/
int myabs(int a)
{ 		   
	  int temp;
	
		if(a<0)  temp	=	-a;  
	  else 			temp		=	a;
	  return 	temp;
}

