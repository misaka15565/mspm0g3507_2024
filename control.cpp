#include "control.hpp"
extern "C" {
uint16_t time_adjust = 100;
#include "driver/motor.h"
#include "driver/gray_sensor.h"
#include "driver/oled.h"
}
#include "statemachine.hpp"
#include "utils/delay.hpp"
// sensor1是前面的
int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}
// 0-7 --> 1-8
#define sensor1_get(x) GET_NTH_BIT(sensor1_res, x + 1)

// 对于传感器2（后面那个），8号检测在车的左后方，给转换成0号，这样和前面是统一的
//  0-7 --> 8-1
#define sensor2_get(x) GET_NTH_BIT(sensor2_res, 8 - x)

using u16 = uint16_t;
using u8 = uint8_t;
void go() {
// #define SUDOER_DEBUG
#ifdef SUDOER_DEBUG
    int speed_A = motorA_getspeed();
    int speed_B = motorB_getspeed();
    int target_left_speed = 10;
    int target_right_speed = 10;
    int res_A = Velocity_A(target_left_speed, speed_A);
    int res_B = Velocity_B(target_right_speed, speed_B);
    res_A = range_protect(res_A, 0, 1000);
    res_B = range_protect(res_B, 0, 1000);
    oled_print(5, "target %d %d", speed_A, speed_B);
    oled_print(6, "pwm %d %d", res_A, res_B);
    Set_PWM(res_A, res_B);

    return;
#endif
#ifndef SUDOER_DEBUG
    static state_machine sm;
    // 计算前进速度（可以pid可以固定
    // int default_left_speed = 47 / 3; // 顺时针跑的时候
    // int default_right_speed = 33 / 3;
    int default_left_speed = 10; // 顺时针跑的时候
    int default_right_speed = 10;
    int offset_speed = 4;
    float scale = 0;

    // 先获取黑线在哪，某位为0则说明是黑的
    uint8_t blacks[8];
    uint8_t blacks_count = 0;
    uint16_t sum = 0;
    for (int i = 0; i < 8; ++i) {
        if (sensor1_get(i) == 0) {
            blacks[blacks_count] = i * 2;
            ++blacks_count;
            sum += i * 2;
        }
    }
    static bool is_running_on_black_line = true;
    static bool adjust_action_done = false;
    if (blacks_count == 0 && is_running_on_black_line) {
        is_running_on_black_line = false;
        sm.action_performed(reach_blackline_end);
        // 未检测到黑线
        // TODO
        if (!adjust_action_done) {
            adjust_action_done = true;
            // 先写顺时针的
            Set_PWM(0, 0);
            int target_left_speed = 1;
            int target_right_speed = 0;
            uint32_t tar_time = sys_cur_tick_us + 1000 * time_adjust;
            while (sys_cur_tick_us < tar_time) {
                int speed_A = motorA_getspeed();
                int speed_B = motorB_getspeed();

                int res_A = Velocity_A(target_left_speed, speed_A);
                int res_B = Velocity_B(target_right_speed, speed_B);
                res_A = range_protect(res_A, 0, 1000);
                res_B = range_protect(res_B, 0, 1000);
                oled_print(5, "target %d %d", target_left_speed, target_right_speed);
                oled_print(6, "pwm %d %d", res_A, res_B);
                Set_PWM(res_A, 0);
            }
            return;
        }

    } else {
        is_running_on_black_line = true;
        adjust_action_done = false;
        u16 black_pos = sum / blacks_count;
        if (black_pos < 7) {
            // 若黑线在7号位置是中间，小于7说明黑线在左边
            scale = -0.5;
        } else if (black_pos > 7) {
            scale = 0.5;
        } else {
            scale = 0;
        }
        oled_print(0, "black point %d", black_pos);
        oled_print(1, "scale %f", scale);
    }

    int target_left_speed = default_left_speed + offset_speed * scale;
    int target_right_speed = default_right_speed - offset_speed * scale;

    // 得出目标速度
    int speed_A = motorA_getspeed();
    int speed_B = motorB_getspeed();

    int res_A = Velocity_A(target_left_speed, speed_A);
    int res_B = Velocity_B(target_right_speed, speed_B);
    res_A = range_protect(res_A, 0, 1000);
    res_B = range_protect(res_B, 0, 1000);
    oled_print(5, "target %d %d", target_left_speed, target_right_speed);
    oled_print(6, "pwm %d %d", res_A, res_B);
    Set_PWM(res_A, res_B);
#endif
}