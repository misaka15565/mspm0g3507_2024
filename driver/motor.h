#ifndef _MOTOR_H
#define _MOTOR_H
// 在使用我司的D157B驱动模块的时候，PB1接AIN1、PB3解AIN2,PA12接BIN1、PA13接着BIN2
int Velocity_A(int TargetVelocity, int CurrentVelocity);
int Velocity_B(int TargetVelocity, int CurrentVelocity);
void Set_PWM(int pwma, int pwmb);
void update_speed_irq();
int motorA_getspeed();
int motorB_getspeed();
void PID_clear_A();
void PID_clear_B();
extern float Velcity_Kp, Velcity_Ki, Velcity_Kd;
#endif