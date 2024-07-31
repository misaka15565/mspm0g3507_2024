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
}

float mpu60550_JDX(float ny,float ty)//ny为当前偏航角，ty为目标偏航角
{
    float zj = ny - ty;
    if(zj < -180)
    {
        zj = 360 + zj
    }
    else if(zj > 180)
    {
        zj = -(360 - zj);
    }
    return zj;
}
