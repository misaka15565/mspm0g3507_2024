#include "control.hpp"
extern "C" {
uint16_t time_adjust = 500;
#include "driver/motor.h"
#include "driver/gray_sensor.h"
#include "driver/oled.h"
#include "driver/BEEP.h"
#include "stdlib.h"
}
#include "ti_msp_dl_config.h"
#include "statemachine.hpp"
#include "utils/delay.hpp"
#include "speed.hpp"
#include <cmath>
// sensor1是前面的

// 0-7 --> 1-8
#define sensor1_get(x) GET_NTH_BIT(sensor1_res_flitered, x + 1)

// 对于传感器2（后面那个），8号检测在车的左后方，给转换成0号，这样和前面是统一的
//  0-7 --> 1-8
#define sensor2_get(x) GET_NTH_BIT(sensor2_res_flitered, x + 1)
#define S__BKPT()
using u16 = uint16_t;
using u8 = uint8_t;
using i16 = int16_t;

float weight_front = 0.15;
float weight_mid = 0.07;
uint16_t distance_adjust_after_leave_blackline = 0;

// 返回-1说明没黑线
// 0是黑线在传感器最左，14是黑线在传感器最右
i16 get_sensor1_blackline_pos() {
    u16 sum = 0;
    u8 black_count = 0;
    for (int i = 0; i < 8; ++i) {
        if (sensor1_get(i) == 0) {
            sum += i * 2;
            ++black_count;
        }
    }
    if (black_count == 0) {
        return -1;
    }
    return sum / black_count;
}

// 返回-1说明没黑线
// 0是黑线在传感器最左，14是黑线在传感器最右
i16 get_sensor2_blackline_pos() {
    u16 sum = 0;
    u8 black_count = 0;
    for (int i = 0; i < 8; ++i) {
        if (sensor2_get(i) == 0) {
            sum += i * 2;
            ++black_count;
        }
    }
    if (black_count == 0) {
        return -1;
    }
    return sum / black_count;
}

struct line_patrol_output {
    float scale;
    bool stoped;
};

static constexpr uint32_t time_1s_us = 1000000;

// 巡线核心
// 输出量：scale
line_patrol_output line_patrol_core(uint32_t starttime) {
    line_patrol_output ret;
    ret.scale = 0;
    ret.stoped = false;
    constexpr i16 mid_pos = 7;
    // 传感器更新

    // 尽量让黑线在两个传感器的中间位置

    i16 blackline_pos1 = get_sensor1_blackline_pos();
    i16 blackline_pos2 = get_sensor2_blackline_pos();

    if (blackline_pos1 == -1) {
        // 未检测到黑线
        if (starttime + time_1s_us < sys_cur_tick_us) {
            set_target_speed(0, 0);
            ret.stoped = true;
            return ret;
        } else {
            ret.scale = 0;
            return ret;
        }
    }
    if (blackline_pos2 == -1) {
        // 车刚进入，后面还没检测到黑线
        ret.scale = (blackline_pos1 - mid_pos) * weight_front;
        return ret;
    }

    // 现在两个都检测到

    // 右转正，左转负

    // 如果黑线在中线的左边
    if (blackline_pos1 < mid_pos && blackline_pos2 < mid_pos) {
        // 两个黑线都在左边，左转
        ret.scale = (blackline_pos1 - mid_pos) * weight_front;
        ret.scale += (blackline_pos2 - mid_pos) * weight_mid;
        ret.scale /= 2;
    } else if (blackline_pos1 > mid_pos && blackline_pos2 > mid_pos) {
        // 两个黑线都在右边，右转
        ret.scale = (blackline_pos1 - mid_pos) * weight_front;
        ret.scale += (blackline_pos2 - mid_pos) * weight_mid;
        ret.scale /= 2;
    } else if (blackline_pos1 < mid_pos && blackline_pos2 > mid_pos) {
        // 1在左，2在右，左转
        ret.scale = (blackline_pos1 - mid_pos) * weight_front;
        ret.scale -= (blackline_pos2 - mid_pos) * weight_mid;
        ret.scale /= 2;
    } else if (blackline_pos1 > mid_pos && blackline_pos2 < mid_pos) {
        // 1在右，2在左，右转
        ret.scale = (blackline_pos1 - mid_pos) * weight_front;
        ret.scale -= (blackline_pos2 - mid_pos) * weight_mid;
        ret.scale /= 2;
    }
    return ret;
}

