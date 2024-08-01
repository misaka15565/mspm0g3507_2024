#include "control.hpp"
#include <cstring>
extern "C" {
uint16_t time_adjust = 500;
#include "driver/motor.h"
#include "driver/encoder.h"
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

i16 posture_tar_from_prob2 = 6;
i16 posture_tar_to_prob3 = 5;

i16 posture_tar_from_prob3_clockwise = 6;
i16 posture_tar_to_prob3_clockwise = 5;

i16 posture_tar_from_prob3_counter_clockwise = 8;
i16 posture_tar_to_prob3_counter_clockwise = 9;

constexpr uint32_t time_half_circle_need = 5 * 1000000;

constexpr uint32_t half_circle_outside_distance = 8467 - 3419 - 300;
constexpr uint32_t half_circle_inside_distance = 7100 - 3408 - 300;
enum direction {
    LEFT,
    RIGHT
};

void micro_adjust(direction d) {
    if (distance_adjust_after_leave_blackline == 0) return;
    if (d == LEFT) {
        motor_R_run_distance(distance_adjust_after_leave_blackline);
    } else {
        motor_L_run_distance(distance_adjust_after_leave_blackline);
    }
}

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

// 姿态调整核心
void posture_adjust_core(const i16 target_pos_from, const i16 target_pos_to) {
    // 先调整到target_pos_from，然后调整到target_pos_to
    // 理论上，这样可以使得黑线被调整到target_pos_to的左极限或者右极限位置
    PID_clear_A();
    PID_clear_B();
    delay_ms(500);
    while (true) {
        i16 pos = get_sensor2_blackline_pos();
        oled_print(7, "pos %d sensor %02X", pos, sensor2_res_flitered);
        OLED_Refresh();
        if (pos == -1) {
            set_target_speed(0, 0);
            break;
        } else if (pos < target_pos_from) {
            motor_L_run_distance(1);
        } else if (pos > target_pos_from) {
            motor_R_run_distance(1);
        } else {
            set_target_speed(0, 0);
            break;
        }
    }
    PID_clear_A();
    PID_clear_B();

    // 从from调整到to应该只会向一个方向调整
    if (target_pos_from < target_pos_to) {
        while (true) {
            i16 pos = get_sensor2_blackline_pos();
            oled_print(7, "pos %d sensor %02X", pos, sensor2_res_flitered);
            OLED_Refresh();
            if (pos == -1) {
                set_target_speed(0, 0);
                break;
            } else if (pos < target_pos_to) {
                motor_L_run_distance(1);
            } else {
                set_target_speed(0, 0);
                break;
            }
        }
    } else if (target_pos_from > target_pos_to) {
        while (true) {
            i16 pos = get_sensor2_blackline_pos();
            oled_print(7, "pos %d sensor %02X", pos, sensor2_res_flitered);
            OLED_Refresh();
            if (pos == -1) {
                set_target_speed(0, 0);
                break;
            } else if (pos > target_pos_to) {
                motor_R_run_distance(1);
            } else {
                set_target_speed(0, 0);
                break;
            }
        }
    } else {
        set_target_speed(0, 0);
    }
    PID_clear_A();
    PID_clear_B();
}

// 姿态调整测试
void posture_adjust_test(i16 t_f, i16 t_t) {
    uint8_t tmp = oled_disable_print;
    oled_disable_print = 0;
    posture_adjust_core(t_f, t_t);
    oled_disable_print = tmp;
}

struct line_patrol_output {
    float scale;
    bool stoped;
};

static constexpr uint32_t time_1s_us = 1000000;

enum line_patrol_dir {
    clockwise,        // 顺时针
    counter_clockwise // 逆时针

};

struct line_patrol_input {
    uint32_t starttime;
    int32_t start_distance_L; // 开始巡线时左轮的编码器值，注意abs后使用
    int32_t start_distance_R; // 开始巡线时右轮的编码器值，注意abs后使用
    line_patrol_dir dir;
};

// 巡线核心
// 输出量：scale
line_patrol_output line_patrol_core(line_patrol_input input) {
    line_patrol_output ret;
    ret.scale = 0;
    ret.stoped = false;
    constexpr i16 mid_pos = 7;
    // 传感器更新

    // 尽量让黑线在两个传感器的中间位置

    i16 blackline_pos1 = get_sensor1_blackline_pos();
    i16 blackline_pos2 = get_sensor2_blackline_pos();
    int32_t start_distance_inside, start_distance_outside;
    int32_t now_distance_inside, now_distance_outside;
    if (input.dir == counter_clockwise) {
        // 逆时针，左轮是内，右轮是外
        start_distance_inside = input.start_distance_L;
        start_distance_outside = input.start_distance_R;
        now_distance_inside = encoderA_get();
        now_distance_outside = encoderB_get();
    } else {
        // 顺时针，左轮是外，右轮是内
        start_distance_inside = input.start_distance_R;
        start_distance_outside = input.start_distance_L;
        now_distance_inside = encoderB_get();
        now_distance_outside = encoderA_get();
    }

    if (blackline_pos1 == -1) {
        // 未检测到黑线
        if (input.starttime + time_1s_us < sys_cur_tick_us
            && abs(now_distance_inside - start_distance_inside) > half_circle_inside_distance
            && abs(now_distance_outside - start_distance_outside) > half_circle_outside_distance) {
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


// 本来是16 10 6

// 将小车放在位置 A 点，小车能自动行驶到 B 点后，沿半弧线行驶到 C
// 点，再由 C 点自动行驶到 D 点，最后沿半弧线行驶到 A 点停车，每经过一个点，
// 声光提示一次。完成一圈用时不大于 30 秒。（

int prob2_distances_record[2][6] = {};

void go_problem2() {
    memset(prob2_distances_record, 0, sizeof(prob2_distances_record));
    prob2_distances_record[0][0] = encoderA_get();
    prob2_distances_record[1][0] = encoderB_get();
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
    prob2_distances_record[0][1] = encoderA_get();
    prob2_distances_record[1][1] = encoderB_get();

    // B-->C，寻迹
    oled_print(0, "B->C");
    OLED_Refresh();
    disable_acclimit();
    uint32_t start_time_B = sys_cur_tick_us;
    line_patrol_input input;
    input.starttime = start_time_B;
    input.start_distance_L = encoderA_get();
    input.start_distance_R = encoderB_get();
    input.dir = clockwise;
    while (true) {
        auto ret = line_patrol_core(input);
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
    delay_ms(500);
    prob2_distances_record[0][2] = encoderA_get();
    prob2_distances_record[1][2] = encoderB_get();

    // 向右微调一定编码器值
    // 左电机运动
    micro_adjust(RIGHT);
    // 调整角度，使车头指向D
    oled_print(0, "adjust direction C->D");
    OLED_Refresh();
    //posture_adjust_core(posture_tar_from_prob2, posture_tar_to_prob3);
    PID_clear_A();
    PID_clear_B();

    prob2_distances_record[0][3] = encoderA_get();
    prob2_distances_record[1][3] = encoderB_get();

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

    prob2_distances_record[0][4] = encoderA_get();
    prob2_distances_record[1][4] = encoderB_get();

    // D-->A，寻迹
    oled_print(0, "D->A");
    OLED_Refresh();
    disable_acclimit();
    uint32_t start_time_D = sys_cur_tick_us;
    line_patrol_input input2;
    input2.starttime = start_time_D;
    input2.start_distance_L = encoderA_get();
    input2.start_distance_R = encoderB_get();
    input2.dir = clockwise;
    while (true) {
        auto ret = line_patrol_core(input2);
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

    prob2_distances_record[0][5] = encoderA_get();
    prob2_distances_record[1][5] = encoderB_get();
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
    // C-->B 逆时针
    // 寻迹
    disable_acclimit();
    uint32_t start_time_C = sys_cur_tick_us;
    line_patrol_input input;
    input.starttime = start_time_C;
    input.start_distance_L = encoderA_get();
    input.start_distance_R = encoderB_get();
    input.dir = counter_clockwise;
    while (true) {
        constexpr int default_left_speed = inside_speed;
        constexpr int default_right_speed = outside_speed;
        auto ret = line_patrol_core(input);
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
    micro_adjust(LEFT);
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
    line_patrol_input input2;
    input2.starttime = start_time_D;
    input2.start_distance_L = encoderA_get();
    input2.start_distance_R = encoderB_get();
    input2.dir = clockwise;

    while (true) {
        constexpr int default_left_speed = outside_speed;
        constexpr int default_right_speed = inside_speed;
        auto ret = line_patrol_core(input2);
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
    micro_adjust(RIGHT);
    delay_ms(500);
}

uint16_t adjust_at_A_param = 260;
uint16_t adjust_at_B_param = 330;
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