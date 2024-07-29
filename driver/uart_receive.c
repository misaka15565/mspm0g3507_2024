#include "uart_receive.h"
#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
volatile char uart_openmv_buffer[256];
volatile int16_t uart_openmv_pos;
static const char start_char = '#';
static const char end_char = '$';
volatile int16_t target_speed_L = 0;
volatile int16_t target_speed_R = 0;
void UART_OPENMV_INST_IRQHandler(void) {
    char rData = 0;
    static bool recv_enable = false;
    switch (DL_UART_Main_getPendingInterrupt(UART_OPENMV_INST)) {
    case DL_UART_MAIN_IIDX_RX:
        rData = DL_UART_Main_receiveData(UART_OPENMV_INST);
        // DL_UART_Main_transmitData(UART_OPENMV_INST,rData);
        if (rData == start_char) {
            uart_openmv_pos = 0;
            recv_enable = true;
        } else if (rData == end_char) {
            // do sth
            uart_openmv_pos = 0;
            uart_openmv_buffer[uart_openmv_pos] = 0;
            recv_enable = false;
        } else if (recv_enable) {
            uart_openmv_buffer[uart_openmv_pos] = rData;
            ++uart_openmv_pos;
            uart_openmv_buffer[uart_openmv_pos] = 0;
        }
        break;
    default: break;
    }
}