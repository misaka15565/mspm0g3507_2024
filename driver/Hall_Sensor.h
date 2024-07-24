//******************************************************************************              
//name:             GUA_Hall_Sensor.h             
//introduce:        霍尔传感器驱动的头文件      
//author:           甜甜的大香瓜                     
//email:            897503845@qq.com         
//QQ group          香瓜单片机之STM8/STM32(164311667)                      
//changetime:       2017.03.06                     
//****************************************************************************** 
#ifndef _HALL_SENSOR_H_
#define _HALL_SENSOR_H_
 
/*********************宏定义************************/  
 
//霍尔传感器的触发状态
#define HALL_SENSOR_STATUS_TRIGGER                      0		//霍尔传感器触发
#define HALL_SENSOR_STATUS_IDLE                         1		//霍尔传感器没触发
#define HALL_SENSOR_STATUS_DISAPPERAS_SHAKS             2		//霍尔传感器消抖中
 
/*********************外部函数声明************************/ 
unsigned char Hall_Sensor_Check_Pin(void);  
void Hall_Sensor_Init(void);
 
#endif
