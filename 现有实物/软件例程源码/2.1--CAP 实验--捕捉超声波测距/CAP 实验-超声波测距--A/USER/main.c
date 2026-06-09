//实验名称：	CAP捕捉实验
//实验内容：	本实验通过CAP捕捉模块，捕捉超声波信号，实现超声波测距功能。
//									主程序初始化指示灯、OLED、通用定时器2、超声波捕捉模块。通用定时器中断产生时间标志
//									供主循环闪亮指示灯；捕捉中断测量超声波信号并转换成距离。
//									主循环闪亮LED并周期性更新超声波测量的距离显示在OLED显示器上。

#include "stm32f10x.h"
#include "sys.h"
  /**************************************************************************
作者：中科深谷
http://www.si-valley.cn/
**************************************************************************/
u32 Distance,Distance_avr;                        //超声波测距
u32 	Distance_Temp,Distance_Count=0,Distance_All =0;

#define		TIME_500MS_SIZE											50
#define		TIME_50MS_SIZE												5

u8		TIME_50MS_FLAG;
u8		TIME_500MS_FLAG;

u16	time_50ms_ram	=	0;
u16	time_500ms_ram	=	0;

void TIM2_Int_Init(u16 arr,u16 psc);						//被调用函数申明

//++++++++++++++++++++++
int main(void)
{ 
			delay_init();	    	            												//=====延时函数初始化	
	
			JTAG_Set(JTAG_SWD_DISABLE);     	//=====关闭JTAG接口
			JTAG_Set(SWD_ENABLE);           				//=====打开SWD接口 可以利用主板的SWD接口调试
	
			LED_Init();                     												//=====初始化与 LED 连接的硬件接口
			MY_NVIC_PriorityGroupConfig(2);					//=====设置中断分组
	
			OLED_Init();                    												//=====OLED初始化	    

			TIM2_Int_Init(99,7199);			 //10Khz的计数频率,计数到100为10ms  72000/(7199+1)=10(KHZ)
	
			TIM3_Cap_Init(0XFFFF,72-1);	    					//=====超声波初始化
	
			delay_ms(1000);
			OLED_ShowString(30,10,"Si Valley");
			OLED_Refresh_Gram();												//刷新
	
			while(1)
			{
				
						if(TIME_500MS_FLAG == 1)
						{
									TIME_500MS_FLAG = 0;
									LED	=	!LED;
						
									//========第六行显示距离=======================//
									OLED_ShowNumber(70-40,50,(u16)Distance_avr,5,12);		//Distance
									OLED_ShowString(105-40,50,"mm");
									
									OLED_Refresh_Gram();									//刷新	
						}
			}
}  


void TIM2_Int_Init(u16 arr,u16 psc)
{
   TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	 NVIC_InitTypeDef NVIC_InitStructure;

	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //时钟使能
	
	//定时器TIM2初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); //使能指定的TIM2中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级2级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM2, ENABLE);  //使能TIMx					 
}
//定时器2中断服务程序   10ms/times
void TIM2_IRQHandler(void)   //TIM2中断			
{
		if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  //检查TIM2更新中断发生与否
		{
					TIM_ClearITPendingBit(TIM2, TIM_IT_Update  );  //清除TIMx更新中断标志 
			
					time_50ms_ram++;
					if(time_50ms_ram >= TIME_50MS_SIZE)
					{
									time_50ms_ram = 0;
						
									Read_Distane();																//===获取超声波测量距离值
									
									Distance_Temp = Distance;
									Distance_Count++;                            //=====平均值计数器
									Distance_All+=Distance_Temp;          //=====多次采样累积
									if(Distance_Count >=10) 		
									{	
												Distance_avr =Distance_All/10;
												Distance_All=0,Distance_Count=0;	//=====求平均值	
									}
					}														

					time_500ms_ram++;
					if(time_500ms_ram >= TIME_500MS_SIZE)
					{
								time_500ms_ram = 0;
						
								TIME_500MS_FLAG	=	1;
					}														
		}
}
