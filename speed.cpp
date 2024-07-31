#include "speed.hpp"

#include "ti_msp_dl_config.h"
extern "C" {
#include "driver/motor.h"
#include "driver/oled.h"
#include "driver/encoder.h"
}
constexpr uint16_t max_pwm = 500;
int res_A = 0;
int res_B = 0;
int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}
int tarsL;
int tarsR;
void set_target_speed(int l, int r) {
    tarsL = l;
    tarsR = r;
    oled_print(2, "tarsp %d %d", l, r);
    oled_print(1, "pwm %d %d", res_A, res_B);
    oled_print(3, "real %d %d", motorA_getspeed(), motorB_getspeed());
    oled_print(4, "distance %d %d", encoderA_get(), encoderB_get());
    OLED_Refresh();
}
void speed_pid_irqHandler() {
    // 得出目标速度
    int speed_A = motorA_getspeed();
    int speed_B = motorB_getspeed();

    res_A = Velocity_A(tarsL, speed_A);
    res_B = Velocity_B(tarsR, speed_B);
    res_A = range_protect(res_A, 0, max_pwm);
    res_B = range_protect(res_B, 0, max_pwm);
    Set_PWM(res_A, res_B);
}
