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


static bool en_acclimit = true;

void enable_acclimit() {
    en_acclimit = true;
}
void disable_acclimit() {
    en_acclimit = false;
}

// 每ms一次
void speed_pid_irqHandler() {
    static int count = 0;
    static int tol = 3;
    static int last_tarsL = 0;
    static int last_tarsR = 0;
    //  限制加速用
    static int left_real_tar_speed = 0;
    static int right_real_tar_speed = 0;
    ++count;
    if (count == 50) {
        //  限制每50ms加速一次
        count = 0;
        // 如果启用加速限制
        if (en_acclimit) {
            if (left_real_tar_speed < tarsL) {
                left_real_tar_speed++;
            } else if (left_real_tar_speed > tarsL) {
                left_real_tar_speed--;
            }

            if (right_real_tar_speed < tarsR) {
                right_real_tar_speed++;
            } else if (right_real_tar_speed > tarsR) {
                right_real_tar_speed--;
            }
        }
    }
    if (en_acclimit == false) {
        // 如果不启用加速限制
        left_real_tar_speed = tarsL;
        right_real_tar_speed = tarsR;
        // 立刻生效
    }

    if (tarsL == 0) {
        left_real_tar_speed = 0;
    }
    if (tarsR == 0) {
        right_real_tar_speed = 0;
    }

    // 获取当前转速
    int speed_A = motorA_getspeed();
    int speed_B = motorB_getspeed();

    res_A = Velocity_A(left_real_tar_speed, speed_A);
    res_B = Velocity_B(right_real_tar_speed, speed_B);
    res_A = range_protect(res_A, 0, max_pwm);
    res_B = range_protect(res_B, 0, max_pwm);
    Set_PWM(res_A, res_B);
}
