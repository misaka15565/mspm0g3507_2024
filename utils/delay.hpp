#ifndef DELAY_HPP
#define DELAY_HPP
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
void delay_ms(uint32_t);
void delay_us(uint32_t);
void delay_us_nointerrupt(uint32_t us);
extern volatile uint32_t sys_cur_tick_us;
#ifdef __cplusplus
}
#endif

#endif