#include "motor.h"
#include "encoder.h"
#include "math.h"
#include "ti_msp_dl_config.h"
#include <stdlib.h>

float Velcity_Kp = 1.0, Velcity_Ki = 0.5, Velcity_Kd; // 相关速度PID参数
/***************************************************************************
函数功能：电机的PID闭环控制
入口参数：左右电机的编码器值
返回值  ：电机的PWM
***************************************************************************/
static volatile int distanceA_buffer[10] = {};
static volatile int distanceB_buffer[10] = {};
static volatile int distanceA_buffer_index = 0;
static volatile int distanceB_buffer_index = 0;

static volatile int speed_B = 0;
static volatile int speed_A = 0;
// 每ms获取一次累计脉冲数，以本次获取的值减去上10ms的值，得到速度
void update_speed_irq() {
    int last_10ms_distance_A = distanceA_buffer[distanceA_buffer_index];
    int last_10ms_distance_B = distanceB_buffer[distanceB_buffer_index];
    distanceA_buffer[distanceA_buffer_index] = encoderA_get();
    distanceB_buffer[distanceB_buffer_index] = encoderB_get();
    speed_A = distanceA_buffer[distanceA_buffer_index] - last_10ms_distance_A;
    speed_B = distanceB_buffer[distanceB_buffer_index] - last_10ms_distance_B;
    distanceA_buffer_index = (distanceA_buffer_index + 1) % 10;
    distanceB_buffer_index = (distanceB_buffer_index + 1) % 10;
}
int motorB_getspeed() {
    return abs(speed_B);
}
int motorA_getspeed() {
    return abs(speed_A);
}

int Velocity_A(int TargetVelocity, int CurrentVelocity) {
    int Bias; // 定义相关变量
    static int ControlVelocityA,
        Last_biasA; // 静态变量，函数调用结束后其值依然存在

    Bias = TargetVelocity - CurrentVelocity; // 求速度偏差

    ControlVelocityA +=
        Velcity_Ki * (Bias - Last_biasA) + Velcity_Kp * Bias; // 增量式PI控制器
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
        Velcity_Ki * (Bias - Last_biasB) + Velcity_Kp * Bias; // 增量式PI控制器
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

// ch0 左电机
// ch1 右电机

void Set_PWM(int pwma, int pwmb) {
    const DL_TIMER_CC_INDEX motorA = DL_TIMER_CC_0_INDEX;
    const DL_TIMER_CC_INDEX motorB = DL_TIMER_CC_1_INDEX;
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, pwma, motorA);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, pwmb, motorB);
}
