#include "control.hpp"
extern "C" {
#include "driver/motor.h"
}

int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}

volatile bool front_sensors[8];
volatile bool back_sensors[8];
// 假设前后2*8个传感器的数据存在上面两个数组里
void go() {
    int speed_B = motorB_getspeed();
    int speed_A = motorA_getspeed();
    int res_A = Velocity_A(30, speed_A);
    int res_B = Velocity_B(30, speed_B);
    res_A = range_protect(res_A, 0, 600);
    res_B = range_protect(res_B, 0, 600);
    Set_PWM(res_A, res_B);
}