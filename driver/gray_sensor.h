/*
 * Copyright (c) 2022 感为智能科技(济南)
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 */

#ifndef INC_GW_GRAYSCALE_SENSOR_H_
#define INC_GW_GRAYSCALE_SENSOR_H_
#include "stdint.h"
void gw_gray_serial_read();
extern volatile uint8_t sensor1_res;
extern volatile uint8_t sensor2_res;

/**
 * @brief 从I2C得到的8位的数字信号的数据 读取第n位的数据
 * @param sensor_value_8 数字IO的数据
 * @param n 第1位从1开始, n=1 是传感器的第一个探头数据, n=8是最后一个
 * @return
 */
#define GET_NTH_BIT(sensor_value, nth_bit) (((sensor_value) >> ((nth_bit)-1)) & 0x01)

#endif /* INC_GW_GRAYSCALE_SENSOR_H_ */
