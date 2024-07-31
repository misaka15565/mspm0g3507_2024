#include "menu.hpp"

#include "speed.hpp"
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
#include "driver/encoder.h"
#include "gyro.h"
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
void Menu_Process(uint8 *menuName, MENU_PRMT *prmt, MENU_TABLE *table, uint8 num) {
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

static uint16 kp_mul_100 = 0;
static uint16 ki_mul_100 = 0;

static uint16 adjust_param_uint_core_temp;
// 通用uint16调参核心
void adjust_uint16_param_menu_core(char *name, uint16 &ref) {
    adjust_param_uint_core_temp = ref;
    MENU_TABLE subMenu1_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"+1 val", []() {
                 ++adjust_param_uint_core_temp;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)"-1 val", []() {
                 --adjust_param_uint_core_temp;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)"+10 val", []() {
                 adjust_param_uint_core_temp += 10;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)"-10 val", []() {
                 adjust_param_uint_core_temp -= 10;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)"+100 val", []() {
                 adjust_param_uint_core_temp += 100;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)"-100 val", []() {
                 adjust_param_uint_core_temp -= 100;
             },
             &adjust_param_uint_core_temp},
            {(uint8 *)name, nullptr, &adjust_param_uint_core_temp}};
    MENU_PRMT subMenu1_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(subMenu1_Table) / sizeof(subMenu1_Table[0]); // 菜单项数
    Menu_Process((uint8 *)name, &subMenu1_Prmt, subMenu1_Table, menuNum);
    ref = adjust_param_uint_core_temp;
}
static char param_adjust_float_buf[64];
static float adjust_param_float_core_temp;
static char *param_name;
void adjust_float_param_menu_core(char *name, float &ref) {
    adjust_param_float_core_temp = ref;
    param_name = name;
    sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
    MENU_TABLE subMenu1_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"+0.01 val", []() {
                 adjust_param_float_core_temp += 0.01;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"-0.01 val", []() {
                 adjust_param_float_core_temp -= 0.01;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"+0.1 val", []() {
                 adjust_param_float_core_temp += 0.1;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"-0.1 val", []() {
                 adjust_param_float_core_temp -= 0.1;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"+1 val", []() {
                 adjust_param_float_core_temp += 1;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"-1 val", []() {
                 adjust_param_float_core_temp -= 1;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr},
            {(uint8 *)"*-1", []() {
                 adjust_param_float_core_temp *= -1;
                 sprintf(param_adjust_float_buf, "%s=%f", param_name, adjust_param_float_core_temp);
             },
             nullptr}};
    MENU_PRMT subMenu1_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(subMenu1_Table) / sizeof(subMenu1_Table[0]); // 菜单项数
    Menu_Process((uint8 *)param_adjust_float_buf, &subMenu1_Prmt, subMenu1_Table, menuNum);
    ref = adjust_param_float_core_temp;
}

void pid_adjust_menu() {
    kp_mul_100 = Velcity_Kp * 100;
    ki_mul_100 = Velcity_Ki * 100;
    MENU_TABLE table[] = {
        {(uint8 *)"return ", nullptr, nullptr},
        {(uint8 *)"try run R motor", []() {
             Set_PWM(0, 0);
             PID_clear_A();
             PID_clear_B();
             motor_R_run_distance_at_speed(290, 5);
             set_target_speed(10, 10);
             delay_ms(5000);
             set_target_speed(0, 0);
         },
         nullptr},
        {(uint8 *)"try run L motor", []() {
             Set_PWM(0, 0);
             PID_clear_A();
             PID_clear_B();
             motor_L_run_distance_at_speed(290, 5);
             set_target_speed(10, 10);
             delay_ms(5000);
             set_target_speed(0, 0);
         },
         nullptr},
        {(uint8 *)"read abs encoder", []() {
             int origin_left = encoderA_get();
             int origin_right = encoderB_get();
             OLED_Clear();
             // 不退出的，你要reset
             while (true) {
                 int left = encoderA_get();
                 int right = encoderB_get();
                 oled_print(0, "left=%d", left - origin_left);
                 oled_print(1, "right=%d", right - origin_right);
                 OLED_Refresh();
             }
         },
         nullptr},
        {(uint8 *)"kp*100 ++", []() {
             ++kp_mul_100;
         },
         &kp_mul_100},
        {(uint8 *)"ki*100 ++", []() {
             ++ki_mul_100;
         },
         &ki_mul_100},
        {(uint8 *)"kp*100 --", []() {
             --kp_mul_100;
         },
         &kp_mul_100},
        {(uint8 *)"ki*100 --", []() {
             --ki_mul_100;
         },
         &ki_mul_100}};
    MENU_PRMT prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(table) / sizeof(table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   pid adjust   =- ", &prmt, table, menuNum);
    Velcity_Kp = (float)kp_mul_100 / 100.0;
    Velcity_Ki = (float)ki_mul_100 / 100.0;
}

void turn_direction_adjust_menu() {
    // 调整硬编码转向的参数，是control.hpp里的全局变量

    MENU_TABLE table[] = {
        {(uint8 *)"return ", nullptr, nullptr},
        {(uint8 *)"adj_at_A_par", []() {
             adjust_uint16_param_menu_core("at_A_par", adjust_at_A_param);
         },
         nullptr},
        {(uint8 *)"adj_at_B_par", []() {
             adjust_uint16_param_menu_core("at_B_par", adjust_at_B_param);
         },
         nullptr},
        {(uint8 *)"adj_params[0][0]", []() {
             adjust_uint16_param_menu_core("params[0][0]", adjust_params[0][0]);
         },
         nullptr},
        {(uint8 *)"adj_params[0][1]", []() {
             adjust_uint16_param_menu_core("params[0][1]", adjust_params[0][1]);
         },
         nullptr},
        {(uint8 *)"adj_params[1][0]", []() {
             adjust_uint16_param_menu_core("params[1][0]", adjust_params[1][0]);
         },
         nullptr},
        {(uint8 *)"adj_params[1][1]", []() {
             adjust_uint16_param_menu_core("params[1][1]", adjust_params[1][1]);
         },
         nullptr},
        {(uint8 *)"adj_params[2][0]", []() {
             adjust_uint16_param_menu_core("params[2][0]", adjust_params[2][0]);
         },
         nullptr},
        {(uint8 *)"adj_params[2][1]", []() {
             adjust_uint16_param_menu_core("params[2][1]", adjust_params[2][1]);
         },
         nullptr},
        {(uint8 *)"adj_params[3][0]", []() {
             adjust_uint16_param_menu_core("params[3][0]", adjust_params[3][0]);
         },
         nullptr},
        {(uint8 *)"adj_params[3][1]", []() {
             adjust_uint16_param_menu_core("params[3][1]", adjust_params[3][1]);
         },
         nullptr}};

    MENU_PRMT prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(table) / sizeof(table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   turn direction adjust   =- ", &prmt, table, menuNum);
}

void param_adjust_menu() {
    MENU_TABLE table[] = {
        {(uint8 *)"return ", nullptr, nullptr},
        //{(uint8 *)"time adjust", adjust_time_adjust, nullptr},
        {(uint8 *)"pwm test", pwm_test_menu, nullptr},
        {(uint8 *)"pid adjust", pid_adjust_menu, nullptr},
        {(uint8 *)"adj weight mid", []() {
             adjust_float_param_menu_core("w_mid", weight_mid);
         },
         nullptr},
        {(uint8 *)"adj weight front", []() {
             adjust_float_param_menu_core("w_side", weight_front);
         },
         nullptr},
        {(uint8 *)"turn distance", turn_direction_adjust_menu, nullptr},
    };
    MENU_PRMT prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(table) / sizeof(table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   Setting   =- ", &prmt, table, menuNum);
}

static uint16 close_oled_while_run = 1;
// 有捕获的lambda不能转为void(*)()函数指针，所以就这样放着吧
static uint16 now_problem_id = now_problem;
void main_menu_start() {
    MENU_TABLE MainMenu_Table[] =
        {
            {(uint8 *)"return ", nullptr, nullptr},
            {(uint8 *)"switch next problem", []() {
                 now_problem_id++;
                 if (now_problem_id > problem_4) {
                     now_problem_id = problem_1;
                 }
                 now_problem = (problem)(now_problem_id);
             },
             &now_problem_id},
            {(uint8 *)"run cur prob", []() {
                 PID_clear_A();
                 PID_clear_B();
                 if (close_oled_while_run) {
                     oled_disable_print = 1;
                 }
                 delay_ms(1000);
                 switch (now_problem) {
                 case problem_1:
                     go_problem1();
                     break;
                 case problem_2:
                     go_problem2();
                     break;
                 case problem_3:
                     go_problem3();
                     break;
                 case problem_4:
                     go_problem4();
                     break;
                 default:
                     break;
                 }
                 oled_disable_print = 0;
             },
             &now_problem_id},
            //{(uint8 *)"go problem1", go_problem1, nullptr},
            //{(uint8 *)"go problem2", go_problem2, nullptr},
            //{(uint8 *)"go problem3", go_problem3, nullptr},
            //{(uint8 *)"go problem4", go_problem4, nullptr},
            {(uint8 *)"keep oled toggle", []() {
                 close_oled_while_run = 1 - close_oled_while_run;
             },
             &close_oled_while_run},
            {(uint8 *)"adj parmas", param_adjust_menu, nullptr},
        };
    // 一级菜单
    MENU_PRMT MainMenu_Prmt;
    OLED_Clear();
    uint8 menuNum = sizeof(MainMenu_Table) / sizeof(MainMenu_Table[0]); // 菜单项数
    Menu_Process((uint8 *)" -=   Setting   =- ", &MainMenu_Prmt, MainMenu_Table, menuNum);

    if (close_oled_while_run) {
        oled_disable_print = 1;
    }
}
