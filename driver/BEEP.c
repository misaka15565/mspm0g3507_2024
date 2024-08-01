#include "BEEP.h"

#include "ti_msp_dl_config.h"

static int beep_ms_count = 0;

void BEEP_ms(int m) {
    beep_ms_count = m;
    DL_GPIO_writePins(BEEP_PORT, BEEP_PIN_PIN | LED_Board_LED0_PIN);
    // DL_GPIO_writePins(LED_Board_PORT, LED_Board_LED0_PIN);
}

void BEEP_IRQ() {
    if (beep_ms_count > 0) {
        --beep_ms_count;
        if (beep_ms_count == 0) {
            DL_GPIO_clearPins(BEEP_PORT, BEEP_PIN_PIN | LED_Board_LED0_PIN);
            // DL_GPIO_clearPins(LED_Board_PORT, LED_Board_LED0_PIN);
        }
    }
}
