#ifndef CONTROL_HPP
#define CONTROL_HPP
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
extern uint16_t time_adjust;
extern int16_t posture_tar_from_prob2;
extern int16_t posture_tar_to_prob3;
void go();
void go_problem1();
extern int prob2_distances_record[2][6];
void go_problem2();
void go_problem3();
void go_problem4();
void posture_adjust_test(int16_t target_pos_from, int16_t target_pos_to);
extern float weight_front;
extern float weight_mid;
extern uint16_t adjust_at_A_param, adjust_at_B_param;
extern uint16_t distance_adjust_after_leave_blackline;
extern uint16_t adjust_params[4][2];
#ifdef __cplusplus
}
#endif
#endif