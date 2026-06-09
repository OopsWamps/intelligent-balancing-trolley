#include "control.h"	
#include "filter.h"	
  /**************************************************************************
×÷Õß£ºÖĞ¿ÆÉî¹È
http://www.si-valley.cn/
**************************************************************************/
int Balance_Pwm,Velocity_Pwm,Turn_Pwm;
u8 Flag_Target;
u32 Flash_R_Count;
int Voltage_Temp,Voltage_Count,Voltage_All;
/**************************************************************************
º¯Êı¹¦ÄÜ£ºËùÓĞµÄ¿ØÖÆ´úÂë¶¼ÔÚÕâÀïÃæ
         5ms¶¨Ê±ÖĞ¶ÏÓÉMPU6050µÄINTÒı½Å´¥·¢
         ÑÏ¸ñ±£Ö¤²ÉÑùºÍÊı¾İ´¦ÀíµÄÊ±¼äÍ¬²½				 
**************************************************************************/
int EXTI15_10_IRQHandler(void) 
{    
	 if(INT==0)		
	{   
			EXTI->PR=1<<12;                                                   //Çå³ıÖĞ¶Ï±êÖ¾Î»   
		  Flag_Target=!Flag_Target;
		
		  if(delay_flag==1)
			{
				 if(++delay_50==10)	 delay_50=0,delay_flag=0;    //¸øÖ÷º¯ÊıÌá¹©50msµÄ¾«×¼ÑÓÊ±
			}

			if(Flag_Target==1)                                                  //5ms¶ÁÈ¡Ò»´ÎÍÓÂİÒÇºÍ¼ÓËÙ¶È¼ÆµÄÖµ£¬¸ü¸ßµÄ²ÉÑùÆµÂÊ¿ÉÒÔ¸ÄÉÆ¿¨¶ûÂüÂË²¨ºÍ»¥²¹ÂË²¨µÄĞ§¹û
			{																																								//10ms  -----2020/10/6   yuan
					Get_Angle(Way_Angle);                                          //===¸üĞÂ×ËÌ¬		ÈıÖÖÖ®Ò»·½·¨»ñÈ¡½Ç¶È  yuan
						
					if(++Flash_R_Count==150&&Angle_Balance>30)		Flash_Read();      //=====¶ÁÈ¡FlashµÄPID²ÎÊı		
					Voltage_Temp=Get_battery_volt();		                        //=====¶ÁÈ¡µç³ØµçÑ¹		
					Voltage_Count++;                                                    //=====Æ½¾ùÖµ¼ÆÊıÆ÷
					Voltage_All+=Voltage_Temp;                                    //=====¶à´Î²ÉÑùÀÛ»ı
					if(Voltage_Count==100) Voltage=Voltage_All/100,Voltage_All=0,Voltage_Count=0;//=====ÇóÆ½¾ùÖµ		
					return 0;	                                               
			}                                                                   						//10ms¿ØÖÆÒ»´Î£¬ÎªÁË±£Ö¤M·¨²âËÙµÄÊ±¼ä»ù×¼£¬Ê×ÏÈ¶ÁÈ¡±àÂëÆ÷Êı¾İ

			Encoder_Left=Read_Encoder(2);                            //===¶ÁÈ¡±àÂëÆ÷µÄÖµ
			Encoder_Right=Read_Encoder(4);                          //===¶ÁÈ¡±àÂëÆ÷µÄÖµ
	  	Get_Angle(Way_Angle);                                          //===¸üĞÂ×ËÌ¬	
			Read_Distane();                                                      //===»ñÈ¡³¬Éù²¨²âÁ¿¾àÀëÖµ
  		if(Bi_zhang==0)		Led_Flash(100);                          //===LEDÉÁË¸;³£¹æÄ£Ê½ 1s¸Ä±äÒ»´ÎÖ¸Ê¾µÆµÄ×´Ì¬	
			if(Bi_zhang==1)		Led_Flash(0);                              //===LEDÉÁË¸;±ÜÕÏÄ£Ê½ Ö¸Ê¾µÆ³£ÁÁ	
			
			Key();                                                              					//===É¨Ãè°´¼ü×´Ì¬ µ¥»÷Ë«»÷¿ÉÒÔ¸Ä±äĞ¡³µÔËĞĞ×´Ì¬
 			Balance_Pwm = balance(Angle_Balance,Gyro_Balance);                  //===Æ½ºâPID¿ØÖÆ	
	    Velocity_Pwm = velocity(Encoder_Left,Encoder_Right);                  	//===ËÙ¶È»·PID¿ØÖÆ	 ¼Ç×¡£¬ËÙ¶È·´À¡ÊÇÕı·´À¡£¬¾ÍÊÇĞ¡³µ¿ìµÄÊ±ºòÒªÂıÏÂÀ´¾ÍĞèÒªÔÙÅÜ¿ìÒ»µã
 	    Turn_Pwm      = turn(Encoder_Left,Encoder_Right,Gyro_Turn);         //===×ªÏò»·PID¿ØÖÆ     
 		  Moto1 = -Balance_Pwm+Velocity_Pwm-Turn_Pwm;                           //===¼ÆËã×óÂÖµç»ú×îÖÕPWM
 	  	Moto2 = -Balance_Pwm+Velocity_Pwm+Turn_Pwm;                          //===¼ÆËãÓÒÂÖµç»ú×îÖÕPWM
			
   		Xianfu_Pwm();                                                       	//===PWMÏŞ·ù
			
			if(Pick_Up(Acceleration_Z,Angle_Balance,Encoder_Left,Encoder_Right))		//===¼ì²éÊÇ·ñĞ¡³µ±»ÄÃÆğ
					Flag_Stop=1;	                                                      //===Èç¹û±»ÄÃÆğ¾Í¹Ø±Õµç»ú
			if(Put_Down(Angle_Balance,Encoder_Left,Encoder_Right))              //===¼ì²éÊÇ·ñĞ¡³µ±»·ÅÏÂ
					Flag_Stop=0;	                                                      //===Èç¹û±»·ÅÏÂ¾ÍÆô¶¯µç»ú
      if(Turn_Off(Angle_Balance,Voltage)==0)                  //===Èç¹û²»´æÔÚÒì³£
					Set_Pwm(Moto1,Moto2);                                     //===¸³Öµ¸øPWM¼Ä´æÆ÷  
	}       	
	 return 0;	  
} 

