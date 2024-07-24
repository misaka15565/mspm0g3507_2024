#include "uart_receive.h"
#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
char uart_openmv_buffer[256];
int16_t uart_openmv_pos;
static const char start_char = '#';
static const char end_char = '$';
void UART_OPENMV_INST_IRQHandler(void) {
    char rData = 0;
    static bool recv_enable = false;
    switch (DL_UART_Main_getPendingInterrupt(UART_OPENMV_INST)) {
    case DL_UART_MAIN_IIDX_RX:
        rData = DL_UART_Main_receiveData(UART_OPENMV_INST);
        if (rData == start_char) {
            uart_openmv_pos = 0;
            recv_enable = true;
        } else if (rData == end_char) {
            // do sth
            uart_openmv_pos = 0;
            recv_enable = false;
        } else if (recv_enable) {
            uart_openmv_buffer[uart_openmv_pos] = rData;
            ++uart_openmv_pos;
        }
        break;
    default: break;
    }
}