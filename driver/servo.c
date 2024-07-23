#include "servo.h"

#include "ti_msp_dl_config.h"

void setAngle(int angle) {
    // 0.5ms 0 0.5/20*10000=250
    // 2.5ms 180 1250
    int pwm = (float)angle / 180.0 * 1000.0 + 250;
    DL_TimerA_setCaptureCompareValue(PWM_SERVO_INST, pwm, DL_TIMER_CC_0_INDEX);
}