/**************************************************************************
º¯Êı¹¦ÄÜ£ºÖ±Á¢PD¿ØÖÆ
Èë¿Ú²ÎÊı£º½Ç¶È¡¢½ÇËÙ¶È
·µ»Ø  Öµ£ºÖ±Á¢¿ØÖÆPWM
×÷    Õß£ºÆ½ºâĞ¡³µÖ®¼Ò
**************************************************************************/
int balance(float Angle,float Gyro)
{  
   float Bias;
	 int balance;
	 Bias=Angle-ZHONGZHI;                       								//===Çó³öÆ½ºâµÄ½Ç¶ÈÖĞÖµ ºÍ»úĞµÏà¹Ø
	 balance=Balance_Kp*Bias+Gyro*Balance_Kd;   //===¼ÆËãÆ½ºâ¿ØÖÆµÄµç»úPWM  PD¿ØÖÆ   kpÊÇPÏµÊı kdÊÇDÏµÊı 
	 return balance;
}

/**************************************************************************
º¯Êı¹¦ÄÜ£ºËÙ¶ÈPI¿ØÖÆ ĞŞ¸ÄÇ°½øºóÍËËÙ¶È£¬ÇëĞŞTarget_Velocity£¬±ÈÈç£¬¸Ä³É60¾Í±È½ÏÂıÁË
Èë¿Ú²ÎÊı£º×óÂÖ±àÂëÆ÷¡¢ÓÒÂÖ±àÂëÆ÷
·µ»Ø  Öµ£ºËÙ¶È¿ØÖÆPWM
×÷    Õß£ºÆ½ºâĞ¡³µÖ®¼Ò
**************************************************************************/
int velocity(int encoder_left,int encoder_right)
{  
     static float Velocity,Encoder_Least,Encoder,Movement;
	  static float Encoder_Integral,Target_Velocity;
	  //=============Ò£¿ØÇ°½øºóÍË²¿·Ö=======================
	  if(Bi_zhang==1&&Flag_sudu==1)  Target_Velocity=55;                 			//Èç¹û½øÈë±ÜÕÏÄ£Ê½,×Ô¶¯½øÈëµÍËÙÄ£Ê½
    else 	                         	Target_Velocity = 110;                 
		if(1==Flag_Qian)    	  Movement = Target_Velocity/Flag_sudu;	         	//===Ç°½ø±êÖ¾Î»ÖÃ1 
		else if(1==Flag_Hou)	Movement = -Target_Velocity/Flag_sudu;         		//===ºóÍË±êÖ¾Î»ÖÃ1
	  else  Movement=0;	
	  if(Bi_zhang==1&&Distance<500&&Flag_Left!=1&&Flag_Right!=1)     //±ÜÕÏ±êÖ¾Î»ÖÃ1ÇÒ·ÇÒ£¿Ø×ªÍäµÄÊ±ºò£¬½øÈë±ÜÕÏÄ£Ê½
	  Movement=-Target_Velocity/Flag_sudu;
   //=============ËÙ¶ÈPI¿ØÖÆÆ÷=======================//	
		Encoder_Least =(encoder_left+encoder_right)-0; //===»ñÈ¡×îĞÂËÙ¶ÈÆ«²î==²âÁ¿ËÙ¶È£¨×óÓÒ±àÂëÆ÷Ö®ºÍ£©-Ä¿±êËÙ¶È£¨´Ë´¦ÎªÁã£© 
		Encoder *= 0.8;		                                                															//===Ò»½×µÍÍ¨ÂË²¨Æ÷       
		Encoder += Encoder_Least*0.2;	                                    									//===Ò»½×µÍÍ¨ÂË²¨Æ÷    
		Encoder_Integral +=Encoder;                                       											//===»ı·Ö³öÎ»ÒÆ »ı·ÖÊ±¼ä£º10ms
		Encoder_Integral=Encoder_Integral-Movement;                       					//===½ÓÊÕÒ£¿ØÆ÷Êı¾İ£¬¿ØÖÆÇ°½øºóÍË
		if(Encoder_Integral>10000)  	Encoder_Integral=10000;           					//===»ı·ÖÏŞ·ù
		if(Encoder_Integral<-10000)	Encoder_Integral=-10000;          					//===»ı·ÖÏŞ·ù	
		Velocity=Encoder*Velocity_Kp+Encoder_Integral*Velocity_Ki;  					//===ËÙ¶È¿ØÖÆ	
		if(Turn_Off(Angle_Balance,Voltage)==1||Flag_Stop==1)   Encoder_Integral=0;      //===µç»ú¹Ø±ÕºóÇå³ı»ı·Ö
	  return Velocity;
}

