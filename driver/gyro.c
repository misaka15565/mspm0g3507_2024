#include "ti_msp_dl_config.h"
#include "./mpu6050/bsp_mpu6050.h"
#include "driver/menu.hpp"
#include "gyro.h"
int16_t sum(int16_t *data){
    int16_t res = 0;
    for (int16_t i = 0 ; i<10 ; i++) {
        res+=data[i];
    }
    return res;
}
float avg(int16_t res){
    return res/10;
}
float absf(float src){
    if (src>0) {
        return src;
    }else{
        return -src;
    }
}
short cp(float avg, int16_t *data){
    for (int16_t i = 0; i<10; i++) {
        if (absf(data[i]-avg)>1) {
            return -1;
        }
    }
    return 1;
}
void gyroinit(){
    short gyroData[10];
    int16_t data[10];
    for (int16_t i = 0; i<100; i++) {
        MPU6050ReadGyro(gyroData);
        data[i % 10] = gyroData[0];
        if (i>=10) {
            if(cp(avg(sum(data)),data)==1){
                break;
            };
        }
    }    
}
