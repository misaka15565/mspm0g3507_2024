#include "menu.hpp"
#include "ti_msp_dl_config.h"
#include <cstdint>
#include "key.hpp"
#include "utils/delay.hpp"
#include "control.hpp"
#include "statemachine.hpp"
extern "C" {
#include "oled.h"
#include "motor.h"
#include <stdio.h>
}
#define PAGE_DISP_NUM 6

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
        OLED_Clear();
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

void adjust_time_adjust() {
    MENU_TABLE subMenu1_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"+ val", []() {
                 ++time_adjust;
             },
             nullptr},
            {(uint8 *)"- val", []() {
                 --time_adjust;
             },
             nullptr},
            {(uint8 *)"+10 val", []() {
                 time_adjust += 10;
             },
             nullptr},
            {(uint8 *)"-10 val", []() {
                 time_adjust -= 10;
             },
             nullptr},
            {(uint8 *)"+100 val", []() {
                 time_adjust += 100;
             },
             nullptr},
            {(uint8 *)"-100 val", []() {
                 time_adjust -= 100;
             },
             nullptr},
            {(uint8 *)"val show", nullptr, &time_adjust}};
    MENU_PRMT subMenu1_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(subMenu1_Table) / sizeof(subMenu1_Table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   adjust   =- ", &subMenu1_Prmt, subMenu1_Table, menuNum);
}

void pwm_test_menu() {
    MENU_TABLE table[] = {
        {(uint8 *)"return ", nullptr, nullptr},
        {(uint8 *)"close pwm", []() {
             DL_TimerG_stopCounter(PWM_MOTOR_INST);
         },
         nullptr},
        {(uint8 *)"open pwm", []() {
             DL_TimerG_startCounter(PWM_MOTOR_INST);
         },
         nullptr},
        {(uint8 *)"pwm ch0 add", []() {
             static uint16_t pwm_val = 0;
             pwm_val += 100;
             pwm_val %= 1100;
             DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_val,
                                              DL_TIMER_CC_0_INDEX);
         },
         nullptr},
        {(uint8 *)"pwm ch1 add", []() {
             static uint16_t pwm_val = 0;
             pwm_val += 100;
             pwm_val %= 1100;
             DL_TimerG_setCaptureCompareValue(PWM_MOTOR_INST, pwm_val,
                                              DL_TIMER_CC_1_INDEX);
         },
         nullptr},
    };
    MENU_PRMT prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(table) / sizeof(table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   pwm test   =- ", &prmt, table, menuNum);
}

// 有捕获的lambda不能转为void(*)()函数指针，所以就这样放着吧
static uint16 now_problem_id = now_problem;
void main_menu_start() {
    MENU_TABLE MainMenu_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"set problem1", []() {
                 now_problem = problem_1;
                 now_problem_id = problem_1;
             },
             nullptr},
            {(uint8 *)"set problem2", []() {
                 now_problem = problem_2;
                 now_problem_id = problem_2;
             },
             nullptr},
            {(uint8 *)"set problem3", []() {
                 now_problem = problem_3;
                 now_problem_id = problem_3;
             },
             nullptr},
            {(uint8 *)"set problem4", []() {
                 now_problem = problem_4;
                 now_problem_id = problem_4;
             },
             nullptr},
            {(uint8 *)"now problem is", nullptr, &now_problem_id},
            {(uint8 *)"oled flush test", []() {
                 uint32_t time_start = sys_cur_tick_us;
                 for (int i = 0; i < 100; ++i) {
                     OLED_Refresh();
                 }
                 oled_print(0, "%d\n", sys_cur_tick_us - time_start);
                 delay_ms(1000);
             },
             nullptr},
            {(uint8 *)"adjust time adjust", adjust_time_adjust, nullptr}};
    // 一级菜单
    MENU_PRMT MainMenu_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(MainMenu_Table) / sizeof(MainMenu_Table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   Setting   =- ", &MainMenu_Prmt, MainMenu_Table, menuNum);
}