// 从A到B，到B停车并声光提示，用时不大于15s
// 注意：车安放时前传感器下没黑线
void go_problem1() {
    constexpr u16 speed_default = 15;
    // 记录当前时间
    uint32_t start_time = sys_cur_tick_us;
    enable_acclimit();
    while (true) {
        // 传感器更新

        // 判断

        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 保持直行
            set_target_speed(speed_default, speed_default);
        } else {
            // 检测到黑线
            // 判定时间，若离出发不到1s，则忽略本次检测
            if (sys_cur_tick_us - start_time < 1000000) {
                continue;
            }
            // 到达终点，停车，报警
            set_target_speed(0, 0);
            BEEP_ms(1000);
            return;
        }
    }
}

constexpr int outside_speed_default = 15; // 寻迹时外轮的基础速度
constexpr int inside_speed_default = 10;
constexpr int offset_speed_default = 6;
// 本来是16 10 6

// 将小车放在位置 A 点，小车能自动行驶到 B 点后，沿半弧线行驶到 C
// 点，再由 C 点自动行驶到 D 点，最后沿半弧线行驶到 A 点停车，每经过一个点，
// 声光提示一次。完成一圈用时不大于 30 秒。（

void go_problem2() {
    constexpr u16 default_mid_speed = 15; // 默认直行速度
    constexpr u16 default_left_speed = outside_speed_default;
    constexpr u16 default_right_speed = inside_speed_default;
    constexpr u16 offset_speed = offset_speed_default;

    // mpu6050_prepare();
    // mpu6050_updateYaw();
    // A-->B
    uint32_t start_time_A = sys_cur_tick_us;
    oled_print(0, "A->B");
    OLED_Refresh();
    enable_acclimit();
    while (true) {
        // 传感器更新

        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 保持直行
            set_target_speed(default_mid_speed, default_mid_speed);
        } else {
            // 检测到黑线
            // 判定时间，若离出发不到1s，则忽略本次检测
            if (sys_cur_tick_us - start_time_A < 1000000) {
                continue;
            }
            // 到达终点，报警
            BEEP_ms(1000);
            break;
        }
    }
    // B-->C，寻迹
    oled_print(0, "B->C");
    OLED_Refresh();
    disable_acclimit();
    uint32_t start_time_B = sys_cur_tick_us;
    while (true) {
        auto ret = line_patrol_core(start_time_B);
        if (ret.stoped) {
            // 未检测到黑线
            // 可能已经到达了C点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            float scale = ret.scale;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
    // 向右微调一定编码器值
    // 左电机运动
    motor_L_run_distance(distance_adjust_after_leave_blackline);
    // 调整角度，使车头指向D
    oled_print(0, "adjust direction C->D");
    OLED_Refresh();
    i16 last_blackline_pos2 = -1;
    char last_run_motor = 'x';
    while (true) {
        // 不调了
        break;
        // 更新传感器

        // 判断
        // 保持右轮不动，左轮转动，让黑线在后传感器的12处

        i16 blackline_pos2 = get_sensor2_blackline_pos();
        if (blackline_pos2 == -1) {
            // 后面未检测到黑线
            // 得稍微动一下
            set_target_speed(0, 0);
            if (last_run_motor == 'L') {
                motor_R_run_distance(1);
            } else if (last_run_motor == 'R') {
                motor_L_run_distance(1);
            } else {
                motor_L_run_distance(1);
            }
            if (last_blackline_pos2 == 14 || last_blackline_pos2 == 0) {
                BEEP_ms(5000);
                break;
            }
        } else if (blackline_pos2 == 8) {
            // 进入下个阶段
            set_target_speed(0, 0);
            break;
        } else if (blackline_pos2 < 7) {
            motor_L_run_distance(1);
            last_run_motor = 'L';
        } else {
            motor_R_run_distance(7);
            last_run_motor = 'R';
        }
        if (blackline_pos2 != -1) last_blackline_pos2 = blackline_pos2;
    }
    PID_clear_A();
    PID_clear_B();
    // delay_ms(1000);
    //  C-->D
    uint32_t start_time_C = sys_cur_tick_us;
    oled_print(0, "C->D");
    OLED_Refresh();
    enable_acclimit();
    while (true) {
        // 传感器更新

        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 保持直行
            set_target_speed(default_mid_speed, default_mid_speed);
        } else {
            // 检测到黑线
            // 判定时间，若离出发不到1s，则忽略本次检测
            if (sys_cur_tick_us - start_time_C < 1000000) {
                continue;
            }
            // 到达终点，报警
            BEEP_ms(1000);
            break;
        }
    }
    // D-->A，寻迹
    oled_print(0, "D->A");
    OLED_Refresh();
    disable_acclimit();
    uint32_t start_time_D = sys_cur_tick_us;
    while (true) {
        auto ret = line_patrol_core(start_time_D);
        if (ret.stoped) {
            // 未检测到黑线
            // 可能已经到达了A点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            float scale = ret.scale;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
}

// 将小车放在位置 A 点，小车能自动行驶到 C 点后，沿半弧线行驶到 B
// 点，再由 B 点自动行驶到 D 点，最后沿半弧线行驶到 A 点停车。每经过一个点，
// 声光提示一次。完成一圈用时不大于 40 秒。
void go_problem3_inner_func(const int adj_A, const int adj_B) {
    constexpr int default_mid_speed = 15;
    float scale = 0;

    constexpr int outside_speed = outside_speed_default;
    constexpr int inside_speed = inside_speed_default;
    constexpr int offset_speed = offset_speed_default;

    // A-->C
    // 手工将小车放在A点，车头朝向B点
    // 调整
    Set_PWM(0, 0);
    PID_clear_A();
    PID_clear_B();
    motor_L_run_distance_at_speed(adj_A, 5);
    delay_ms(500);
    set_target_speed(0, 0);
    enable_acclimit();
    uint32_t start_time_A = sys_cur_tick_us;
    while (true) {
        // 传感器更新

        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 保持直行
            set_target_speed(default_mid_speed, default_mid_speed);
        } else {
            // 检测到黑线
            // 判定时间，若离出发不到1s，则忽略本次检测
            if (sys_cur_tick_us - start_time_A < 1000000) {
                continue;
            }
            // 到达终点，报警
            BEEP_ms(1000);
            break;
        }
    }
    // C-->B
    // 寻迹
    disable_acclimit();
    uint32_t start_time_C = sys_cur_tick_us;
    while (true) {
        constexpr int default_left_speed = inside_speed;
        constexpr int default_right_speed = outside_speed;
        auto ret = line_patrol_core(start_time_C);
        if (ret.stoped) {
            // 未检测到黑线
            // 可能已经到达了A点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            float scale = ret.scale;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
    // 在B点调整方向，使得车头指向D
    delay_ms(1000);
    Set_PWM(0, 0);
    PID_clear_A();
    PID_clear_B();
    // 先微调
    // 右电机运动
    motor_R_run_distance(distance_adjust_after_leave_blackline);
    motor_R_run_distance_at_speed(adj_B, 5);
    PID_clear_A();
    PID_clear_B();
    delay_ms(1000);
    set_target_speed(0, 0);
    enable_acclimit();
    // B-->D
    // 直行
    uint32_t start_time_B = sys_cur_tick_us;
    while (true) {
        // 传感器更新

        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 保持直行
            set_target_speed(default_mid_speed, default_mid_speed);
        } else {
            // 检测到黑线
            // 判定时间，若离出发不到1s，则忽略本次检测
            if (sys_cur_tick_us - start_time_B < 1000000) {
                continue;
            }
            // 到达终点，报警
            BEEP_ms(1000);
            break;
        }
    }
    // D-->A
    // 寻迹
    disable_acclimit();
    uint32_t start_time_D = sys_cur_tick_us;
    while (true) {
        constexpr int default_left_speed = outside_speed;
        constexpr int default_right_speed = inside_speed;
        auto ret = line_patrol_core(start_time_D);
        if (ret.stoped) {
            // 未检测到黑线
            // 可能已经到达了A点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            float scale = ret.scale;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
    // 运动到A后，微调方向
    // 左电机运动
    motor_L_run_distance(distance_adjust_after_leave_blackline);
    delay_ms(500);
}

uint16_t adjust_at_A_param = 270;
uint16_t adjust_at_B_param = 310;
uint16_t adjust_params[4][2] = {
    {270, 310},
    {270, 310},
    {280, 320},
    {290, 330},
};
void go_problem3() {
    go_problem3_inner_func(adjust_at_A_param, adjust_at_B_param);
}
void go_problem4() {
    for (int i = 0; i < 4; ++i) {
        go_problem3_inner_func(adjust_params[i][0], adjust_params[i][0]);
    }
}

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
    static uint32_t last_do_adjust_action_time = 0;
    if (blacks_count == 0 && is_running_on_black_line) {
        S__BKPT();
        is_running_on_black_line = false;
        sm.action_performed(reach_blackline_end);
        S__BKPT();
        // 未检测到黑线
        // TODO
        if (!adjust_action_done && last_do_adjust_action_time + 1000000 < sys_cur_tick_us) {
            S__BKPT();
            adjust_action_done = true;
            last_do_adjust_action_time = sys_cur_tick_us;
            // 先写顺时针的
            // Set_PWM(0, 0);
            BEEP_ms(100);
            int tls = 1;
            int tlr = 0;
            uint32_t tar_time = sys_cur_tick_us + 1000 * time_adjust;
            set_target_speed(tls, tlr);
            while (sys_cur_tick_us < tar_time) {
            }
        }

    } else if (blacks_count != 0) {
        S__BKPT();
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
    set_target_speed(target_left_speed, target_right_speed);
#endif
}