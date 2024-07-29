#include "key.hpp"
#include "ti_msp_dl_config.h"
volatile KEY_MSG_t keymsg;
#include <cstdint>
using int8 = int8_t;
using uint8 = uint8_t;

#define KEY_DOWN_TIME 2  // 消抖确认按下时间
#define KEY_HOLD_TIME 50 // 长按hold确认时间，最多253，否则需要修改 keytime 的类型
                         // 如果按键一直按下去，则每隔 KEY_HOLD_TIME - KEY_DOWN_TIME 时间会发送一个 KEY_HOLD 消息

KEY_STATUS_e key_get(KEY_e key) {
    if (key == KEY_NEXT) {
        return DL_GPIO_readPins(KEY_NEXT_PORT, KEY_NEXT_PIN) == 0 ? KEY_DOWN : KEY_UP;
        // 按键s3 pb21
    }
    if (key == KEY_SELECT) {
        return DL_GPIO_readPins(KEY_SELECT_PORT, KEY_SELECT_PIN) == 0 ? KEY_DOWN : KEY_UP;
        // 按键s2 pa18
    }
}

void key_IRQHandler(void) {
    int8 keynum;
    static uint8 keytime[KEY_MAX]; // 静态数组，保存各数组按下时间

    // KEY_MSG_t keymsg; // 按键消息

    for (keynum = 0; keynum < KEY_MAX; keynum++) // 每个按键轮询
    {
        if (key_get((KEY_e)keynum) == KEY_DOWN) // 判断按键是否按下
        {
            keytime[keynum]++; // 按下时间累加

            if (keytime[keynum] <= KEY_DOWN_TIME) // 判断时间是否没超过消抖确认按下时间
            {
                continue;                                    // 没达到，则继续等待
            } else if (keytime[keynum] == KEY_DOWN_TIME + 1) // 判断时间是否为消抖确认按下时间
            {
                // 确认按键按下
                keymsg.key = (KEY_e)keynum;
                keymsg.status = KEY_DOWN;
            }
        } else {
            if (keytime[keynum] > KEY_DOWN_TIME) // 如果确认过按下按键
            {
                keymsg.key = (KEY_e)keynum;
                keymsg.status = KEY_UP;
            }

            keytime[keynum] = 0; // 时间累计清0
        }
    }
}
// 阻塞等待按键按下
KEY_e KeySan() {
    while (keymsg.status == KEY_UP) {
    }
    keymsg.status = KEY_UP;
    return keymsg.key;
}