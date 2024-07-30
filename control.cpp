#include "control.hpp"
extern "C" {
#include "driver/motor.h"
#include "driver/gray_sensor.h"
}

// sensor1是前面的
int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}
// 0-7 --> 1-8
#define sensor1_get(x) GET_NTH_BIT(sensor1_res, x + 1)

//对于传感器2（后面那个），8号检测在车的左后方，给转换成0号，这样和前面是统一的
// 0-7 --> 8-1
#define sensor2_get(x) GET_NTH_BIT(sensor2_res, 8 - x);

using u16 = uint16_t;
using u8 = uint8_t;
void go() {
    // 计算前进速度（可以pid可以固定
    int default_left_speed = 47; // 顺时针跑的时候
    int default_right_speed = 33;

    int offset_left_speed = 0;
    int offset_right_speed = 0;

    // 先获取黑线在哪，某位为0则说明是黑的
    uint8_t blacks[8];
    uint8_t blacks_count = 0;
    uint16_t sum;
    for (int i = 0; i < 8; ++i) {
        if (sensor1_get(i) == 0) {
            blacks[blacks_count] = i * 2;
            ++blacks_count;
            sum += i * 2;
        }
    }

    if (blacks_count == 0) {
        // 未检测到黑线
        // TODO
    } else {
        u16 black_pos = sum / blacks_count;
        if (black_pos < 9) {
            // 若黑线在9号位置是中间，小于9说明黑线在左边
            offset_left_speed = -10;
        } else if (black_pos > 9) {
            offset_left_speed = +5;
        }
    }

    int target_left_speed = default_left_speed + offset_left_speed;
    int target_right_speed = default_right_speed + offset_right_speed;

    // 得出目标速度
    int speed_A = motorA_getspeed();
    int speed_B = motorB_getspeed();

    int res_A = Velocity_A(target_left_speed, speed_A);
    int res_B = Velocity_B(target_right_speed, speed_B);
    res_A = range_protect(res_A, 0, 1000);
    res_B = range_protect(res_B, 0, 1000);
    Set_PWM(res_A, res_B);
}