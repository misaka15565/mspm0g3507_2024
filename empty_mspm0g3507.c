/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "driver/encoder.h"
#include "driver/motor.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include "driver/mpu6050/bsp_mpu6050.h"
#include "driver/mpu6050/inv_mpu_dmp_motion_driver.h"
#include "driver/mpu6050/inv_mpu.h"
#include "driver/oled.h"
#include "driver/menu.hpp"
#include "driver/key.hpp"
#include "control.hpp"
#include "driver/gray_sensor.h"
#include "utils/delay.hpp"
#include "speed.hpp"
#include "driver/BEEP.h"
#include "statemachine.hpp"
int main(void) {
    SYSCFG_DL_init();

    board_init();
    MPU6050_Init();
    while (mpu_dmp_init()) {
        printf("dmp error\r\n");
        delay_ms(200);
    }
    printf("Initialization Data Succeed \r\n");
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(10, 5, (uint8_t *)"hello", 1);
    OLED_Refresh();
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_0_INDEX); // right
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_1_INDEX); // left

    DL_TimerG_startCounter(PWM_MOTOR_INST);
    main_menu_start();
    OLED_Clear();
    OLED_Refresh();
    int angle = 0;
    uint32_t last_time = sys_cur_tick_us;
    uint16_t dmp_try_count = 0;
    float startyaw[50];
    float baseyaw = 0;
    delay_ms(2000); // 等一会，让人离开
    switch (now_problem) {
    case problem_empty:
        break;
    case problem_1:
        go_problem1();
        break;
    case problem_2:
        go_problem2();
        break;
    case problem_3:
        go_problem3();
        break;
    case problem_4:
        go_problem4();
        break;
    }
    set_target_speed(0, 0);
    oled_disable_print = 0;

    // 显示一些信息
    while (1) {
        uint32_t time_use = sys_cur_tick_us - last_time;
        last_time = sys_cur_tick_us;
        oled_print(7, "time=%d", time_use);
        oled_print(5, "pwm %d %d", res_A, res_B);
        float p, r, y;
        p = -999;
        r = -999;
        y = -999;
        // 注意：即使status为0，也不代表读到的数据是正确的

        uint8_t status = mpu_dmp_get_data(&p, &r, &y);
        ++dmp_try_count;
        oled_print(2, "dmp try %d", dmp_try_count);
        if (status == 0) {
            // printf("pitch %.2f roll %.2f yaw %.2f\n", p, r, y);
            oled_print(0, "pitch=%.2f roll=%.2f", p, r);
            oled_print(1, "yaw=%.2f wait=%d", y, i2c_waitcount);
            dmp_try_count = 0;
        }
        if (dmp_try_count > 100) {
            // 陀螺仪挂了，死啦
        }
        oled_print(3, "sensor 1:%02X 2:%02X", (int32_t)sensor1_res, (int32_t)sensor2_res);
        oled_print(4, "speed A=%d B=%d", motorA_getspeed(), motorB_getspeed());

        // 控制部分
        gw_gray_serial_read();
        // go();
        static uint16_t oled_count = 0;
        OLED_Refresh();
    }
}

// 1ms
void TIMER_0_INST_IRQHandler(void) {
    // DL_GPIO_togglePins(GPIO_B_PORT, GPIO_B_LED2_GREEN_PIN);
    static int count = 0;
    BEEP_IRQ();
    update_speed_irq();
    speed_pid_irqHandler();
    ++count;
    if (count == 10) {
        key_IRQHandler();
        count = 0;
    }
}
