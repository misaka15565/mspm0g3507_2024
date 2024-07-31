#ifndef _GYRO_H
#define _GYRO_H

void gyroinit();
void mpu6050_prepare();
float angle_sub(float a, float b);
float angle_add(float a, float b);
extern float system_yaw;
void mpu6050_updateYaw();//注意：从mpu读取数据的函数不能在中断中调用，因为没有抢占优先级

#endif