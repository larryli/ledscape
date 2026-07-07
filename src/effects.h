/*
 * WS2812 彩虹流水灯效果库
 *
 * 50 种灯效，针对 1 米 30 颗灯珠远距离观看优化
 */

#ifndef _EFFECTS_H
#define _EFFECTS_H

#include <stdint.h>

/* 配置宏 */
#ifndef NR_LEDS
#define NR_LEDS 30
#endif

#define HUE_TABLE_SIZE 256

/* ==================== 效果枚举 (50种) ==================== */

typedef enum {
    // 彩虹类 (01-08)
    EFFECT_RAINBOW_FLOW = 0,        // 01  彩虹流水
    EFFECT_RAINBOW_FLOW_REVERSE,    // 02  反向彩虹流水
    EFFECT_RAINBOW_BREATH,          // 03  彩虹呼吸
    EFFECT_RAINBOW_PULSE,           // 04  彩虹脉冲
    EFFECT_RAINBOW_BOUNCE,          // 05  彩虹弹跳
    EFFECT_RAINBOW_THEATER,         // 06  彩虹剧场追逐
    EFFECT_RAINBOW_WIPE,            // 07  彩虹擦除
    EFFECT_RAINBOW_WAVE,            // 08  彩虹波浪

    // 呼吸/脉冲类 (09-14)
    EFFECT_WARM_WHITE_BREATH,       // 09  暖白呼吸
    EFFECT_ICE_BLUE_BREATH,         // 10  冰蓝呼吸
    EFFECT_RED_PULSE,               // 11  红色脉冲
    EFFECT_GREEN_PULSE,             // 12  绿色脉冲
    EFFECT_BLUE_PULSE,              // 13  蓝色脉冲
    EFFECT_COLOR_STROBE,            // 14  彩色闪烁

    // 流动/追逐类 (15-20)
    EFFECT_COMET,                   // 15  彗星
    EFFECT_COMET_REVERSE,           // 16  反向彗星
    EFFECT_METEOR_SHOWER,           // 17  流星雨
    EFFECT_COLOR_WIPE,              // 18  颜色擦除
    EFFECT_COLOR_WIPE_REVERSE,      // 19  反向擦除
    EFFECT_THEATER_CHASE,           // 20  剧场追逐

    // 自然模拟类 (21-25)
    EFFECT_CANDLE_LIGHT,            // 21  烛光摇曳
    EFFECT_AURORA,                  // 22  极光效果
    EFFECT_SUNSET,                  // 23  日落渐变
    EFFECT_OCEAN_WAVE,              // 24  海浪效果
    EFFECT_FIRE_FLICKER,            // 25  火焰闪烁

    // 特殊效果类 (26-30)
    EFFECT_SPARKLE,                 // 26  随机闪烁
    EFFECT_SNOW,                    // 27  雪花效果
    EFFECT_DUAL_COLOR,              // 28  双色交替
    EFFECT_TRIPLE_COLOR,            // 29  三色旋转
    EFFECT_GRADIENT_FLOW,           // 30  渐变流水

    // 新增远距离效果 (31-50)
    EFFECT_RAINBOW_SPIRAL,          // 31  彩虹螺旋
    EFFECT_BREATHING_RAINBOW,       // 32  呼吸彩虹
    EFFECT_COMET_RAINBOW,           // 33  彩虹彗星
    EFFECT_FIREWORKS,               // 34  烟花爆发
    EFFECT_WAVE_BURST,              // 35  波浪爆发
    EFFECT_COLOR_RIPPLE,            // 36  颜色涟漪
    EFFECT_PULSE_RING,              // 37  脉冲环
    EFFECT_TWO_COLOR_GRADIENT,      // 38  双色渐变
    EFFECT_RAINBOW_METEOR,          // 39  彩虹流星
    EFFECT_DUAL_COMET,              // 40  双彗星
    EFFECT_BREATHING_WARM,          // 41  暖色呼吸
    EFFECT_BREATHING_COOL,          // 42  冷色呼吸
    EFFECT_RAINBOW_STROBE,          // 43  彩虹频闪
    EFFECT_COLOR_CHASE,             // 44  颜色追逐
    EFFECT_COLOR_CHASE_REVERSE,     // 45  反向追逐
    EFFECT_WARM_COOL_ALTERNATE,     // 46  冷暖交替
    EFFECT_RAINBOW_FADE,            // 47  彩虹淡入淡出
    EFFECT_CENTER_BURST,            // 48  中心爆发
    EFFECT_EDGE_GLOW,               // 49  边缘发光
    EFFECT_RAINBOW_BOUNCE_BREATH,   // 50  彩虹弹跳呼吸

    // 补充效果 (51-60)
    EFFECT_DOUBLE_RAINBOW,          // 51  双彩虹流动
    EFFECT_RAINBOW_STATIC,          // 52  静态彩虹
    EFFECT_COLOR_CHASE_RAINBOW,     // 53  彩虹追逐
    EFFECT_WARM_GLOW,               // 54  暖光效果
    EFFECT_COOL_GLOW,               // 55  冷光效果
    EFFECT_RAINBOW_SPIRAL_REVERSE,  // 56  反向彩虹螺旋
    EFFECT_COMET_FADE,              // 57  彗星淡出
    EFFECT_ALTERNATING_PULSE,       // 58  交替脉冲
    EFFECT_BREATHING_PULSE,         // 59  呼吸脉冲
    EFFECT_RAINBOW_BOUNCE_REVERSE,  // 60  反向彩虹弹跳

    EFFECT_COUNT                    // 效果总数
} EffectType;

/* ==================== 配置结构体 ==================== */

typedef struct {
    uint8_t hue_offset;
    uint8_t speed;
    uint8_t brightness;
    int8_t direction;
    uint8_t mode;
} EffectConfig;

/* ==================== 全局变量 ==================== */

extern EffectConfig g_effect_config;
extern EffectType g_current_effect;
extern uint32_t g_frame_count;

/* ==================== 核心函数 ==================== */

void Effects_Init(void);
void Effects_Next(void);
void Effects_SwitchTo(EffectType effect);
void Effects_Update(void);
uint32_t Effects_CalculateLED(int ledno);

/* ==================== 辅助函数 ==================== */

static inline uint8_t Clamp8(int16_t v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static inline uint8_t Mul8(uint8_t a, uint8_t b) {
    return (uint8_t)((uint16_t)a * b >> 8);
}

#endif