/**************************************************************************
º¯Êı¹¦ÄÜ£º×ªÏò¿ØÖÆ  ĞŞ¸Ä×ªÏòËÙ¶È£¬ÇëĞŞ¸ÄTurn_Amplitude¼´¿É
Èë¿Ú²ÎÊı£º×óÂÖ±àÂëÆ÷¡¢ÓÒÂÖ±àÂëÆ÷¡¢ZÖáÍÓÂİÒÇ
·µ»Ø  Öµ£º×ªÏò¿ØÖÆPWM
×÷    Õß£ºÆ½ºâĞ¡³µÖ®¼Ò
**************************************************************************/
int turn(int encoder_left,int encoder_right,float gyro)//×ªÏò¿ØÖÆ
{
	 static float Turn_Target,Turn,Encoder_temp,Turn_Convert=0.9,Turn_Count;
	  float Turn_Amplitude=88/Flag_sudu,Kp=42,Kd=0;     
	  //=============Ò£¿Ø×óÓÒĞı×ª²¿·Ö=======================//
  	if(1==Flag_Left||1==Flag_Right)                      //ÕâÒ»²¿·ÖÖ÷ÒªÊÇ¸ù¾İĞı×ªÇ°µÄËÙ¶Èµ÷ÕûËÙ¶ÈµÄÆğÊ¼ËÙ¶È£¬Ôö¼ÓĞ¡³µµÄÊÊÓ¦ĞÔ
		{
			if(++Turn_Count==1)
			Encoder_temp=myabs(encoder_left+encoder_right);
			Turn_Convert=50/Encoder_temp;
			if(Turn_Convert<0.6)Turn_Convert=0.6;
			if(Turn_Convert>3)Turn_Convert=3;
		}	
	  else
		{
			Turn_Convert=0.9;
			Turn_Count=0;
			Encoder_temp=0;
		}			
		if(1==Flag_Left)	           			Turn_Target		-=		Turn_Convert;
		else if(1==Flag_Right)	     	Turn_Target		+=	Turn_Convert; 
		else Turn_Target=0;
	
    if(Turn_Target>Turn_Amplitude)  Turn_Target=Turn_Amplitude;    //===×ªÏòËÙ¶ÈÏŞ·ù
	  if(Turn_Target<-Turn_Amplitude) Turn_Target=-Turn_Amplitude;
		if(Flag_Qian==1||Flag_Hou==1)  Kd=0.5;					//1.4;        
		else Kd=0;   				//×ªÏòµÄÊ±ºòÈ¡ÏûÍÓÂİÒÇµÄ¾ÀÕı ÓĞµãÄ£ºıPIDµÄË¼Ïë
  	//=============×ªÏòPD¿ØÖÆÆ÷=======================//
		Turn	=	-Turn_Target*Kp - gyro*Kd;                 	//===½áºÏZÖáÍÓÂİÒÇ½øĞĞPD¿ØÖÆ
	  return Turn;
}

