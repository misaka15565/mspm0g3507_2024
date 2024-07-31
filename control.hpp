#ifndef CONTROL_HPP
#define CONTROL_HPP
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
extern uint16_t time_adjust;
void go();
void go_problem1();
void go_problem2();
void go_problem3();
void go_problem4();
extern float weight_front;
extern float weight_mid;
#ifdef __cplusplus
}
#endif
#endif