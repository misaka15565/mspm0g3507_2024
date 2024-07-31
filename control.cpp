#include "control.hpp"
extern "C" {
uint16_t time_adjust = 500;
#include "driver/motor.h"
#include "driver/gray_sensor.h"
#include "driver/oled.h"
#include "driver/BEEP.h"
#include "driver/gyro.h"
#include "stdlib.h"
}
#include "ti_msp_dl_config.h"
#include "statemachine.hpp"
#include "utils/delay.hpp"
#include "speed.hpp"
#include <cmath>
// sensor1是前面的

// 0-7 --> 1-8
#define sensor1_get(x) GET_NTH_BIT(sensor1_res, x + 1)

// 对于传感器2（后面那个），8号检测在车的左后方，给转换成0号，这样和前面是统一的
//  0-7 --> 8-1
#define sensor2_get(x) GET_NTH_BIT(sensor2_res, 8 - x)
#define S__BKPT()
using u16 = uint16_t;
using u8 = uint8_t;
using i16 = int16_t;
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

// 从A到B，到B停车并声光提示，用时不大于15s
// 注意：车安放时前传感器下没黑线
void go_problem1() {
    constexpr u16 speed_default = 15;
    // 记录当前时间
    uint32_t start_time = sys_cur_tick_us;
    enable_acclimit();
    while (true) {
        // 传感器更新
        gw_gray_serial_read();

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

// 使用陀螺仪进行第二题的控制，放下车时一定要准
void go_problem2_mpu() {
    mpu6050_prepare();
    constexpr u16 default_mid_speed = 15; // 默认直行速度
    constexpr u16 default_left_speed = 15;
    constexpr u16 default_right_speed = 10;
    constexpr u16 offset_speed = 6;
    float scale = 0;
    mpu6050_updateYaw();
    const float origin_yaw = system_yaw;
    // A-->B
    oled_print(0, "A->B");
    oled_print(1, "origin_yaw=%f", origin_yaw);
    OLED_Refresh();
    delay_ms(5000);
    enable_acclimit();
    uint32_t start_time_A = sys_cur_tick_us;
    while (true) {
        // 传感器更新
        gw_gray_serial_read();
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
    while (true) {
        float scale = 0;
        // 传感器更新
        gw_gray_serial_read();
        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        i16 blackline_pos2 = get_sensor2_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 可能已经到达了C点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            // 检测到黑线
            // 根据黑线位置调整车的状态
            // 黑线在中间则位置是7
            scale = (blackline_pos1 - 7) * 0.2;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
    // 调整角度，使车头指向D
    oled_print(0, "adjust direction C->D");
    OLED_Refresh();
    while (true) {
        // 更新传感器
        mpu6050_updateYaw();

        // 判断
        const float target_angle = angle_add(180.0, origin_yaw);
        oled_print(0, "yaw=%f", system_yaw);
        oled_print(1, "target yaw=%f", target_angle);
        oled_print(2, "origin_yaw=%f", origin_yaw);
        OLED_Refresh();
        if (fabsf(angle_sub(system_yaw, target_angle)) < 1) {
            // 调整完成
            delay_ms(5000);
            break;
        } else {
            // 调整
            if (angle_sub(system_yaw, target_angle) > 0) {
                // 右转
                set_target_speed(1, 0);
            } else {
                // 左转
                set_target_speed(0, 1);
            }
        }
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
        gw_gray_serial_read();
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
    while (true) {
        float scale = 0;
        // 传感器更新
        gw_gray_serial_read();
        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        i16 blackline_pos2 = get_sensor2_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 可能已经到达了A点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            // 检测到黑线
            // 根据黑线位置调整车的状态
            // 黑线在中间则位置是7
            scale = (blackline_pos1 - 7) * 0.2;

            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
}

// 将小车放在位置 A 点，小车能自动行驶到 B 点后，沿半弧线行驶到 C
// 点，再由 C 点自动行驶到 D 点，最后沿半弧线行驶到 A 点停车，每经过一个点，
// 声光提示一次。完成一圈用时不大于 30 秒。（
void go_problem2() {
    // go_problem2_mpu();
    // return;
    constexpr u16 default_mid_speed = 15; // 默认直行速度
    constexpr u16 default_left_speed = 15;
    constexpr u16 default_right_speed = 10;
    constexpr u16 offset_speed = 6;
    float scale = 0;
    // mpu6050_prepare();
    // mpu6050_updateYaw();
    const float origin_yaw = system_yaw;
    // A-->B
    uint32_t start_time_A = sys_cur_tick_us;
    oled_print(0, "A->B");
    OLED_Refresh();
    enable_acclimit();
    while (true) {
        // 传感器更新
        gw_gray_serial_read();
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
    while (true) {
        float scale = 0;
        // 传感器更新
        gw_gray_serial_read();
        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        i16 blackline_pos2 = get_sensor2_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 可能已经到达了C点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            // 检测到黑线
            // 根据黑线位置调整车的状态
            // 黑线在中间则位置是7
            scale = (blackline_pos1 - 7) * 0.17;
            // 希望后传感器也是黑线在中间
            /*
            if (blackline_pos2 < 7) {
                scale += -0.25;
            } else if (blackline_pos2 > 7) {
                scale += 0.25;
            } else {
                scale += 0;
            }
*/
            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
    // 调整角度，使车头指向D
    oled_print(0, "adjust direction C->D");
    OLED_Refresh();
    i16 last_blackline_pos2 = -1;
    char last_run_motor = 'x';
    while (true) {
        // 更新传感器
        gw_gray_serial_read();
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
        } else if (blackline_pos2 == 12) {
            // 进入下个阶段
            set_target_speed(0, 0);
            break;
        } else if (blackline_pos2 < 12) {
            motor_L_run_distance(1);
            last_run_motor = 'L';
        } else {
            motor_R_run_distance(1);
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
        gw_gray_serial_read();
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
    while (true) {
        float scale = 0;
        // 传感器更新
        gw_gray_serial_read();
        // 判断
        // 传感器1检测黑线的位置
        i16 blackline_pos1 = get_sensor1_blackline_pos();
        i16 blackline_pos2 = get_sensor2_blackline_pos();
        if (blackline_pos1 == -1) {
            // 未检测到黑线
            // 可能已经到达了A点
            // 先停车
            set_target_speed(0, 0);
            BEEP_ms(1000);
            break;
        } else {
            // 检测到黑线
            // 根据黑线位置调整车的状态
            // 黑线在中间则位置是7
            scale = (blackline_pos1 - 7) * 0.2;
            // 希望后传感器也是黑线在中间
            /*
            if (blackline_pos2 < 7) {
                scale += -0.25;
            } else if (blackline_pos2 > 7) {
                scale += 0.25;
            } else {
                scale += 0;
            }*/
            u16 target_left_speed = default_left_speed + offset_speed * scale;
            u16 target_right_speed = default_right_speed - offset_speed * scale;
            set_target_speed(target_left_speed, target_right_speed);
        }
    }
}
void go_problem3() {
    mpu6050_prepare();
}
void go_problem4() {
    mpu6050_prepare();
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