#include "usart2.h"
  /**************************************************************************
作者：Yuan
**************************************************************************/
u8 Usart2_Receive;

/**************************************************************************
函数功能：串口2初始化
入口参数： bound:波特率  9600
返回  值：无
**************************************************************************/
void uart2_init(u32 bound)
{  	 
	  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能UGPIOA时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART2时钟
	//USART2_TX  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);
   
  //USART2_RX	  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PA.3
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  //Usart2 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	
   //USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(USART2, &USART_InitStructure);     //初始化串口2
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启串口接受中断
  USART_Cmd(USART2, ENABLE);                    //使能串口2 
}

/**************************************************************************
函数功能：串口2接收中断：1，语音无线控制
入口参数：无
返回  值：无
**************************************************************************/
void USART2_IRQHandler(void)
{	
		if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收到数据
		{	  
					static	int uart_receive=0;			//无线接收相关变量
			//		static u8 Flag_PID,i,j,Receive[50];
			//		static float Data;
					uart_receive=USART_ReceiveData(USART2); 
					Usart2_Receive=uart_receive;
				
			//		if(uart_receive==0x59)  Flag_sudu=2;  //低速挡（默认值）
			//		if(uart_receive==0x58)  Flag_sudu=1;  //高速档
					
					if(uart_receive>10)  //默认使用
					{			
						if(uart_receive==0x5A)					Flag_Qian=0,Flag_Hou=0,Flag_Left=0,Flag_Right=0;		//////////////刹车
						
						else if(uart_receive==0x41)		Flag_Qian=1,Flag_Hou=0,Flag_Left=0,Flag_Right=0;		//////////////向前走
						else if(uart_receive==0x45)		Flag_Qian=0,Flag_Hou=1,Flag_Left=0,Flag_Right=0;		//////////////向后走
						else if(uart_receive==0x47)		Flag_Qian=0,Flag_Hou=0,Flag_Left=1,Flag_Right=0;  	//向左转
						else if(uart_receive==0x43)		Flag_Qian=0,Flag_Hou=0,Flag_Left=0,Flag_Right=1;  	//向右转
						
						else 																				Flag_Qian=0,Flag_Hou=0,Flag_Left=0,Flag_Right=0;		//////////////刹车
					}			
		}  											 
} 
