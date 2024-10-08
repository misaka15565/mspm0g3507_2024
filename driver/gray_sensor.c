#include <stdint.h>
#include "gray_sensor.h"
#include "ti_msp_dl_config.h"
#include "utils/delay.hpp"
/* 默认地址 */
#define GW_GRAY_ADDR_DEF 0x4C
#define GW_GRAY_PING 0xAA
#define GW_GRAY_PING_OK 0x66
#define GW_GRAY_PING_RSP GW_GRAY_PING_OK

/* 开启开关数据模式 */
#define GW_GRAY_DIGITAL_MODE 0xDD

/* 开启连续读取模拟数据模式 */
#define GW_GRAY_ANALOG_BASE_ 0xB0
#define GW_GRAY_ANALOG_MODE (GW_GRAY_ANALOG_BASE_ + 0)

/* 传感器归一化寄存器(v3.6及之后的固件) */
#define GW_GRAY_ANALOG_NORMALIZE 0xCF

/* 循环读取单个探头模拟数据 n从1开始到8 */
#define GW_GRAY_ANALOG(n) (GW_GRAY_ANALOG_BASE_ + (n))

/* 黑色滞回比较参数操作 */
#define GW_GRAY_CALIBRATION_BLACK 0xD0
/* 白色滞回比较参数操作 */
#define GW_GRAY_CALIBRATION_WHITE 0xD1

// 设置所需探头的模拟信号(CE: channel enable)
#define GW_GRAY_ANALOG_CHANNEL_ENABLE 0xCE
#define GW_GRAY_ANALOG_CH_EN_1 (0x1 << 0)
#define GW_GRAY_ANALOG_CH_EN_2 (0x1 << 1)
#define GW_GRAY_ANALOG_CH_EN_3 (0x1 << 2)
#define GW_GRAY_ANALOG_CH_EN_4 (0x1 << 3)
#define GW_GRAY_ANALOG_CH_EN_5 (0x1 << 4)
#define GW_GRAY_ANALOG_CH_EN_6 (0x1 << 5)
#define GW_GRAY_ANALOG_CH_EN_7 (0x1 << 6)
#define GW_GRAY_ANALOG_CH_EN_8 (0x1 << 7)
#define GW_GRAY_ANALOG_CH_EN_ALL (0xFF)

/* 读取错误信息 */
#define GW_GRAY_ERROR 0xDE

/* 设备软件重启 */
#define GW_GRAY_REBOOT 0xC0

/* 读取固件版本号 */
#define GW_GRAY_FIRMWARE 0xC1

/**
 * @brief 从一个变量分离出所有的bit
 */
#define SEP_ALL_BIT8(sensor_value, val1, val2, val3, val4, val5, val6, val7, val8) \
    do {                                                                           \
        val1 = GET_NTH_BIT(sensor_value, 1);                                       \
        val2 = GET_NTH_BIT(sensor_value, 2);                                       \
        val3 = GET_NTH_BIT(sensor_value, 3);                                       \
        val4 = GET_NTH_BIT(sensor_value, 4);                                       \
        val5 = GET_NTH_BIT(sensor_value, 5);                                       \
        val6 = GET_NTH_BIT(sensor_value, 6);                                       \
        val7 = GET_NTH_BIT(sensor_value, 7);                                       \
        val8 = GET_NTH_BIT(sensor_value, 8);                                       \
    } while (0)

/* 设置设备I2C地址 */
#define GW_GRAY_CHANGE_ADDR 0xAD

/* 广播重置地址所需要发的数据 */
#define GW_GRAY_BROADCAST_RESET "\xB8\xD0\xCE\xAA\xBF\xC6\xBC\xBC"
volatile uint8_t sensor1_res = 0; // 每bit 8 7 6 5 4 3 2 1号的数据
volatile uint8_t sensor2_res = 0;

volatile uint8_t sensor1_res_flitered = 0;
volatile uint8_t sensor2_res_flitered = 0;

