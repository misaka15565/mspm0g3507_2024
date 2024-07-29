#ifndef UART_RECEIVE_H
#define UART_RECEIVE_H
#include "ti_msp_dl_config.h"
extern volatile char uart_openmv_buffer[256];
void UART_OPENMV_INST_IRQHandler(void);
#endif