/*
 * 立创开发板软硬件资料与相关扩展板软硬件资料官网全部开源
 * 开发板官网：www.lckfb.com
 * 技术支持常驻论坛，任何技术问题欢迎随时交流学习
 * 立创论坛：https://oshwhub.com/forum
 * 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
 * 不靠卖板赚钱，以培养中国工程师为己任
 * Change Logs:
 * Date           Author       Notes
 * 2024-05-23     LCKFB     first version
 */
#include "board.h"
#include "stdio.h"
#include "string.h"
#include "utils/delay.hpp"
volatile unsigned int delay_times = 0;
volatile unsigned char uart_data = 0;

void board_init(void) {
    // SYSCFG_DL_init();

    NVIC_EnableIRQ(I2C_MPU6050_INST_INT_IRQN);
}