/**************************************************************************
º¯Êı¹¦ÄÜ£º¸³Öµ¸øPWM¼Ä´æÆ÷
Èë¿Ú²ÎÊı£º×óÂÖPWM¡¢ÓÒÂÖPWM
·µ»Ø  Öµ£ºÎŞ
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
º¯Êı¹¦ÄÜ£ºÏŞÖÆPWM¸³Öµ 
Èë¿Ú²ÎÊı£ºÎŞ
·µ»Ø  Öµ£ºÎŞ
**************************************************************************/
void Xianfu_Pwm(void)
{	
	  int Amplitude		=	6900;    						//===PWMÂú·ùÊÇ7200 ÏŞÖÆÔÚ6900
//		if(Flag_Qian==1)  Moto1+=DIFFERENCE;  //DIFFERENCEÊÇÒ»¸öºâÁ¿Æ½ºâĞ¡³µµç»úºÍ»úĞµ°²×°²îÒìµÄÒ»¸ö±äÁ¿¡£Ö±½Ó×÷ÓÃÓÚÊä³ö£¬ÈÃĞ¡³µ¾ßÓĞ¸üºÃµÄÒ»ÖÂĞÔ¡£
//	  if(Flag_Hou==1)   Moto2-=DIFFERENCE;
    if(Moto1<-Amplitude) 		Moto1	=	-Amplitude;	
		if(Moto1>Amplitude)  		Moto1	=	Amplitude;	
	  if(Moto2<-Amplitude) 		Moto2	=	-Amplitude;	
		if(Moto2>Amplitude)  		Moto2	=	Amplitude;		
}
/**************************************************************************
º¯Êı¹¦ÄÜ£º°´¼üĞŞ¸ÄĞ¡³µÔËĞĞ×´Ì¬ 
Èë¿Ú²ÎÊı£ºÎŞ
·µ»Ø  Öµ£ºÎŞ
**************************************************************************/
void Key(void)
{	
	u8 tmp,tmp2;
	tmp=click_N_Double(50); 
	if(tmp==1)	Flag_Stop	=	!Flag_Stop;			//µ¥»÷¿ØÖÆĞ¡³µµÄÆôÍ£
	if(tmp==2)	Flag_Show	=	!Flag_Show;			//Ë«»÷¿ØÖÆĞ¡³µµÄÏÔÊ¾×´Ì¬
	tmp2=Long_Press();                   
  if(tmp2==1) 		Bi_zhang=!Bi_zhang;					//³¤°´¿ØÖÆĞ¡³µÊÇ·ñ½øÈë³¬Éù²¨±ÜÕÏÄ£Ê½ 
}

