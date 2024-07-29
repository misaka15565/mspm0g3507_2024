#ifndef KEY_HPP
#define KEY_HPP

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    KEY_NEXT, //
    KEY_SELECT,

    KEY_MAX,
} KEY_e;
// key状态枚举定义
typedef enum {
    KEY_DOWN = 0, // 按键按下时对应电平
    KEY_UP = 1,   // 按键弹起时对应电平

    KEY_HOLD, // 长按按键(用于定时按键扫描)

} KEY_STATUS_e;

// 按键消息结构体
typedef struct
{
    KEY_e key;           // 按键编号
    KEY_STATUS_e status; // 按键状态
} KEY_MSG_t;

void key_IRQHandler();
KEY_e KeySan();
extern volatile KEY_MSG_t keymsg;

#ifdef __cplusplus
}
#endif

#endif