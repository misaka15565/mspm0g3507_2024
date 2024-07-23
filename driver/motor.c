#include "motor.h"
#include "encoder.h"
#include "math.h"
#include "ti_msp_dl_config.h"

// 在使用我司的D157B驱动模块的时候，PB1接AIN1、PB3解AIN2,PA12接BIN1、PA13接着BIN2
float Velcity_Kp = 1.0, Velcity_Ki = 0.5, Velcity_Kd; // 相关速度PID参数
/***************************************************************************
函数功能：电机的PID闭环控制
入口参数：左右电机的编码器值
返回值  ：电机的PWM
***************************************************************************/
volatile int speed_left = 0;
volatile int speed_right = 0;
void update_speed_irq() {
  speed_left = encoderB_get();
  encoderB_clear();
  speed_right = encoderA_get();
  encoderA_clear();
}
int getspeed_left() { return speed_left; }
int getspeed_right() { return -speed_right; }

int Velocity_A(int TargetVelocity, int CurrentVelocity) {
  int Bias; // 定义相关变量
  static int ControlVelocityA,
      Last_biasA; // 静态变量，函数调用结束后其值依然存在

  Bias = TargetVelocity - CurrentVelocity; // 求速度偏差

  ControlVelocityA +=
      Velcity_Ki * (Bias - Last_biasA) +
      Velcity_Kp * Bias; // 增量式PI控制器
                         // Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
                         // Velcity_Ki*Bias 速度控制值由Bias不断积分得到
                         // 偏差越大加速度越大
  Last_biasA = Bias;
  if (ControlVelocityA > 999)
    ControlVelocityA = 999;
  else if (ControlVelocityA < -999)
    ControlVelocityA = -999;
  return ControlVelocityA; // 返回速度控制值
}

/***************************************************************************
函数功能：电机的PID闭环控制
入口参数：左右电机的编码器值
返回值  ：电机的PWM
***************************************************************************/
int Velocity_B(int TargetVelocity, int CurrentVelocity) {
  int Bias; // 定义相关变量
  static int ControlVelocityB,
      Last_biasB; // 静态变量，函数调用结束后其值依然存在

  Bias = TargetVelocity - CurrentVelocity; // 求速度偏差

  ControlVelocityB +=
      Velcity_Ki * (Bias - Last_biasB) +
      Velcity_Kp * Bias; // 增量式PI控制器
                         // Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
                         // Velcity_Ki*Bias 速度控制值由Bias不断积分得到
                         // 偏差越大加速度越大
  Last_biasB = Bias;
  if (ControlVelocityB > 999)
    ControlVelocityB = 999;
  else if (ControlVelocityB < -999)
    ControlVelocityB = -999;
  return ControlVelocityB; // 返回速度控制值
}
/*
void Set_PWM(int pwma, int pwmb) {
  //
在使用我司的D157B驱动模块的时候，PB1接AIN1、PB3解AIN2,PA12接BIN1、PA13接着BIN2
  if (pwma > 0) {
    DL_Timer_setCaptureCompareValue(PWM_0_INST, ABS(3199), GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, ABS(3199 - pwma),
                                    GPIO_PWM_0_C1_IDX);
  } else {
    DL_Timer_setCaptureCompareValue(PWM_0_INST, ABS(3199 + pwma),
                                    GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, ABS(3199), GPIO_PWM_0_C1_IDX);
  }
  if (pwmb > 0) {
    DL_Timer_setCaptureCompareValue(PWM_1_INST, ABS(3199), GPIO_PWM_1_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_1_INST, ABS(3199 - pwmb),
                                    GPIO_PWM_1_C1_IDX);
  } else {
    DL_Timer_setCaptureCompareValue(PWM_1_INST, ABS(3199 + pwmb),
                                    GPIO_PWM_1_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_1_INST, ABS(3199), GPIO_PWM_1_C1_IDX);
  }
}
*/