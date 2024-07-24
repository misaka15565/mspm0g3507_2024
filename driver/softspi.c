#include "softspi.h"
#include "ti_msp_dl_config.h"

void softspi_writedata(uint8_t data) {
  DL_SPI_transmitData8(HARDSPI_TFT_INST, data);
  while (DL_SPI_isBusy(HARDSPI_TFT_INST))
    ;
}