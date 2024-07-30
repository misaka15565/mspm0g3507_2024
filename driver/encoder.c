#include "encoder.h"
#include "stdint.h"
#include "ti_msp_dl_config.h"

uint32_t gpio_interrup;
volatile int32_t Get_Encoder_count2 = 0;
volatile int32_t Get_Encoder_count1 = 0;
// 使用我司D157B驱动模块时，E1A连接PA16、E1B连接PA15、E2A连接PA17，E2B连接PA22

void GROUP1_IRQHandler(void) {
  // 使用我司D157B驱动模块时，E1A连接PA16、E1B连接PA15、E2A连接PA17，E2B连接PA22
  // 获取中断信号
  gpio_interrup = DL_GPIO_getEnabledInterruptStatus(
      GPIOA, ENCODER_A_E1A_PIN | ENCODER_A_E1B_PIN | ENCODER_B_E2A_PIN |
                 ENCODER_B_E2B_PIN);
  // encoderA
  if ((gpio_interrup & ENCODER_A_E1A_PIN) == ENCODER_A_E1A_PIN) {
    if (!DL_GPIO_readPins(GPIOA, ENCODER_A_E1B_PIN)) {
      Get_Encoder_count1--;
    } else {
      Get_Encoder_count1++;
    }
  } else if ((gpio_interrup & ENCODER_A_E1B_PIN) == ENCODER_A_E1B_PIN) {
    if (!DL_GPIO_readPins(GPIOA, ENCODER_A_E1A_PIN)) {
      Get_Encoder_count1++;
    } else {
      Get_Encoder_count1--;
    }
  }
  // encoderB
  if ((gpio_interrup & ENCODER_B_E2A_PIN) == ENCODER_B_E2A_PIN) {
    if (!DL_GPIO_readPins(GPIOA, ENCODER_B_E2B_PIN)) {
      Get_Encoder_count2--;
    } else {
      Get_Encoder_count2++;
    }
  } else if ((gpio_interrup & ENCODER_B_E2B_PIN) == ENCODER_B_E2B_PIN) {
    if (!DL_GPIO_readPins(GPIOA, ENCODER_B_E2A_PIN)) {
      Get_Encoder_count2++;
    } else {
      Get_Encoder_count2--;
    }
  }
  DL_GPIO_clearInterruptStatus(GPIOA, ENCODER_A_E1A_PIN | ENCODER_A_E1B_PIN |
                                          ENCODER_B_E2A_PIN |
                                          ENCODER_B_E2B_PIN);
}

int32_t encoderB_get() { return Get_Encoder_count1; }
int32_t encoderA_get() { return Get_Encoder_count2; }
void encoderB_clear() { Get_Encoder_count1 = 0; }
void encoderA_clear() { Get_Encoder_count2 = 0; }