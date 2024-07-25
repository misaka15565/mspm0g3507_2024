/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-05-28     LCKFB-LP    first version
 */

#include "mpu6050.h"
#include "stdio.h"
#include "lcd.h"
#include "ti_msp_dl_config.h"

enum I2cControllerStatus {
    I2C_STATUS_IDLE = 0,
    I2C_STATUS_TX_STARTED,
    I2C_STATUS_TX_INPROGRESS,
    I2C_STATUS_TX_COMPLETE,
    I2C_STATUS_RX_STARTED,
    I2C_STATUS_RX_INPROGRESS,
    I2C_STATUS_RX_COMPLETE,
    I2C_STATUS_ERROR,
} gI2cControllerStatus;

uint32_t gTxLen, gTxCount, gRxCount, gRxLen;
uint8_t gTxPacket[128];
uint8_t gRxPacket[128];

static void softi2c_sda_out() {
    DL_GPIO_initDigitalOutput(MPU6050_SDA_IOMUX);
    DL_GPIO_setPins(MPU6050_PORT, MPU6050_SDA_PIN);
    DL_GPIO_enableOutput(MPU6050_PORT, MPU6050_SDA_PIN);
}

static void softi2c_sda_in() {
    DL_GPIO_initDigitalInputFeatures(MPU6050_SDA_IOMUX,
                                     DL_GPIO_INVERSION_DISABLE,
                                     DL_GPIO_RESISTOR_PULL_UP,
                                     DL_GPIO_HYSTERESIS_DISABLE,
                                     DL_GPIO_WAKEUP_DISABLE);
}
void delay_us(int us) {
    while (us--) delay_cycles(CPUCLK_FREQ / 1000000);
}
static softi2c_write_scl(uint8_t bit) {
    if (bit) {
        DL_GPIO_setPins(MPU6050_PORT, MPU6050_SCL_PIN);
    } else {
        DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SCL_PIN);
    }
    delay_us(8);
}
static void softi2c_write_sda(uint8_t bit) {
    softi2c_sda_out();
    if (bit) {
        DL_GPIO_setPins(MPU6050_PORT, MPU6050_SDA_PIN);
    } else {
        DL_GPIO_clearPins(MPU6050_PORT, MPU6050_SDA_PIN);
    }
    delay_us(8);
}
static uint8_t softi2c_read_sda(void) {
    uint8_t b;
    uint32_t bitval;
    softi2c_sda_in();
    bitval = DL_GPIO_readPins(MPU6050_PORT, MPU6050_SDA_PIN);
    if (bitval)
        b = 1;
    else
        b = 0;
    delay_us(8);
    return b;
}
static void softi2c_Start(void) {
    softi2c_sda_out();
    softi2c_write_sda(1);
    softi2c_write_scl(1);
    softi2c_write_sda(0);
    softi2c_write_scl(0);
}

static void softi2c_Stop(void) {
    softi2c_sda_out();
    softi2c_write_sda(0);
    softi2c_write_scl(1);
    softi2c_write_sda(1);
}
static void softi2c_SendByte(uint8_t byte) {
    softi2c_sda_out();
    for (int i = 0; i < 8; ++i) {
        softi2c_write_sda(byte & (0x80 >> i));
        softi2c_write_scl(1);
        softi2c_write_scl(0);
    }
}

static uint8_t softi2c_ReceiveByte() {
    softi2c_sda_out();
    uint8_t byte = 0;
    softi2c_write_sda(1);
    for (int i = 0; i < 8; ++i) {
        softi2c_sda_in();
        softi2c_write_scl(1);
        if (softi2c_read_sda() == 1) {
            byte |= (0x80 >> i);
        }
        softi2c_write_scl(0);
    }
    return byte;
}

static void softi2c_SendAck(uint8_t AckBit) {
    softi2c_sda_out();
    softi2c_write_sda(AckBit);
    softi2c_write_scl(1);
    softi2c_write_scl(0);
}

