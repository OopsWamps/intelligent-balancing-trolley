#include "infrared.h"

u16 turn_dir=0;
void Init_Infrared_Gpio(void)
{
	//IN1,IN2,IN3,IN4:PA7,PC15,PC14,PC13
	
  GPIO_InitTypeDef 		GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 		//使能PA端口时钟
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;	           								 					//端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         									//上拉输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);					      															//根据设定参数初始化GPIOA 

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 		//使能PC端口时钟
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_14|GPIO_Pin_13;	   //端口配置
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;         									//上拉输入
  GPIO_Init(GPIOC, &GPIO_InitStructure);					      														//根据设定参数初始化GPIOC 
}

void tracking(void)
{
//黑线寻迹
	if(RIN1==0&&RIN2==0&&RIN3==0&&RIN4==0)      //未检测到黑线
	turn_dir=0;
	else if(RIN1==1&&RIN3==0&&RIN4==0)//TURN RIGHT   //检测到黑线右偏
	turn_dir=1;
	else if(RIN4==1&&RIN1==0&&RIN2==0)//TURN LEFT    //黑线左偏
	turn_dir=2;

//白线寻迹
//	    if(RIN1==1 && RIN2==1 && RIN3==1 &&RIN4==1)
//	    turn_dir=0;
//	    else if(RIN1==0 && RIN3==1 && RIN4==1)  //TURN RIGHT
//	    turn_dir=1;
//	    else if(RIN4==0 && RIN1==1 && RIN2==1)  //TURN LEFT
//	    turn_dir=2;

//common
	    else if((RIN2==0||RIN3==0)&& RIN1==1 && RIN4==1) 	//代表白线在正中心
	    turn_dir=3;

			else if((RIN2==1||RIN3==1)&&RIN1==0&&RIN4==0)    	//代表黑线在正中心
			turn_dir=3;

	    else    turn_dir = 0;
}
