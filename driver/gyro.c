#include "ti_msp_dl_config.h"
#include "./mpu6050/bsp_mpu6050.h"
#include "mpu6050/inv_mpu_dmp_motion_driver.h"
#include "driver/menu.hpp"
#include "driver/mpu6050/inv_mpu.h"
#include "gyro.h"
int16_t sum(int16_t *data) {
    int16_t res = 0;
    for (int16_t i = 0; i < 10; i++) {
        res += data[i];
    }
    return res;
}
float avg(int16_t res) {
    return res / 10;
}
float absf(float src) {
    if (src > 0) {
        return src;
    } else {
        return -src;
    }
}
short cp(float avg, int16_t *data) {
    for (int16_t i = 0; i < 10; i++) {
        if (absf(data[i] - avg) > 1) {
            return -1;
        }
    }
    return 1;
}
void gyroinit() {
    short gyroData[10];
    int16_t data[10];
    for (int16_t i = 0; i < 100; i++) {
        MPU6050ReadGyro(gyroData);
        data[i % 10] = gyroData[0];
        if (i >= 10) {
            if (cp(avg(sum(data)), data) == 1) {
                break;
            };
        }
    }
}

void mpu6050_prepare() {
    float p, r, y;
    p = -999;
    r = -999;
    y = -999;
    float start_y[10];
    while (true) {
        int maxindex = 0;
        int minindex = 0;
        uint8_t status;
        for (int i = 0; i < 10; i++) {
            status = mpu_dmp_get_data(&p, &r, &y);
            start_y[i] = y;
        }
        for (int j = 1; j < 10; j++) {
            if (start_y[minindex] > start_y[j]) {
                minindex = j;
            }
            if (start_y[maxindex] < start_y[j]) {
                maxindex = j;
            }
        }
        if (start_y[maxindex] - start_y[minindex] < 1) {
            break;
        }
    }
    for (int i = 0; i < 3; ++i) {
        mpu6050_updateYaw();
    }
}

float angle_sub(float a, float b) {
    float res = a - b;
    if (res > 180) {
        res -= 360;
    }
    if (res < -180) {
        res += 360;
    }
    return res;
}

float angle_add(float a, float b) {
    float res = a + b;
    if (res > 180) {
        res -= 360;
    }
    if (res < -180) {
        res += 360;
    }
    return res;
}

float system_yaw = 0;

//893us
void mpu6050_updateYaw() {
    static float yaw_buffer[3] = {};
    float p, r, y;
    p = -999;
    r = -999;
    y = -999;
    uint8_t status = mpu_dmp_get_data(&p, &r, &y);
    if (status == 0) {
        yaw_buffer[2] = yaw_buffer[1];
        yaw_buffer[1] = yaw_buffer[0];
        yaw_buffer[0] = y;
        system_yaw = (yaw_buffer[0] + yaw_buffer[1] + yaw_buffer[2]) / 3.0;
    }
}