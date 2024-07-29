#ifndef MENU_HPP
#define MENU_HPP
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef struct {
    uint8 ExitMark; // 退出菜单(0-不退出，1-退出)
    uint8 Cursor;   // 光标值(当前光标位置)
    uint8 PageNo;   // 菜单页(显示开始项)
    uint8 Index;    // 菜单索引(当前选择的菜单项)
    uint8 DispNum;  // 显示项数(每页可以现在菜单项)
    uint8 MaxPage;  // 最大页数(最大有多少种显示页)
} MENU_PRMT;        // 菜单参数

typedef struct {
    uint8 *MenuName;        // 菜单项目名称
    void (*ItemHook)(void); // 要运行的菜单函数
    uint16 *DebugParam;     // 要调试的参数
} MENU_TABLE;               // 菜单执行
// 按键端口的枚举
void main_menu_start();
void Menu_Process(uint8 *menuName, MENU_PRMT *prmt, const MENU_TABLE *table, uint8 num);
#ifdef __cplusplus
}
#endif

#endif