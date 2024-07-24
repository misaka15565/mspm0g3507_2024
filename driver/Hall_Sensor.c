//******************************************************************************              
//name:             GUA_Hall_Sensor.c             
//introduce:        霍尔传感器驱动      
//author:           甜甜的大香瓜                     
//email:            897503845@qq.com     
//QQ group          香瓜单片机之STM8/STM32(164311667)                  
//changetime:       2017.03.06                     
//******************************************************************************
#include "ti/driverlib/dl_gpio.h"
#include "ti_msp_dl_config.h" 
#include "Hall_Sensor.h"
 
//消抖总次数
#define HALL_SENSOR_DISAPPERAS_SHAKS_COUNT      500000
/*********************内部变量************************/
static unsigned long Hall_Sensor_DisapperasShakes_IdleCount = 0;			//消抖时的空闲状态计数值
static unsigned long Hall_Sensor_DisapperasShakes_TriggerCount = 0;	//消抖时的触发状态计数值
 
/*********************内部函数************************/ 
unsigned char Hall_Sensor_Check_Pin(void)    
{    
  //没触发
  if(DL_GPIO_readPins(GPIO_HALL_SENSOR_PORT, GPIO_HALL_SENSOR_Hall_Sensor_PIN)) 
  {
    //计数
    Hall_Sensor_DisapperasShakes_IdleCount++;
    Hall_Sensor_DisapperasShakes_TriggerCount = 0;
    
    //判断计数是否足够
    if(Hall_Sensor_DisapperasShakes_IdleCount >= HALL_SENSOR_DISAPPERAS_SHAKS_COUNT)
    {
      return HALL_SENSOR_STATUS_IDLE;    
    }
  }
  //触发
  else
  {
    //计数
    Hall_Sensor_DisapperasShakes_IdleCount = 0;
    Hall_Sensor_DisapperasShakes_TriggerCount++;
    
    //判断计数是否足够
    if(Hall_Sensor_DisapperasShakes_TriggerCount >= HALL_SENSOR_DISAPPERAS_SHAKS_COUNT)
    {
      return HALL_SENSOR_STATUS_TRIGGER;    
    }    
  }  
  
  return HALL_SENSOR_STATUS_DISAPPERAS_SHAKS;  
} 