/**************************************************************************
º¯Êı¹¦ÄÜ£ºÒì³£¹Ø±Õµç»ú
Èë¿Ú²ÎÊı£ºÇã½ÇºÍµçÑ¹
·µ»Ø  Öµ£º1£ºÒì³£  0£ºÕı³£
**************************************************************************/
u8 Turn_Off(float angle, int voltage)
{
	    u8 temp;
	
			if(angle<-40||angle>40||1==Flag_Stop||voltage<1110)			//µç³ØµçÑ¹µÍÓÚ11.1V¹Ø±Õµç»ú
			{	                                                 					//===Çã½Ç´óÓÚ40¶È¹Ø±Õµç»ú
					temp=1;                                         		//===Flag_StopÖÃ1¹Ø±Õµç»ú
					AIN1=0;                                            
					AIN2=0;
					BIN1=0;
					BIN2=0;
      }
			else
					temp=0;
      return 	temp;			
}
	
/**************************************************************************
º¯Êı¹¦ÄÜ£º»ñÈ¡½Ç¶È ÈıÖÖËã·¨¾­¹ıÎÒÃÇµÄµ÷Ğ££¬¶¼·Ç³£ÀíÏë 
Èë¿Ú²ÎÊı£º»ñÈ¡½Ç¶ÈµÄËã·¨ 1£ºDMP  2£º¿¨¶ûÂü 3£º»¥²¹ÂË²¨
·µ»Ø  Öµ£ºÎŞ
**************************************************************************/
void Get_Angle(u8 way)
{ 
	    float Accel_Y,Accel_Angle,Accel_Z,Gyro_X,Gyro_Z;
	   	Temperature=Read_Temperature();      	//===¶ÁÈ¡MPU6050ÄÚÖÃÎÂ¶È´«¸ĞÆ÷Êı¾İ£¬½üËÆ±íÊ¾Ö÷°åÎÂ¶È¡£
	
	    if(way==1)                           										//===DMPµÄ¶ÁÈ¡ÔÚÊı¾İ²É¼¯ÖĞ¶Ï¶ÁÈ¡£¬ÑÏ¸ñ×ñÑ­Ê±ĞòÒªÇó
			{	
					Read_DMP();                      								//===¶ÁÈ¡¼ÓËÙ¶È¡¢½ÇËÙ¶È¡¢Çã½Ç
					Angle_Balance=-Roll;             						//===¸üĞÂÆ½ºâÇã½Ç
					Gyro_Balance=-gyro[0];            					//===¸üĞÂÆ½ºâ½ÇËÙ¶È
					Gyro_Turn=gyro[2];               							//===¸üĞÂ×ªÏò½ÇËÙ¶È
				  Acceleration_Z=accel[2];         					//===¸üĞÂZÖá¼ÓËÙ¶È¼Æ
			}			
      else
      {
					Gyro_X=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_XOUT_L);    //¶ÁÈ¡YÖáÍÓÂİÒÇ
					Gyro_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_L);    //¶ÁÈ¡ZÖáÍÓÂİÒÇ
					Accel_Y=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_YOUT_L); //¶ÁÈ¡XÖá¼ÓËÙ¶È¼Æ
					Accel_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L); //¶ÁÈ¡ZÖá¼ÓËÙ¶È¼Æ
					if(Gyro_X>32768)  Gyro_X-=65536;                      //Êı¾İÀàĞÍ×ª»»  Ò²¿ÉÍ¨¹ıshortÇ¿ÖÆÀàĞÍ×ª»»
					if(Gyro_Z>32768)  Gyro_Z-=65536;                      //Êı¾İÀàĞÍ×ª»»
					if(Accel_Y>32768) Accel_Y-=65536;                     //Êı¾İÀàĞÍ×ª»»
					if(Accel_Z>32768) Accel_Z-=65536;                     //Êı¾İÀàĞÍ×ª»»
					Gyro_Balance=Gyro_X;                                  				//¸üĞÂÆ½ºâ½ÇËÙ¶È
					Accel_Angle=atan2(Accel_Y,Accel_Z)*180/PI;       //¼ÆËãÇã½Ç	
					Gyro_X=Gyro_X/16.4;                                    				//ÍÓÂİÒÇÁ¿³Ì×ª»»	
						
//					if(Way_Angle==2)		  	Kalman_Filter(Accel_Angle,Gyro_X);		//¿¨¶ûÂüÂË²
					if(way==2)		  	Kalman_Filter(Accel_Angle,Gyro_X);		//¿¨¶ûÂüÂË²

