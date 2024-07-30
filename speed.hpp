#ifndef SPEED_HPP
#define SPEED_HPP

#ifdef __cplusplus
extern "C" {
#endif
extern int res_A, res_B;
#include <stdint.h>

void set_target_speed(int, int);
void speed_pid_irqHandler();

#ifdef __cplusplus
}
#endif
#endif