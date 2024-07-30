#include "delay.hpp"

#include "ti_msp_dl_config.h"
extern "C" {
volatile uint32_t sys_cur_tick_us=0;
void SysTick_Handler() {
    ++sys_cur_tick_us;
}
}

// systick被配置为1us计数一次
void delay_us(uint32_t us) {
    uint32_t target_time = sys_cur_tick_us + us;
    while (sys_cur_tick_us < target_time)
        ;
}
void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}