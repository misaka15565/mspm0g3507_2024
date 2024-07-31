#ifndef SPEED_HPP
#define SPEED_HPP

#ifdef __cplusplus
extern "C" {
#endif
extern int res_A, res_B;
#include <stdint.h>

void set_target_speed(int, int);
void speed_pid_irqHandler();
void enable_acclimit();
void disable_acclimit();

#ifdef __cplusplus
}
#endif
#endif