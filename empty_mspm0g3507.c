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
#include "driver/servo.h"
#include "driver/mpu6050/bsp_mpu6050.h"
#include "driver/mpu6050/inv_mpu_dmp_motion_driver.h"
#include "driver/mpu6050/inv_mpu.h"
#include "driver/oled.h"
#include "driver/menu.hpp"
#include "driver/key.hpp"
int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}

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
    main_menu_start();
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_0_INDEX); // right
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_1_INDEX); // left

    DL_TimerG_startCounter(PWM_MOTOR_INST);
    int angle = 0;
    uint32_t last_time = ((volatile SysTick_Type *)SysTick)->VAL;
    uint16_t dmp_try_count = 0;
    while (1) {
        uint32_t time_use = ((volatile SysTick_Type *)SysTick)->VAL;
        oled_print(4, "time=%d", time_use);
        angle = (angle + 1) % 360;
        setAngle(angle < 180 ? angle : 360 - angle);
        volatile float p, r, y;
        p = -999;
        r = -999;
        y = -999;
        uint8_t status = mpu_dmp_get_data(&p, &r, &y);
        ++dmp_try_count;
        oled_print(5, "dmp try %d", dmp_try_count);
        if (status == 0) {
            // printf("pitch %.2f roll %.2f yaw %.2f\n", p, r, y);
            oled_print(0, "pitch=%.2f", p);
            oled_print(1, "roll=%.2f", r);
            oled_print(2, "yaw=%.2f", y);
            oled_print(3, "wait=%d", i2c_waitcount);
            dmp_try_count = 0;
        }

        if (dmp_try_count > 100) {
            //陀螺仪挂了，死啦
        }

        int speed_B = motorB_getspeed();
        int speed_A = motorA_getspeed();
        int res_A = Velocity_A(15, speed_A);
        int res_B = Velocity_B(15, speed_B);
        res_A = range_protect(res_A, 0, 300);
        res_B = range_protect(res_B, 0, 300);
        Set_PWM(res_A, res_B);

        OLED_Refresh();
        continue;
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, res_B,
                                         DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, res_A,
                                         DL_TIMER_CC_1_INDEX);
    }
}

// 10ms
void TIMER_0_INST_IRQHandler(void) {
    // DL_GPIO_togglePins(GPIO_B_PORT, GPIO_B_LED2_GREEN_PIN);
    static int count = 0;
    ++count;
    if (count == 49) {
        // DL_GPIO_togglePins(GPIO_A_PORT, GPIO_A_LED1_PIN);
        count = 0;
    }
    update_speed_irq();
    key_IRQHandler();
}