static void gw_gray_serial_read() {
    uint8_t ret1 = 0;
    uint8_t ret2 = 0;

    for (int i = 0; i < 8; ++i) {
        /* 输出时钟下降沿 */
        // HAL_GPIO_WritePin(GW_GRAY_SERIAL_GPIO_GROUP, GW_GRAY_SERIAL_GPIO_CLK, 0);
        DL_GPIO_clearPins(GRAY_SENSOR_1_PORT, GRAY_SENSOR_1_CLK_1_PIN);
        DL_GPIO_clearPins(GRAY_SENSOR_2_PORT, GRAY_SENSOR_2_CLK_2_PIN);
        delay_us_nointerrupt(5); // 外部有上拉源(大约10k电阻) 可不加此行

        ret1 |= (DL_GPIO_readPins(GRAY_SENSOR_1_PORT, GRAY_SENSOR_1_DAT_1_PIN) == 0 ? 0 : 1) << i;
        ret2 |= (DL_GPIO_readPins(GRAY_SENSOR_2_PORT, GRAY_SENSOR_2_DAT_2_PIN) == 0 ? 0 : 1) << i;

        /* 输出时钟上升沿,让传感器更新数据*/
        DL_GPIO_setPins(GRAY_SENSOR_1_PORT, GRAY_SENSOR_1_CLK_1_PIN);
        DL_GPIO_setPins(GRAY_SENSOR_2_PORT, GRAY_SENSOR_2_CLK_2_PIN);
        /* 主控频率高的需要给一点点延迟,延迟需要在5us左右 */
        delay_us_nointerrupt(5);
    }

    sensor1_res = ret1;
    sensor2_res = ret2;
}
// sensor1是前面的

// 0-7 --> 1-8
#define sensor1_get(x) GET_NTH_BIT(sensor1_res, x + 1)

// 对于传感器2（后面那个），8号检测在车的左后方，给转换成0号，这样和前面是统一的
//  0-7 --> 1-8
#define sensor2_get(x) GET_NTH_BIT(sensor2_res, x + 1)

#define sensor_data_get(data, x) GET_NTH_BIT(data, x + 1)
// 在定时器中断中运行
void gw_gray_update_irq() {
    gw_gray_serial_read();
    // 如果近11次中有1次及以上为黑色，则认为是黑色
    // 记录近11次中，各bit有几次是黑色
    // 黑色是0
    static int8_t sensor1_last10_isblack_count[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    static int8_t sensor2_last10_isblack_count[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    static uint8_t sensor1_last10_record[10] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    static uint8_t sensor2_last10_record[10] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    // 记录了前十次拿到的原始数据
    static uint8_t record_index = 0;

    // 更新数据
    for (int i = 0; i < 8; ++i) {
        if (sensor1_get(i) == 0) {
            sensor1_last10_isblack_count[i]++;
        }
        if (sensor2_get(i) == 0) {
            sensor2_last10_isblack_count[i]++;
        }
    }

    // 拼接过滤后数据
    sensor1_res_flitered = 0;
    sensor2_res_flitered = 0;
    for (int i = 0; i < 8; ++i) {
        if (sensor1_last10_isblack_count[i] < 1) {
            sensor1_res_flitered |= 1 << i;
        }
        if (sensor2_last10_isblack_count[i] < 1) {
            sensor2_res_flitered |= 1 << i;
        }
    }

    // 数据出队
    for (int i = 0; i < 8; ++i) {
        if (sensor_data_get(sensor1_last10_record[record_index], i) == 0) {
            sensor1_last10_isblack_count[i]--;
        }
        if (sensor_data_get(sensor2_last10_record[record_index], i) == 0) {
            sensor2_last10_isblack_count[i]--;
        }
    }
    // 数据入队
    sensor1_last10_record[record_index] = sensor1_res;
    sensor2_last10_record[record_index] = sensor2_res;

    record_index = (1 + record_index) % 10;
}