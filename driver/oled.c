#include "oled.h"
#include "ti_msp_dl_config.h"
#include "mpu6050/board.h"
#include <stdarg.h>
#define OLED_CMD 0                                                  // 写命令
#define OLED_DATA 1                                                 // 写数据
#define OLED_SCL_Clr() DL_GPIO_clearPins(OLED_PORT, OLED_S_SCL_PIN) // at32_led_on(I2C_SCL)
#define OLED_SCL_Set() DL_GPIO_setPins(OLED_PORT, OLED_S_SCL_PIN)   // at32_led_off(I2C_SCL)
uint8_t OLED_GRAM[144][8];
#define OLED_SDA_Clr() DL_GPIO_clearPins(OLED_PORT, OLED_S_SDA_PIN) // at32_led_on(I2C_SDA)
#define OLED_SDA_Set() DL_GPIO_setPins(OLED_PORT, OLED_S_SDA_PIN)   // at32_led_off(I2C_SDA)
static const unsigned char asc2_0806[][6] =
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // sp
        {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00}, // !
        {0x00, 0x00, 0x07, 0x00, 0x07, 0x00}, // "
        {0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
        {0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
        {0x00, 0x62, 0x64, 0x08, 0x13, 0x23}, // %
        {0x00, 0x36, 0x49, 0x55, 0x22, 0x50}, // &
        {0x00, 0x00, 0x05, 0x03, 0x00, 0x00}, // '
        {0x00, 0x00, 0x1c, 0x22, 0x41, 0x00}, // (
        {0x00, 0x00, 0x41, 0x22, 0x1c, 0x00}, // )
        {0x00, 0x14, 0x08, 0x3E, 0x08, 0x14}, // *
        {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08}, // +
        {0x00, 0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
        {0x00, 0x08, 0x08, 0x08, 0x08, 0x08}, // -
        {0x00, 0x00, 0x60, 0x60, 0x00, 0x00}, // .
        {0x00, 0x20, 0x10, 0x08, 0x04, 0x02}, // /
        {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
        {0x00, 0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
        {0x00, 0x42, 0x61, 0x51, 0x49, 0x46}, // 2
        {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
        {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
        {0x00, 0x27, 0x45, 0x45, 0x45, 0x39}, // 5
        {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
        {0x00, 0x01, 0x71, 0x09, 0x05, 0x03}, // 7
        {0x00, 0x36, 0x49, 0x49, 0x49, 0x36}, // 8
        {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
        {0x00, 0x00, 0x36, 0x36, 0x00, 0x00}, // :
        {0x00, 0x00, 0x56, 0x36, 0x00, 0x00}, // ;
        {0x00, 0x08, 0x14, 0x22, 0x41, 0x00}, // <
        {0x00, 0x14, 0x14, 0x14, 0x14, 0x14}, // =
        {0x00, 0x00, 0x41, 0x22, 0x14, 0x08}, // >
        {0x00, 0x02, 0x01, 0x51, 0x09, 0x06}, // ?
        {0x00, 0x32, 0x49, 0x59, 0x51, 0x3E}, // @
        {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
        {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36}, // B
        {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22}, // C
        {0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
        {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41}, // E
        {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01}, // F
        {0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
        {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
        {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00}, // I
        {0x00, 0x20, 0x40, 0x41, 0x3F, 0x01}, // J
        {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41}, // K
        {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40}, // L
        {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
        {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
        {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
        {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06}, // P
        {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
        {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46}, // R
        {0x00, 0x46, 0x49, 0x49, 0x49, 0x31}, // S
        {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01}, // T
        {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
        {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
        {0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
        {0x00, 0x63, 0x14, 0x08, 0x14, 0x63}, // X
        {0x00, 0x07, 0x08, 0x70, 0x08, 0x07}, // Y
        {0x00, 0x61, 0x51, 0x49, 0x45, 0x43}, // Z
        {0x00, 0x00, 0x7F, 0x41, 0x41, 0x00}, // [
        {0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55}, // 55
        {0x00, 0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
        {0x00, 0x04, 0x02, 0x01, 0x02, 0x04}, // ^
        {0x00, 0x40, 0x40, 0x40, 0x40, 0x40}, // _
        {0x00, 0x00, 0x01, 0x02, 0x04, 0x00}, // '
        {0x00, 0x20, 0x54, 0x54, 0x54, 0x78}, // a
        {0x00, 0x7F, 0x48, 0x44, 0x44, 0x38}, // b
        {0x00, 0x38, 0x44, 0x44, 0x44, 0x20}, // c
        {0x00, 0x38, 0x44, 0x44, 0x48, 0x7F}, // d
        {0x00, 0x38, 0x54, 0x54, 0x54, 0x18}, // e
        {0x00, 0x08, 0x7E, 0x09, 0x01, 0x02}, // f
        {0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C}, // g
        {0x00, 0x7F, 0x08, 0x04, 0x04, 0x78}, // h
        {0x00, 0x00, 0x44, 0x7D, 0x40, 0x00}, // i
        {0x00, 0x40, 0x80, 0x84, 0x7D, 0x00}, // j
        {0x00, 0x7F, 0x10, 0x28, 0x44, 0x00}, // k
        {0x00, 0x00, 0x41, 0x7F, 0x40, 0x00}, // l
        {0x00, 0x7C, 0x04, 0x18, 0x04, 0x78}, // m
        {0x00, 0x7C, 0x08, 0x04, 0x04, 0x78}, // n
        {0x00, 0x38, 0x44, 0x44, 0x44, 0x38}, // o
        {0x00, 0xFC, 0x24, 0x24, 0x24, 0x18}, // p
        {0x00, 0x18, 0x24, 0x24, 0x18, 0xFC}, // q
        {0x00, 0x7C, 0x08, 0x04, 0x04, 0x08}, // r
        {0x00, 0x48, 0x54, 0x54, 0x54, 0x20}, // s
        {0x00, 0x04, 0x3F, 0x44, 0x40, 0x20}, // t
        {0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
        {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
        {0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
        {0x00, 0x44, 0x28, 0x10, 0x28, 0x44}, // x
        {0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C}, // y
        {0x00, 0x44, 0x64, 0x54, 0x4C, 0x44}, // z
        {0x14, 0x14, 0x14, 0x14, 0x14, 0x14}, // horiz lines
};
void IIC_delay(void) {
    delay_cycles(79);
}
void I2C_Start(void) {
    OLED_SDA_Set();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SDA_Clr();
    IIC_delay();
    OLED_SCL_Clr();
    IIC_delay();
}
void Send_Byte(uint8_t dat) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (dat & 0x80) {
            OLED_SDA_Set();
        } else {
            OLED_SDA_Clr();
        }
        IIC_delay();
        OLED_SCL_Set();
        IIC_delay();
        OLED_SCL_Clr();
        dat <<= 1;
    }
}
void I2C_WaitAck(void) {
    OLED_SDA_Set();
    IIC_delay();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SCL_Clr();
    IIC_delay();
}
void I2C_Stop(void) {
    OLED_SDA_Clr();
    OLED_SCL_Set();
    IIC_delay();
    OLED_SDA_Set();
}
void OLED_WR_Byte(uint8_t dat, uint8_t mode) {
    I2C_Start();
    Send_Byte(0x78);
    I2C_WaitAck();
    if (mode) {
        Send_Byte(0x40);
    } else {
        Send_Byte(0x00);
    }
    I2C_WaitAck();
    Send_Byte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

void OLED_DisPlay_On(void) {
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x14, OLED_CMD); // 闁跨喐鏋婚幏鐑芥晸閺傘倖瀚归柨鐔告灮閹烽攱娼冮柨鐕傛嫹
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisPlay_Off(void) {
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);
    OLED_WR_Byte(0xAE, OLED_CMD);
}

void OLED_Refresh(void) {
    uint8_t i, n;
    for (i = 0; i < 8; i++) {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        I2C_Start();
        Send_Byte(0x78);
        I2C_WaitAck();
        Send_Byte(0x40);
        I2C_WaitAck();
        for (n = 0; n < 128; n++) {
            Send_Byte(OLED_GRAM[n][i]);
            I2C_WaitAck();
        }
        I2C_Stop();
    }
}

void OLED_Clear(void) {
    uint8_t i, n;
    for (i = 0; i < 8; i++) {
        for (n = 0; n < 128; n++) {
            OLED_GRAM[n][i] = 0;
        }
    }
    // OLED_Refresh();
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t) {
    uint8_t i, m, n;
    i = y / 8;
    m = y % 8;
    n = 1 << m;
    if (t) {
        OLED_GRAM[x][i] |= n;
    } else {
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
        OLED_GRAM[x][i] |= n;
        OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
    }
}

void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode) {
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    uRow = x1;
    uCol = y1;
    if (delta_x > 0)
        incx = 1;
    else if (delta_x == 0)
        incx = 0;
    else {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0;
    else {
        incy = -1;
        delta_y = -delta_x;
    }
    if (delta_x > delta_y)
        distance = delta_x;
    else
        distance = delta_y;
    for (t = 0; t < distance + 1; t++) {
        OLED_DrawPoint(uRow, uCol, mode);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
}

void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r) {
    int a, b, num;
    a = 0;
    b = r;
    while (2 * b * b >= r * r) {
        OLED_DrawPoint(x + a, y - b, 1);
        OLED_DrawPoint(x - a, y - b, 1);
        OLED_DrawPoint(x - a, y + b, 1);
        OLED_DrawPoint(x + a, y + b, 1);

        OLED_DrawPoint(x + b, y + a, 1);
        OLED_DrawPoint(x + b, y - a, 1);
        OLED_DrawPoint(x - b, y - a, 1);
        OLED_DrawPoint(x - b, y + a, 1);

        a++;
        num = (a * a + b * b) - r * r;
        if (num > 0) {
            b--;
            a--;
        }
    }
}
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode) {
    uint8_t i, m, temp, size2, chr1;
    uint8_t x0 = x, y0 = y;
    if (size1 == 8)
        size2 = 6;
    else
        size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); // 闁跨喓鐓喊澶嬪闁跨喐鏋婚幏鐑芥晸閺傘倖瀚规稉锟介柨鐔告灮閹风兘鏁撶悰妤€鍤栭幏鐑芥晸閺傘倖瀚规惔鏃堟晸閺傘倖瀚归柨鐔告灮閹风兘鏁撻弬銈嗗閸楃娀鏁撻弬銈嗗闁跨喕顢滈弬銈嗗闁跨喐鏋婚幏锟�
    chr1 = chr - ' ';                                              // 闁跨喐鏋婚幏鐑芥晸閺傘倖瀚归崑蹇涙晸閻欙紕灏ㄩ幏鐑芥晸鐞涙鎷�
    for (i = 0; i < size2; i++) {
        if (size1 == 8) {
            temp = asc2_0806[chr1][i];
        } else
            return;
        for (m = 0; m < 8; m++) {
            if (temp & 0x01)
                OLED_DrawPoint(x, y, mode);
            else
                OLED_DrawPoint(x, y, !mode);
            temp >>= 1;
            y++;
        }
        x++;
        if ((size1 != 8) && ((x - x0) == size1 / 2)) {
            x = x0;
            y0 = y0 + 8;
        }
        y = y0;
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t mode) {
    while ((*chr >= ' ') && (*chr <= '~')) {
        OLED_ShowChar(x, y, *chr, 8, mode);
        x += 6;
        chr++;
    }
}

// m^n
uint32_t OLED_Pow(uint8_t m, uint8_t n) {
    uint32_t result = 1;
    while (n--) {
        result *= m;
    }
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {
    uint8_t t, temp, m = 0;
    if (size1 == 8) m = 2;
    for (t = 0; t < len; t++) {
        temp = (num / OLED_Pow(10, len - t - 1)) % 10;
        if (temp == 0) {
            OLED_ShowChar(x + (size1 / 2 + m) * t, y, '0', size1, mode);
        } else {
            OLED_ShowChar(x + (size1 / 2 + m) * t, y, temp + '0', size1, mode);
        }
    }
}
void OLED_Init(void) {
    // OLED_RES_Clr();
    delay_ms(200);
    // delay_cycles(7000000);
    // OLED_RES_Set();

    OLED_WR_Byte(0xAE, OLED_CMD); //--turn off oled panel
    OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
    OLED_WR_Byte(0x10, OLED_CMD); //---set high column address

    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)

    OLED_WR_Byte(0x81, OLED_CMD); //--set contrast control register
    OLED_WR_Byte(0xCF, OLED_CMD); // Set SEG Output Current Brightness

    OLED_WR_Byte(0xA1, OLED_CMD); //--Set SEG/Column Mapping     0xa0闁跨喐鏋婚幏鐑芥晸閹活厼鍤栭幏鐑芥晸閺傘倖瀚� 0xa1闁跨喐鏋婚幏鐑芥晸閺傘倖瀚�
    OLED_WR_Byte(0xC8, OLED_CMD); // Set COM/Row Scan Direction   0xc0闁跨喐鏋婚幏鐑芥晸闁炬澘鍤栭幏鐑芥晸閺傘倖瀚� 0xc8闁跨喐鏋婚幏鐑芥晸閺傘倖瀚�
    OLED_WR_Byte(0xA6, OLED_CMD); //--set normal display

    OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3f, OLED_CMD); //--1/64 duty

    OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
    OLED_WR_Byte(0x00, OLED_CMD); //-not offset

    OLED_WR_Byte(0xd5, OLED_CMD); //--set display clock divide ratio/oscillator frequency
    OLED_WR_Byte(0x80, OLED_CMD); //--set divide ratio, Set Clock as 100 Frames/Sec

    OLED_WR_Byte(0xD9, OLED_CMD); //--set pre-charge period
    OLED_WR_Byte(0xF1, OLED_CMD); // Set Pre-Charge as 15 Clocks & Discharge as 1 Clock

    OLED_WR_Byte(0xDA, OLED_CMD); //--set com pins hardware configuration
    OLED_WR_Byte(0x12, OLED_CMD);

    OLED_WR_Byte(0xDB, OLED_CMD); //--set vcomh
    OLED_WR_Byte(0x40, OLED_CMD); // Set VCOM Deselect Level

    OLED_WR_Byte(0x20, OLED_CMD); //-Set Page Addressing Mode (0x00/0x01/0x02)
    OLED_WR_Byte(0x02, OLED_CMD); //

    OLED_WR_Byte(0x8D, OLED_CMD); //--set Charge Pump enable/disable
    OLED_WR_Byte(0x14, OLED_CMD); //--set(0x10) disable

    // OLED_WR_Byte(0xA4,OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
    // OLED_WR_Byte(0xA6,OLED_CMD);// Disable Inverse Display On (0xa6/a7)

    OLED_Clear();
    OLED_WR_Byte(0xAF, OLED_CMD);
}
void oled_print(uint8_t line, char *format, ...) {
    uint8_t y = line * 8;
    static char tmp[64];
    va_list args;
    va_start(args, format);
    vsprintf(tmp, format, args);

    for (uint8_t i = strlen(tmp); i < 144 / 6; ++i) {
        tmp[i] = ' ';        
    }
    tmp[144 / 6] = 0;
    va_end(args);
    OLED_ShowString(0, y, tmp, 1);
}