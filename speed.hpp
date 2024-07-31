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

//使电机转动指定的距离，如果不打滑就一定是准的
void motor_L_run_distance(int distance);
void motor_R_run_distance(int distance);

#ifdef __cplusplus
}
#endif
#endif