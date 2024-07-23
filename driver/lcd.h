#ifndef LCD_Headfile
#define LCD_Headfile

#include <stdint.h>
typedef uint16_t u16;
typedef uint8_t u8;

// LCD重要参数集
typedef struct {
  u16 width;   // LCD 宽度
  u16 height;  // LCD 高度
  u16 id;      // LCD ID
  u8 dir;      // 横屏还是竖屏控制：0，竖屏；1，横屏。
  u16 wramcmd; // 开始写gram指令
  u16 setxcmd; // 设置x坐标指令
  u16 setycmd; // 设置y坐标指令
} _lcd_dev;

// LCD参数
extern _lcd_dev lcddev; // 管理LCD重要参数
/////////////////////////////////////用户配置区///////////////////////////////////
#define USE_HORIZONTAL                                                         \
  0 // 定义液晶屏顺时针旋转方向
    // 0-0度旋转，1-90度旋转，2-180度旋转，3-270度旋转

//////////////////////////////////////////////////////////////////////////////////
// 定义LCD的尺寸
#define LCD_W 320
#define LCD_H 480

// TFTLCD部分外要调用的函数
extern u16 POINT_COLOR; // 默认红色
extern u16 BACK_COLOR;  // 背景颜色.默认为白色

// 画笔颜色
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40 // 棕色
#define BRRED 0XFC07 // 棕红色
#define GRAY 0X8430  // 灰色
// GUI颜色

#define DARKBLUE 0X01CF  // 深蓝色
#define LIGHTBLUE 0X7D7C // 浅蓝色
#define GRAYBLUE 0X5458  // 灰蓝色
// 以上三色为PANEL的颜色

#define LIGHTGREEN 0X841F // 浅绿色
#define LIGHTGRAY 0XEF5B  // 浅灰色(PANNEL)
#define LGRAY 0XC618      // 浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE 0XA651 // 浅灰蓝色(中间层颜色)
#define LBBLUE 0X2B12    // 浅棕蓝色(选择条目的反色)

void LCD_Init(void);
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
void LCD_Clear(u16 Color);
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_DrawPoint(u16 x, u16 y); // 画点
u16 LCD_ReadPoint(u16 x, u16 y);  // 读点
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_SetWindows(u16 xStar, u16 yStar, u16 xEnd, u16 yEnd);

u16 LCD_RD_DATA(void); // 读取LCD数据
void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue);
void LCD_WR_DATA(u8 data);
u16 LCD_ReadReg(u8 LCD_Reg);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteRAM(u16 RGB_Code);
u16 LCD_ReadRAM(void);
u16 LCD_BGR2RGB(u16 c);
void LCD_SetParam(void);
void Lcd_WriteData_16Bit(u16 Data);
void LCD_direction(u8 direction);
void LCD_Show_String(u16 x, u16 y, char *str);
#endif