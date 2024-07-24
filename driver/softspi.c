#include "softspi.h"
#include "ti_msp_dl_config.h"

void softspi_writedata(uint8_t data) {
  DL_SPI_transmitData8(HARDSPI_TFT_INST, data);
  while (DL_SPI_isBusy(HARDSPI_TFT_INST))
    ;
  return;
  //下面是软件方式spi
  for (int i = 8; i > 0; --i) {
    if (data & 0x80) {
      DL_GPIO_setPins(TFT_MOSI_PORT, TFT_MOSI_PIN);
    } else {
      DL_GPIO_clearPins(TFT_MOSI_PORT, TFT_MOSI_PIN);
    }
    DL_GPIO_clearPins(TFT_SCLK_PORT, TFT_SCLK_PIN);
    DL_GPIO_setPins(TFT_SCLK_PORT, TFT_SCLK_PIN);
    data <<= 1;
  }
}