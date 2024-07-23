#ifndef _ENCODER_H
#define _ENCODER_H
#include <stdint.h>
// 使用我司D157B驱动模块时，E1A连接PA16、E1B连接PA15、E2A连接PA17，E2B连接PA22

int32_t encoderA_get();
int32_t encoderB_get();
void encoderA_clear();
void encoderB_clear();
#endif