static uint8_t softi2c_ReceiveAck(void) {
    softi2c_sda_out();
    uint8_t AckBit = 0;
    softi2c_write_sda(1);
    softi2c_write_scl(1);
    softi2c_sda_in();
    AckBit = softi2c_read_sda();
    softi2c_write_scl(0);
    return AckBit;
}
#define MPU6050_ADDRESS 0xD0 // MPU6050的I2C从机地址

char MPU6050_WriteReg(uint8_t regaddr, uint8_t data) {
    softi2c_Start();
    softi2c_SendByte(MPU6050_ADDRESS);
    softi2c_ReceiveAck();
    softi2c_SendByte(regaddr << 1);
    softi2c_ReceiveAck();
    softi2c_SendByte(data);
    softi2c_ReceiveAck();
    softi2c_Stop();
}

uint8_t MPU6050_ReadReg(uint8_t regaddr) {
    uint8_t data;
    softi2c_Start();
    softi2c_SendByte(MPU6050_ADDRESS);
    softi2c_ReceiveAck();
    softi2c_SendByte(regaddr);
    softi2c_ReceiveAck();
    softi2c_Start();
    softi2c_SendByte(MPU6050_ADDRESS | 0x01);
    softi2c_ReceiveAck();
    data = softi2c_ReceiveByte();
    softi2c_SendAck(1);
    softi2c_Stop();
    return data;
}

#define MPU6050_SMPLRT_DIV 0x19
#define MPU6050_CONFIG 0x1A
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACCEL_CONFIG 0x1C

#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_YOUT_L 0x3E
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_ACCEL_ZOUT_L 0x40
#define MPU6050_TEMP_OUT_H 0x41
#define MPU6050_TEMP_OUT_L 0x42
#define MPU6050_GYRO_XOUT_H 0x43
#define MPU6050_GYRO_XOUT_L 0x44
#define MPU6050_GYRO_YOUT_H 0x45
#define MPU6050_GYRO_YOUT_L 0x46
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_GYRO_ZOUT_L 0x48

#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_PWR_MGMT_2 0x6C
#define MPU6050_WHO_AM_I 0x75

void MPU6050_Init(void) {
    // softi2c_init();									//先初始化底层的I2C
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x80);
    delay_us(100 * 1000);
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x00);
    /*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);   // 电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);   // 电源管理寄存器2，保持默认值0，所有轴均不待机
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);   // 采样率分频寄存器，配置采样率
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);       // 配置寄存器，配置DLPF
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);  // 陀螺仪配置寄存器，选择满量程为±2000°/s
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18); // 加速度计配置寄存器，选择满量程为±16g
}

/**
 * 函    数：MPU6050获取ID号
 * 参    数：无
 * 返 回 值：MPU6050的ID号
 */
uint8_t MPU6050_GetID(void) {
    return MPU6050_ReadReg(MPU6050_WHO_AM_I << 1); // 返回WHO_AM_I寄存器的值
}

/**
 * 函    数：MPU6050获取数据
 * 参    数：AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
 * 参    数：GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
 * 返 回 值：无
 */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ) {
    uint8_t DataH, DataL; // 定义数据高8位和低8位的变量

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H); // 读取加速度计X轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L); // 读取加速度计X轴的低8位数据
    *AccX = (DataH << 8) | DataL;                  // 数据拼接，通过输出参数返回

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H); // 读取加速度计Y轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L); // 读取加速度计Y轴的低8位数据
    *AccY = (DataH << 8) | DataL;                  // 数据拼接，通过输出参数返回

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H); // 读取加速度计Z轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L); // 读取加速度计Z轴的低8位数据
    *AccZ = (DataH << 8) | DataL;                  // 数据拼接，通过输出参数返回

    DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H); // 读取陀螺仪X轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L); // 读取陀螺仪X轴的低8位数据
    *GyroX = (DataH << 8) | DataL;                // 数据拼接，通过输出参数返回

    DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H); // 读取陀螺仪Y轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L); // 读取陀螺仪Y轴的低8位数据
    *GyroY = (DataH << 8) | DataL;                // 数据拼接，通过输出参数返回

    DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H); // 读取陀螺仪Z轴的高8位数据
    DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L); // 读取陀螺仪Z轴的低8位数据
    *GyroZ = (DataH << 8) | DataL;                // 数据拼接，通过输出参数返回
}
