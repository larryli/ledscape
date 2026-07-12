/*
 * WS2812 彩虹流水灯效果库
 *
 * 60 种彩虹灯效，针对 1 米 30 颗灯珠远距离观看优化
 */

#ifndef _EFFECTS_H
#define _EFFECTS_H

#include <stdint.h>

/* 配置宏 */
#ifndef NR_LEDS
#define NR_LEDS 30
#endif

#define HUE_TABLE_SIZE 256

/* ==================== 效果枚举 (60种) ==================== */

typedef enum {
    // 彩虹基础类 (01-08)
    EFFECT_RAINBOW_FLOW = 0,        // 01  彩虹流水
    EFFECT_RAINBOW_FLOW_REVERSE,    // 02  反向彩虹流水
    EFFECT_RAINBOW_BREATH,          // 03  彩虹呼吸
    EFFECT_RAINBOW_PULSE,           // 04  彩虹脉冲
    EFFECT_RAINBOW_BOUNCE,          // 05  彩虹弹跳
    EFFECT_RAINBOW_THEATER,         // 06  彩虹剧场追逐
    EFFECT_RAINBOW_WIPE,            // 07  彩虹擦除
    EFFECT_RAINBOW_WAVE,            // 08  彩虹波浪

    // 彩虹旋转类 (09-14)
    EFFECT_RAINBOW_ROTATE,          // 09  彩虹旋转
    EFFECT_RAINBOW_HEARTBEAT,       // 10  彩虹心跳
    EFFECT_RAINBOW_RIPPLE,          // 11  彩虹波纹
    EFFECT_RAINBOW_BREATHE_LIGHT,   // 12  彩虹呼吸灯
    EFFECT_RAINBOW_METEOR_SHOWER,   // 13  彩虹流星雨
    EFFECT_RAINBOW_VORTEX,          // 14  彩虹漩涡

    // 彩虹闪烁类 (15-20)
    EFFECT_RAINBOW_SPARKLE,         // 15  彩虹闪烁
    EFFECT_RAINBOW_CHASE,           // 16  彩虹追逐
    EFFECT_RAINBOW_GRADIENT_BREATH, // 17  彩虹渐变呼吸
    EFFECT_RAINBOW_PULSE_WAVE,      // 18  彩虹脉冲波
    EFFECT_RAINBOW_STROBE_FAST,     // 19  彩虹快闪
    EFFECT_RAINBOW_STROBE_SLOW,     // 20  彩虹慢闪

    // 彩虹流动类 (21-26)
    EFFECT_RAINBOW_BIDIRECTIONAL,   // 21  彩虹双向流
    EFFECT_RAINBOW_THREE_SEGMENT,   // 22  彩虹三段流
    EFFECT_RAINBOW_CENTER_SPREAD,   // 23  彩虹中心扩散
    EFFECT_RAINBOW_EDGE_COLLAPSE,   // 24  彩虹边缘收缩
    EFFECT_RAINBOW_DIAGONAL,        // 25  彩虹对角流
    EFFECT_RAINBOW_JUMP,            // 26  彩虹跳跃

    // 彩虹波形类 (27-32)
    EFFECT_RAINBOW_SLIDE,           // 27  彩虹滑行
    EFFECT_RAINBOW_PULSE_TRAIN,     // 28  彩虹脉冲串
    EFFECT_RAINBOW_BREATH_TRAIN,    // 29  彩虹呼吸串
    EFFECT_RAINBOW_SINE,            // 30  彩虹正弦波
    EFFECT_RAINBOW_SAWTOOTH,        // 31  彩虹锯齿波
    EFFECT_RAINBOW_SQUARE,          // 32  彩虹方波

    // 彩虹彗星类 (33-38)
    EFFECT_RAINBOW_COMET,           // 33  彩虹彗星
    EFFECT_RAINBOW_COMET_LONG,      // 34  彩虹彗星拖尾
    EFFECT_RAINBOW_METEOR,          // 35  彩虹流星
    EFFECT_RAINBOW_METEOR_LONG,     // 36  彩虹流星拖尾
    EFFECT_RAINBOW_DUAL_COMET,      // 37  彩虹双彗星
    EFFECT_RAINBOW_TRIPLE_COMET,    // 38  彩虹三彗星

    // 彩虹呼吸变体 (39-44)
    EFFECT_RAINBOW_PULSE_BREATH,    // 39  彩虹脉冲呼吸
    EFFECT_RAINBOW_FLICKER_BREATH,  // 40  彩虹闪烁呼吸
    EFFECT_RAINBOW_CHASE_BREATH,    // 41  彩虹追逐呼吸
    EFFECT_RAINBOW_WAVE_BREATH,     // 42  彩虹波浪呼吸
    EFFECT_RAINBOW_ROTATE_BREATH,   // 43  彩虹旋转呼吸
    EFFECT_RAINBOW_SPREAD_BREATH,   // 44  彩虹扩散呼吸

    // 彩虹特殊类 (45-50)
    EFFECT_RAINBOW_TRIANGLE,        // 45  彩虹三角波
    EFFECT_RAINBOW_RANDOM_BRIGHT,   // 46  彩虹随机亮度
    EFFECT_RAINBOW_GRADIENT_FLOW,   // 47  彩虹渐变流动
    EFFECT_RAINBOW_DOUBLE_HELIX,    // 48  彩虹双螺旋
    EFFECT_RAINBOW_TRIPLE_HELIX,    // 49  彩虹三螺旋
    EFFECT_RAINBOW_COLLAPSE_BREATH, // 50  彩虹收缩呼吸

    // 彩虹混合类 (51-56)
    EFFECT_RAINBOW_MIX_BREATH,      // 51  彩虹混合呼吸
    EFFECT_RAINBOW_MIX_FLOW,        // 52  彩虹混合流动
    EFFECT_RAINBOW_MIX_PULSE,       // 53  彩虹混合脉冲
    EFFECT_RAINBOW_MIX_FLICKER,     // 54  彩虹混合闪烁
    EFFECT_RAINBOW_MIX_ROTATE,      // 55  彩虹混合旋转
    EFFECT_RAINBOW_MIX_WAVE,        // 56  彩虹混合波浪

    // 彩虹终极类 (57-60)
    EFFECT_RAINBOW_ULTIMATE_FLICKER, // 57  彩虹终极闪烁
    EFFECT_RAINBOW_ULTIMATE_FLOW,    // 58  彩虹终极流动
    EFFECT_RAINBOW_ULTIMATE_PULSE,   // 59  彩虹终极脉冲
    EFFECT_RAINBOW_ULTIMATE_BREATH,  // 60  彩虹终极呼吸

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
