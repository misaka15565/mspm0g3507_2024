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

#include "driver/delay.h"
#include "driver/encoder.h"
#include "driver/lcd.h"
#include "driver/motor.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
int fputc(int _c, FILE *f) {
    while (DL_UART_Main_isBusy(UART_0_INST))
        ;
    DL_UART_Main_transmitData(UART_0_INST, _c);
}
int putc(int _x, FILE *_fp) {
    fputc(_x, _fp);
    return 0;
}
int putchar(int data) {
    fputc(data, NULL);
    return 0;
}
int fputs(const char *_p, FILE *f) {
    while (*_p != 0) {
        fputc(*_p, f);
        ++_p;
    }
    return 0;
}

int range_protect(int x, int low, int high) {
    return x < low ? low : (x > high ? high : x);
}

int main(void) {
    SYSCFG_DL_init();
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    //NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
    printf(__TIME__);
    LCD_Init();
    LCD_Clear(0xf800);
    for (int x = 0; x < 100; ++x) {
        for (int y = 0; y < 200; ++y) {
            LCD_DrawPoint(x, y);
        }
    }
    LCD_Show_String(0, 0, __TIME__);
    
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_0_INDEX); // right
    DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 0,
                                     DL_TIMER_CC_1_INDEX); // left

    DL_TimerG_startCounter(PWM_MOTOR_INST);
    while (1) {
        // DL_UART_Main_transmitData(UART_0_INST, 'c');
        delay_cycles(18000000);
        // printf("encoder %d %d\n", encoderB_get(), encoderA_get()); // left right
        int speed_l = getspeed_left();
        int speed_r = getspeed_right();
        printf("speed %d %d\n", speed_l, speed_r);

        int res_l = Velocity_A(15, speed_l);
        int res_r = Velocity_B(15, speed_r);
        res_l = range_protect(res_l, 0, 300);
        res_r = range_protect(res_r, 0, 300);
        printf("pwm %d %d\n", res_l, res_r);
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, res_r,
                                         DL_TIMER_CC_0_INDEX);
        DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, res_l,
                                         DL_TIMER_CC_1_INDEX);
    }
}

// 10ms
void TIMER_0_INST_IRQHandler(void) {
    // DL_GPIO_togglePins(GPIO_B_PORT, GPIO_B_LED2_GREEN_PIN);
    static int count = 0;
    ++count;
    if (count == 99) {
        DL_GPIO_togglePins(GPIO_A_PORT, GPIO_A_LED1_PIN);
        count = 0;
    }
    //update_speed_irq();
}
