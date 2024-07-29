#include "menu.hpp"
#include "ti_msp_dl_config.h"
#include <cstdint>
#include "key.hpp"
extern "C" {
#include "oled.h"
}
#define PAGE_DISP_NUM 7

using uint16 = uint16_t;
using uint8 = uint8_t;
static constexpr uint8 font_size = 8;
// oled 64*144 / (8*6) =8*24
typedef struct {
    uint8 x;
    uint8 y;
} Site_t; // 光标

void Menu_Display(const MENU_TABLE *menuTable, uint8 pageNo, uint8 dispNum, uint8 cursor) {
    uint8 i;
    Site_t site;
    uint8_t mode = 1;
    for (i = 0; i < dispNum; i++) {
        if (cursor == i) {
            // 修改画笔
            mode = 0;
        }
        /* 显示菜单项 */
        site.x = 0;
        site.y = (i + 1) * 8;
        OLED_ShowString(site.x, site.y, menuTable[pageNo + i].MenuName, mode);
        /* 若此菜单项有需要调的参数，则显示该参数 */
        if (menuTable[pageNo + i].DebugParam != nullptr) {
            site.x = 96;
            uint16 num_t = (*(menuTable[pageNo + i].DebugParam));
            OLED_ShowNum(site.x, site.y, num_t, 4, font_size, mode);
        } else {
            // 没有要调整的参数，显示空白
            site.x = 96;
            OLED_ShowString(site.x, site.y, (uint8 *)"   ", mode);
        }
        mode = 1;
    }
    OLED_Refresh();
}

/******************************************************************************
 * FunctionName   : Menu_PrmtInit()
 * Description    : 初始化菜单参数
 * EntryParameter : prmt - 菜单参数, num - 每页显示项数, page - 最大显示页数
 * ReturnValue    : None
 *******************************************************************************/
void Menu_PrmtInit(MENU_PRMT *prmt, uint8 num, uint8 page) {
    prmt->ExitMark = 0;   // 清除退出菜单标志
    prmt->Cursor = 0;     // 光标清零
    prmt->PageNo = 0;     // 页清零
    prmt->Index = 0;      // 索引清零
    prmt->DispNum = num;  // 页最多显示项目数
    prmt->MaxPage = page; // 最多页数
}

/******************************************************************************
 * FunctionName   : Menu_Move()
 * Description    : 菜单移动
 * EntryParameter :  prmt - 菜单参数, key - 按键值
 * ReturnValue    : 有确认返回0，否则返回1
 ******************************************************************************/
uint8 Menu_Move(MENU_PRMT *prmt, KEY_e key) {
    uint8 rValue = 1;

    switch (key) {
    case KEY_NEXT: // 向下
    {
        if (prmt->Cursor < prmt->DispNum - 1) // 光标没有到底，移动光标
        {
            prmt->Cursor++; // 光标向下移动
        } else              // 光标到底
        {
            if (prmt->PageNo < prmt->MaxPage - 1) // 页面没有到底，页面移动
            {
                prmt->PageNo++; // 下翻一页
            } else              // 页面和光标都到底，返回开始页
            {
                prmt->Cursor = 0;
                prmt->PageNo = 0;
            }
        }
        break;
    }
    case KEY_SELECT: // 确认
    {
        prmt->Index = prmt->Cursor + prmt->PageNo; // 计算执行项的索引
        rValue = 0;

        break;
    }
    default:
        break;
    }
    return rValue; // 返回执行索引
}

void adjustParam(Site_t site, uint16 *param, uint8 max_param_bit) {
    // 就两个按键别调了
}

/******************************************************************************
 * FunctionName   : Menu_Process()
 * Description    : 处理菜单项
 * EntryParameter : menuName - 菜单名称，prmt - 菜单参数，table - 菜单表项, num - 菜单项数
 * ReturnValue    : None
 ******************************************************************************/
void Menu_Process(uint8 *menuName, MENU_PRMT *prmt, const MENU_TABLE *table, uint8 num) {
    KEY_e key;
    Site_t site;

    uint8 page; // 显示菜单需要的页数

    if (num - PAGE_DISP_NUM <= 0)
        page = 1;
    else {
        page = num - PAGE_DISP_NUM + 1;
        num = PAGE_DISP_NUM;
    }

    // 显示项数和页数设置
    Menu_PrmtInit(prmt, num, page);

    do {
        OLED_ShowString(0, 0, menuName, 1); // 显示菜单标题
        // 显示菜单项
        Menu_Display(table, prmt->PageNo, prmt->DispNum, prmt->Cursor);
        key = KeySan(); // 获取按键

        if (Menu_Move(prmt, key) == 0) // 菜单移动 按下确认键
        {
            // 判断此菜单项有无需要调节的参数 有则进入参数调节
            if (table[prmt->Index].DebugParam != nullptr && table[prmt->Index].ItemHook == nullptr) {
                site.x = 120;
                site.y = (1 + prmt->Cursor) * 16;

                OLED_ShowNum(site.x, site.y, *(table[prmt->Index].DebugParam), 4, font_size, 1);
                adjustParam(site, table[prmt->Index].DebugParam, 4);
            }
            // 不是参数调节的话就执行菜单函数
            else if (table[prmt->Index].ItemHook != nullptr) {
                table[prmt->Index].ItemHook(); // 执行相应项
            } else {
                return;
            }
        }
    } while (prmt->ExitMark == 0);

    OLED_Clear();
}

void main_menu_start() {
    static uint16 curquiz_id = 100;
    MENU_TABLE MainMenu_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"curquiz ", nullptr, &curquiz_id},
            {(uint8 *)"change q 1", []() { ++curquiz_id; }, nullptr},
            {(uint8 *)"1234567890123456", nullptr, nullptr},
            {(uint8 *)"pwm open/close", []() {
                 DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 400,
                                                  DL_TIMER_CC_0_INDEX);
                 DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, 400,
                                                  DL_TIMER_CC_1_INDEX);
             },
             nullptr}};
    // 一级菜单
    MENU_PRMT MainMenu_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(MainMenu_Table) / sizeof(MainMenu_Table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   Setting   =- ", &MainMenu_Prmt, MainMenu_Table, menuNum);
}
