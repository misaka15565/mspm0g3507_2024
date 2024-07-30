#include "BEEP.h"

#include "ti_msp_dl_config.h"

static int beep_ms_count = 0;

void BEEP_ms(int m) {
    beep_ms_count = m;
    DL_GPIO_writePins(BEEP_PORT, BEEP_PIN_PIN | MOTOR_DIR_A2_PIN);
    DL_GPIO_writePins(LED_Board_PORT, LED_Board_LED0_PIN | MOTOR_DIR_B1_PIN);
    return;
    uint32_t res = DL_GPIO_readPins(BEEP_PORT, 0xffffffff);
    res |= BEEP_PIN_PIN;
    DL_GPIO_writePinsVal(BEEP_PORT, 0xffffffff, res);
    return;
    DL_GPIO_writePins(BEEP_PORT, BEEP_PIN_PIN);
    DL_GPIO_writePins(MOTOR_DIR_A2_PORT, MOTOR_DIR_A2_PIN);
    DL_GPIO_writePins(MOTOR_DIR_B1_PORT, MOTOR_DIR_B1_PIN);

    DL_GPIO_clearPins(MOTOR_DIR_A1_PORT, MOTOR_DIR_A1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_B2_PORT, MOTOR_DIR_B2_PIN);
}

void BEEP_IRQ() {
    if (beep_ms_count > 0) {
        --beep_ms_count;
        if (beep_ms_count == 0) {
            DL_GPIO_clearPins(BEEP_PORT, BEEP_PIN_PIN | MOTOR_DIR_B2_PIN);
            DL_GPIO_clearPins(LED_Board_PORT, LED_Board_LED0_PIN | MOTOR_DIR_A1_PIN);
            

            DL_GPIO_writePins(MOTOR_DIR_A2_PORT, MOTOR_DIR_A2_PIN);
            DL_GPIO_writePins(MOTOR_DIR_B1_PORT, MOTOR_DIR_B1_PIN);

            DL_GPIO_clearPins(MOTOR_DIR_A1_PORT, MOTOR_DIR_A1_PIN);
            DL_GPIO_clearPins(MOTOR_DIR_B2_PORT, MOTOR_DIR_B2_PIN);
        }
    }
}