//					else if(Way_Angle==3)   Yijielvbo(Accel_Angle,Gyro_X);    			//»¥²¹ÂË²¨
					else if(way==3)   Yijielvbo(Accel_Angle,Gyro_X);    			//»¥²¹ÂË²¨
					Angle_Balance=angle;                                   					//¸üĞÂÆ½ºâÇã½Ç
					Gyro_Turn=Gyro_Z;                                      					//¸üĞÂ×ªÏò½ÇËÙ¶È
					Acceleration_Z=Accel_Z;                                				//===¸üĞÂZÖá¼ÓËÙ¶È¼Æ	
		}
}
/**************************************************************************
º¯Êı¹¦ÄÜ£º¾ø¶ÔÖµº¯Êı
Èë¿Ú²ÎÊı£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
int myabs(int a)
{ 		   
	  int temp;
	
		if(a<0)  temp	=	-a;  
	  else 			temp		=	a;
	  return 	temp;
}
/**************************************************************************
º¯Êı¹¦ÄÜ£º¼ì²âĞ¡³µÊÇ·ñ±»ÄÃÆğ
Èë¿Ú²ÎÊı£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
int Pick_Up(float Acceleration,float Angle,int encoder_left,int encoder_right)
{ 		   
	 static u16 flag,count0,count1,count2;
	
	 if(flag==0)                                                                   //µÚÒ»²½
	 {
	      if(myabs(encoder_left)+myabs(encoder_right)<30)    //Ìõ¼ş1£¬Ğ¡³µ½Ó½ü¾²Ö¹
				count0++;
        else 
        count0=0;		
        if(count0>10)				
		    flag=1,count0=0; 
	 } 
	 
	 if(flag==1)                                                                  //½øÈëµÚ¶ş²½
	 {
		    if(++count1>200)       count1=0,flag=0;                 //³¬Ê±²»ÔÙµÈ´ı2000ms
	      if(Acceleration>26000&&(Angle>(-20+ZHONGZHI))&&(Angle<(20+ZHONGZHI)))   //Ìõ¼ş2£¬Ğ¡³µÊÇÔÚ0¶È¸½½ü±»ÄÃÆğ
		    flag=2; 
	 } 
	 
	 if(flag==2)                                                                  //µÚÈı²½
	 {
		  if(++count2>100)       count2=0,flag=0;                   //³¬Ê±²»ÔÙµÈ´ı1000ms
	    if(myabs(encoder_left+encoder_right)>135)            //Ìõ¼ş3£¬Ğ¡³µµÄÂÖÌ¥ÒòÎªÕı·´À¡´ïµ½×î´óµÄ×ªËÙ   
      {
				flag=0;                                                                                     
				return 1;                                                                //¼ì²âµ½Ğ¡³µ±»ÄÃÆğ
			}
	 }
	 
	 return 0;
}
/**************************************************************************
º¯Êı¹¦ÄÜ£º¼ì²âĞ¡³µÊÇ·ñ±»·ÅÏÂ
Èë¿Ú²ÎÊı£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
int Put_Down(float Angle,int encoder_left,int encoder_right)
{ 		   
	 static u16 flag,count;	 
	 if(Flag_Stop==0)                           							//·ÀÖ¹Îó¼ì      
   return 0;	                 
	 if(flag==0)                                               
	 {
	      if(Angle>(-10+ZHONGZHI)&&Angle<(10+ZHONGZHI)&&encoder_left==0&&encoder_right==0)         //Ìõ¼ş1£¬Ğ¡³µÊÇÔÚ0¶È¸½½üµÄ
		    flag=1; 
	 } 
	 if(flag==1)                                               
	 {
		  if(++count>50)                                          //³¬Ê±²»ÔÙµÈ´ı 500ms
		  {
				count=0;flag=0;
		  }
	    if(encoder_left<-3&&encoder_right<-3&&encoder_left>-60&&encoder_right>-60)     //Ìõ¼ş2£¬Ğ¡³µµÄÂÖÌ¥ÔÚÎ´ÉÏµçµÄÊ±ºò±»ÈËÎª×ª¶¯  
      {
				flag=0;
				flag=0;
				return 1;                                             		//¼ì²âµ½Ğ¡³µ±»·ÅÏÂ
			}
	 }
	return 0;
}


