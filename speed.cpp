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
    if (l == 0 && r == 0) Set_PWM(0, 0); // 立刻停车
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

void motor_L_run_distance(int distance) {
    int left = encoderA_get();
    int right = encoderB_get();
    set_target_speed(1, 0);
    while (1) {
        if (abs(encoderA_get() - left) > distance) {
            Set_PWM(0, 0);
            set_target_speed(0, 0);
            break;
        }
    }
    set_target_speed(0, 0);
}

void motor_R_run_distance(int distance) {
    int left = encoderA_get();
    int right = encoderB_get();
    set_target_speed(0, 1);
    while (1) {
        if (abs(encoderB_get() - right) > distance) {
            Set_PWM(0, 0);
            set_target_speed(0, 0);
            break;
        }
    }
    set_target_speed(0, 0);
}

void motor_L_run_distance_at_speed(int distance, int speed) {
    int left = encoderA_get();
    int right = encoderB_get();
    set_target_speed(speed, 0);
    while (1) {
        if (abs(encoderA_get() - left) > distance) {
            Set_PWM(0, 0);
            set_target_speed(0, 0);
            break;
        }
    }
    set_target_speed(0, 0);
}
// 使右边的电机按指定速度转动指定的距离，如果不打滑就一定是准的
void motor_R_run_distance_at_speed(int distance, int speed) {
    int left = encoderA_get();
    int right = encoderB_get();
    set_target_speed(0, speed);
    while (1) {
        if (abs(encoderB_get() - right) > distance) {
            Set_PWM(0, 0);
            set_target_speed(0, 0);
            break;
        }
    }
    set_target_speed(0, 0);
}