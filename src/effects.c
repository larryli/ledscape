/*
 * WS2812 灯效库 - 60 种效果实现
 *
 * 针对 1 米 30 颗灯珠、远距离观看优化
 * 动画节奏偏慢，色块偏大，对比度高
 */

#include "effects.h"
#include "color_utilities.h"
#include <string.h>

// 简易绝对值
static inline int abs(int x) { return x < 0 ? -x : x; }

/* ==================== 全局状态 ==================== */

EffectConfig g_effect_config;
EffectType g_current_effect;
uint32_t g_frame_count;

// 衰减缓冲区 (供部分效果使用)
static uint8_t tail_buf[NR_LEDS];

/* ==================== 辅助工具 ==================== */

// 简易伪随机 (LFSR)
static uint16_t prng_state = 0xACE1;
static uint8_t prng(void) {
    prng_state ^= prng_state << 7;
    prng_state ^= prng_state >> 9;
    prng_state ^= prng_state << 8;
    return (uint8_t)(prng_state & 0xFF);
}

/* ==================== 彩虹类 (01-08) ==================== */

// 01 彩虹流水
static uint32_t eff_rainbow_flow(int i) {
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, g_effect_config.brightness);
}

// 02 反向彩虹流水
static uint32_t eff_rainbow_flow_rev(int i) {
    return EHSVtoHEX(g_effect_config.hue_offset - i * 8, 255, g_effect_config.brightness);
}

// 03 彩虹呼吸
static uint32_t eff_rainbow_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 04 彩虹脉冲
static uint32_t eff_rainbow_pulse(int i) {
    uint8_t p = (uint8_t)((g_frame_count * 2) & 0xFF);
    uint8_t val = (p < 64) ? 255 : ((p < 128) ? 128 : 32);
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 05 彩虹弹跳
static uint32_t eff_rainbow_bounce(int i) {
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8 * g_effect_config.direction, 255, g_effect_config.brightness);
}

// 06 彩虹剧场追逐 (每3颗灯为一组，间隔熄灭)
static uint32_t eff_rainbow_theater(int i) {
    uint8_t group = (i + (g_frame_count / 4)) % 3;
    if (group != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 12, 255, g_effect_config.brightness);
}

// 07 彩虹擦除 (颜色从头到尾逐颗点亮，然后清除)
static uint32_t eff_rainbow_wipe(int i) {
    uint8_t pos = (g_frame_count / 3) % (NR_LEDS * 2);
    uint8_t led_hue = (i * 256 / NR_LEDS) & 0xFF;
    if (pos < NR_LEDS) {
        return (i <= pos) ? EHSVtoHEX(led_hue, 255, g_effect_config.brightness) : 0;
    } else {
        return (i > (pos - NR_LEDS)) ? 0 : EHSVtoHEX(led_hue, 255, g_effect_config.brightness);
    }
}

// 08 彩虹波浪 (正弦调制亮度的彩虹)
static uint32_t eff_rainbow_wave(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    uint8_t wave_pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = sintable[wave_pos];
    val = (val >> 2) + 64;  // 64-127 范围，保证可见
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 呼吸/脉冲类 (09-14) ==================== */

// 09 暖白呼吸 (色相=25 黄暖色)
static uint32_t eff_warm_white_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(25, 80, Mul8(g_effect_config.brightness, val));
}

// 10 冰蓝呼吸 (色相=160 青蓝)
static uint32_t eff_ice_blue_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(160, 200, Mul8(g_effect_config.brightness, val));
}

// 11 红色脉冲
static uint32_t eff_red_pulse(int i) {
    uint8_t p = (uint8_t)((g_frame_count * 2) & 0xFF);
    uint8_t val = (p < 64) ? 255 : 64;
    return EHSVtoHEX(0, 255, Mul8(g_effect_config.brightness, val));
}

// 12 绿色脉冲
static uint32_t eff_green_pulse(int i) {
    uint8_t p = (uint8_t)((g_frame_count * 2) & 0xFF);
    uint8_t val = (p < 64) ? 255 : 64;
    return EHSVtoHEX(85, 255, Mul8(g_effect_config.brightness, val));
}

// 13 蓝色脉冲
static uint32_t eff_blue_pulse(int i) {
    uint8_t p = (uint8_t)((g_frame_count * 2) & 0xFF);
    uint8_t val = (p < 64) ? 255 : 64;
    return EHSVtoHEX(170, 255, Mul8(g_effect_config.brightness, val));
}

// 14 彩色闪烁 (随机颜色快速切换)
static uint32_t eff_color_strobe(int i) {
    if ((g_frame_count & 0x03) != 0) return 0;
    return EHSVtoHEX(prng(), 255, g_effect_config.brightness);
}

/* ==================== 流动/追逐类 (15-20) ==================== */

// 15 彗星 (亮头 + 衰减尾)
static uint32_t eff_comet(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 16 反向彗星
static uint32_t eff_comet_rev(int i) {
    uint8_t head = NR_LEDS - 1 - ((g_frame_count / 2) % NR_LEDS);
    int dist = (i - head + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 17 流星雨 (多颗流星)
static uint32_t eff_meteor_shower(int i) {
    uint8_t phase = g_frame_count / 2;
    uint8_t val = 0;
    for (int m = 0; m < 3; m++) {
        uint8_t head = ((phase * (7 + m * 5)) + m * 11) % NR_LEDS;
        int dist = (head - i + NR_LEDS) % NR_LEDS;
        if (dist < 6) {
            uint8_t v = 255 - dist * 42;
            if (v > val) val = v;
        }
    }
    return EHSVtoHEX(g_effect_config.hue_offset + i * 4, 200, Mul8(g_effect_config.brightness, val));
}

// 18 颜色擦除 (单一颜色从头到尾填充)
static uint32_t eff_color_wipe(int i) {
    uint8_t pos = (g_frame_count / 3) % (NR_LEDS * 2);
    if (pos < NR_LEDS) {
        return (i <= pos) ? EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness) : 0;
    } else {
        return (i > (pos - NR_LEDS)) ? 0 : EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness);
    }
}

// 19 反向擦除
static uint32_t eff_color_wipe_rev(int i) {
    uint8_t pos = (g_frame_count / 3) % (NR_LEDS * 2);
    if (pos < NR_LEDS) {
        return (i >= (NR_LEDS - 1 - pos)) ? EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness) : 0;
    } else {
        return (i < (NR_LEDS * 2 - 1 - pos)) ? 0 : EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness);
    }
}

// 20 剧场追逐 (每3颗一组，每帧移动)
static uint32_t eff_theater_chase(int i) {
    uint8_t group = (i + (g_frame_count / 4)) % 3;
    if (group != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness);
}

/* ==================== 自然模拟类 (21-25) ==================== */

// 21 烛光摇曳 (暖色随机闪烁)
static uint32_t eff_candle_light(int i) {
    uint8_t flicker = prng();
    uint8_t val = 128 + (flicker >> 2);  // 128-191 范围
    return EHSVtoHEX(20, 200, Mul8(g_effect_config.brightness, val));
}

// 22 极光效果 (蓝绿紫缓慢交织)
static uint32_t eff_aurora(int i) {
    uint8_t hue = 120 + sintable[(i * 17 + g_frame_count) & 0xFF] / 4;
    uint8_t val = sintable[(i * 11 + g_frame_count * 2) & 0xFF];
    val = (val >> 2) + 64;
    return EHSVtoHEX(hue, 200, Mul8(g_effect_config.brightness, val));
}

// 23 日落渐变 (红橙黄暖色缓慢过渡)
static uint32_t eff_sunset(int i) {
    uint8_t base_hue = 0 + (g_frame_count / 8) % 30;  // 0-30 范围 (红→橙)
    uint8_t hue = base_hue + i * 2;
    return EHSVtoHEX(hue, 220, g_effect_config.brightness);
}

// 24 海浪效果 (蓝白色调波浪)
static uint32_t eff_ocean_wave(int i) {
    uint8_t wave = sintable[(i * 15 + g_frame_count * 2) & 0xFF];
    uint8_t hue = 160 + (wave / 16);  // 160-175 蓝色范围
    uint8_t val = (wave >> 2) + 80;
    return EHSVtoHEX(hue, 180, Mul8(g_effect_config.brightness, val));
}

// 25 火焰闪烁 (红橙色随机跳动)
static uint32_t eff_fire_flicker(int i) {
    uint8_t base = prng();
    uint8_t hue = (base < 128) ? 8 : 15;   // 8-15 (红橙)
    uint8_t val = 128 + (base >> 2);       // 128-191
    return EHSVtoHEX(hue, 240, Mul8(g_effect_config.brightness, val));
}

/* ==================== 特殊效果类 (26-30) ==================== */

// 26 随机闪烁 (稀疏白色闪烁点)
static uint32_t eff_sparkle(int i) {
    uint8_t base_hue = g_effect_config.hue_offset + i * 8;
    if (prng() < 20) {  // ~8% 概率白色闪烁
        return EHSVtoHEX(0, 0, g_effect_config.brightness);
    }
    return EHSVtoHEX(base_hue, 255, g_effect_config.brightness / 4);  // 底色较暗
}

// 27 雪花效果 (白色随机点亮)
static uint32_t eff_snow(int i) {
    uint8_t density = prng();
    if (density < 30) {
        return EHSVtoHEX(0, 0, 200);
    }
    return 0;
}

// 28 双色交替 (两种颜色交替排列)
static uint32_t eff_dual_color(int i) {
    if (i % 2 == 0) {
        return EHSVtoHEX(g_effect_config.hue_offset, 255, g_effect_config.brightness);
    } else {
        return EHSVtoHEX(g_effect_config.hue_offset + 128, 255, g_effect_config.brightness);
    }
}

// 29 三色旋转 (三种颜色循环移动)
static uint32_t eff_triple_color(int i) {
    uint8_t group = (i + (g_frame_count / 6)) % 3;
    uint8_t hue = g_effect_config.hue_offset + group * 85;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 30 渐变流水 (柔和渐变色流动)
static uint32_t eff_gradient_flow(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * (256 / NR_LEDS);
    uint8_t val = sintable[(i * 256 / NR_LEDS + g_frame_count * 2) & 0xFF];
    val = (val >> 2) + 64;
    return EHSVtoHEX(hue, 200, Mul8(g_effect_config.brightness, val));
}

/* ==================== 新增远距离效果 (31-50) ==================== */

// 31 彩虹螺旋 (色相随位置和时间同时变化)
static uint32_t eff_rainbow_spiral(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 12 + g_frame_count * 2;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 32 呼吸彩虹 (亮度呼吸 + 色相缓慢变化)
static uint32_t eff_breathing_rainbow(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    uint8_t hue = g_effect_config.hue_offset + i * 6 + g_frame_count / 4;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 33 彩虹彗星 (彗星效果 + 彩虹尾迹)
static uint32_t eff_comet_rainbow(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 34 烟花爆发 (从中心向两边扩散)
static uint32_t eff_fireworks(int i) {
    uint8_t center = NR_LEDS / 2;
    uint8_t burst_phase = (g_frame_count / 4) % NR_LEDS;
    int dist = abs(i - center);
    if (dist > burst_phase) return 0;
    uint8_t val = 255 - (burst_phase - dist) * 16;
    if (val < 32) val = 0;
    uint8_t hue = g_effect_config.hue_offset + i * 10;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 35 波浪爆发 (多波叠加)
static uint32_t eff_wave_burst(int i) {
    uint8_t wave1 = sintable[(i * 20 + g_frame_count * 3) & 0xFF];
    uint8_t wave2 = sintable[(i * 10 + g_frame_count * 5) & 0xFF];
    uint8_t combined = (wave1 + wave2) / 2;
    uint8_t val = (combined >> 2) + 64;
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 200, Mul8(g_effect_config.brightness, val));
}

// 36 颜色涟漪 (从中心向外扩散)
static uint32_t eff_color_ripple(int i) {
    uint8_t center = NR_LEDS / 2;
    uint8_t ripple = (g_frame_count / 2) % NR_LEDS;
    int dist = abs(i - center);
    if (dist > ripple) return 0;
    uint8_t val = 255 - (ripple - dist) * 32;
    if (val < 32) val = 0;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 37 脉冲环 (环形脉冲移动)
static uint32_t eff_pulse_ring(int i) {
    uint8_t ring_pos = (g_frame_count / 2) % NR_LEDS;
    int dist = abs(i - ring_pos);
    if (dist > 3) return 0;
    uint8_t val = 255 - dist * 80;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 38 双色渐变 (两色之间平滑过渡)
static uint32_t eff_two_color_gradient(int i) {
    uint8_t hue1 = g_effect_config.hue_offset;
    uint8_t hue2 = g_effect_config.hue_offset + 128;
    uint8_t blend = (uint8_t)((uint16_t)i * 256 / NR_LEDS);
    uint8_t hue = hue1 + (uint8_t)(((int16_t)(hue2 - hue1) * blend) >> 8);
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 39 彩虹流星 (流星 + 彩虹尾迹)
static uint32_t eff_rainbow_meteor(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    uint8_t hue = g_effect_config.hue_offset + dist * 20;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 40 双彗星 (两个彗星对向移动)
static uint32_t eff_dual_comet(int i) {
    uint8_t head1 = (g_frame_count / 2) % NR_LEDS;
    uint8_t head2 = NR_LEDS - 1 - head1;
    int dist1 = (head1 - i + NR_LEDS) % NR_LEDS;
    int dist2 = (i - head2 + NR_LEDS) % NR_LEDS;
    uint8_t val = 0;
    if (dist1 < 6) val = 255 - dist1 * 42;
    if (dist2 < 6) {
        uint8_t v = 255 - dist2 * 42;
        if (v > val) val = v;
    }
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 41 暖色呼吸 (红橙色呼吸)
static uint32_t eff_breathing_warm(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(15, 220, Mul8(g_effect_config.brightness, val));
}

// 42 冷色呼吸 (蓝紫色呼吸)
static uint32_t eff_breathing_cool(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(180, 200, Mul8(g_effect_config.brightness, val));
}

// 43 彩虹频闪 (彩虹色快速闪烁)
static uint32_t eff_rainbow_strobe(int i) {
    if ((g_frame_count & 0x07) != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, g_effect_config.brightness);
}

// 44 颜色追逐 (单色追逐，带尾迹)
static uint32_t eff_color_chase(int i) {
    uint8_t head = (g_frame_count / 3) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 5) return 0;
    uint8_t val = 255 - dist * 50;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 45 反向追逐
static uint32_t eff_color_chase_rev(int i) {
    uint8_t head = NR_LEDS - 1 - ((g_frame_count / 3) % NR_LEDS);
    int dist = (i - head + NR_LEDS) % NR_LEDS;
    if (dist > 5) return 0;
    uint8_t val = 255 - dist * 50;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 46 冷暖交替 (冷暖色交替排列，缓慢变化)
static uint32_t eff_warm_cool_alternate(int i) {
    uint8_t phase = (g_frame_count / 8) % 2;
    uint8_t hue;
    if ((i + phase) % 2 == 0) {
        hue = 20;  // 暖色 (红橙)
    } else {
        hue = 180; // 冷色 (蓝紫)
    }
    return EHSVtoHEX(hue, 200, g_effect_config.brightness);
}

// 47 彩虹淡入淡出 (整个灯带彩虹色淡入淡出)
static uint32_t eff_rainbow_fade(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(i * (256 / NR_LEDS), 255, Mul8(g_effect_config.brightness, val));
}

// 48 中心爆发 (从中心向外爆发)
static uint32_t eff_center_burst(int i) {
    uint8_t center = NR_LEDS / 2;
    uint8_t burst = (g_frame_count / 3) % NR_LEDS;
    int dist = abs(i - center);
    if (dist > burst) return 0;
    uint8_t val = 255 - dist * 20;
    if (val < 32) val = 0;
    return EHSVtoHEX(g_effect_config.hue_offset + dist * 10, 255, Mul8(g_effect_config.brightness, val));
}

// 49 边缘发光 (两端亮，中间暗)
static uint32_t eff_edge_glow(int i) {
    uint8_t center = NR_LEDS / 2;
    int dist_from_center = abs(i - center);
    uint8_t val = 255 - dist_from_center * 16;
    if (val < 32) val = 0;
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(g_effect_config.hue_offset, 200, Mul8(g_effect_config.brightness, Mul8(val, breath)));
}

// 50 彩虹弹跳呼吸 (弹跳 + 呼吸组合)
static uint32_t eff_rainbow_bounce_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8 * g_effect_config.direction, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 补充效果 (51-60) ==================== */

// 51 双彩虹流动 (两个彩虹对向移动)
static uint32_t eff_double_rainbow(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t blend = sintable[phase] / 2 + 64;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, blend));
}

// 52 静态彩虹 (固定彩虹，无动画)
static uint32_t eff_rainbow_static(int i) {
    return EHSVtoHEX(i * (256 / NR_LEDS), 255, g_effect_config.brightness);
}

// 53 彩虹追逐 (追逐 + 彩虹尾迹)
static uint32_t eff_color_chase_rainbow(int i) {
    uint8_t head = (g_frame_count / 3) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 6) return 0;
    uint8_t val = 255 - dist * 42;
    uint8_t hue = g_effect_config.hue_offset + dist * 20;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 54 暖光效果 (柔和暖色，无动画)
static uint32_t eff_warm_glow(int i) {
    return EHSVtoHEX(25, 150, g_effect_config.brightness);
}

// 55 冷光效果 (柔和冷色，无动画)
static uint32_t eff_cool_glow(int i) {
    return EHSVtoHEX(180, 150, g_effect_config.brightness);
}

// 56 反向彩虹螺旋
static uint32_t eff_rainbow_spiral_reverse(int i) {
    uint8_t hue = g_effect_config.hue_offset - i * 12 - g_frame_count * 2;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 57 彗星淡出 (彗星移动 + 逐渐变暗)
static uint32_t eff_comet_fade(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 10) return 0;
    uint8_t val = 255 - dist * 25;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, val));
}

// 58 交替脉冲 (两组灯交替闪烁)
static uint32_t eff_alternating_pulse(int i) {
    uint8_t phase = (g_frame_count / 8) % 2;
    uint8_t on = ((i + phase) % 2 == 0) ? 255 : 32;
    return EHSVtoHEX(g_effect_config.hue_offset, 255, Mul8(g_effect_config.brightness, on));
}

// 59 呼吸脉冲 (呼吸 + 脉冲组合)
static uint32_t eff_breathing_pulse(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = sintable[phase];
    if (val < 64) val = 32;
    else val = 255;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 60 反向彩虹弹跳
static uint32_t eff_rainbow_bounce_rev(int i) {
    return EHSVtoHEX(g_effect_config.hue_offset - i * 8 * g_effect_config.direction, 255, g_effect_config.brightness);
}

/* ==================== 效果分发表 ==================== */

typedef uint32_t (*EffectFunc)(int);

static const EffectFunc effect_table[EFFECT_COUNT] = {
    eff_rainbow_flow,       // 01
    eff_rainbow_flow_rev,   // 02
    eff_rainbow_breath,     // 03
    eff_rainbow_pulse,      // 04
    eff_rainbow_bounce,     // 05
    eff_rainbow_theater,    // 06
    eff_rainbow_wipe,       // 07
    eff_rainbow_wave,       // 08
    eff_warm_white_breath,  // 09
    eff_ice_blue_breath,    // 10
    eff_red_pulse,          // 11
    eff_green_pulse,        // 12
    eff_blue_pulse,         // 13
    eff_color_strobe,       // 14
    eff_comet,              // 15
    eff_comet_rev,          // 16
    eff_meteor_shower,      // 17
    eff_color_wipe,         // 18
    eff_color_wipe_rev,     // 19
    eff_theater_chase,      // 20
    eff_candle_light,       // 21
    eff_aurora,             // 22
    eff_sunset,             // 23
    eff_ocean_wave,         // 24
    eff_fire_flicker,       // 25
    eff_sparkle,            // 26
    eff_snow,               // 27
    eff_dual_color,         // 28
    eff_triple_color,       // 29
    eff_gradient_flow,      // 30
    eff_rainbow_spiral,     // 31
    eff_breathing_rainbow,  // 32
    eff_comet_rainbow,      // 33
    eff_fireworks,          // 34
    eff_wave_burst,         // 35
    eff_color_ripple,       // 36
    eff_pulse_ring,         // 37
    eff_two_color_gradient, // 38
    eff_rainbow_meteor,     // 39
    eff_dual_comet,         // 40
    eff_breathing_warm,     // 41
    eff_breathing_cool,     // 42
    eff_rainbow_strobe,     // 43
    eff_color_chase,        // 44
    eff_color_chase_rev,    // 45
    eff_warm_cool_alternate,// 46
    eff_rainbow_fade,       // 47
    eff_center_burst,       // 48
    eff_edge_glow,          // 49
    eff_rainbow_bounce_breath, // 50
    eff_double_rainbow,        // 51
    eff_rainbow_static,        // 52
    eff_color_chase_rainbow,   // 53
    eff_warm_glow,             // 54
    eff_cool_glow,             // 55
    eff_rainbow_spiral_reverse,// 56
    eff_comet_fade,            // 57
    eff_alternating_pulse,     // 58
    eff_breathing_pulse,       // 59
    eff_rainbow_bounce_rev,    // 60
};

/* ==================== 公共接口 ==================== */

void Effects_Init(void) {
    g_effect_config.hue_offset = 0;
    g_effect_config.speed = 128;
    g_effect_config.brightness = 255;
    g_effect_config.direction = 1;
    g_effect_config.mode = 0;
    g_current_effect = 0;
    g_frame_count = 0;
    memset(tail_buf, 0, sizeof(tail_buf));
}

void Effects_Next(void) {
    g_current_effect = (EffectType)((g_current_effect + 1) % EFFECT_COUNT);
}

void Effects_SwitchTo(EffectType effect) {
    if (effect < EFFECT_COUNT) g_current_effect = effect;
}

void Effects_Update(void) {
    g_frame_count++;

    // 色相偏移缓慢递增 (所有效果共享)
    g_effect_config.hue_offset += g_effect_config.speed / 32;
    if (g_effect_config.hue_offset == 0) g_effect_config.hue_offset = 1;

    // 弹跳效果：到达边界时反转
    if (g_current_effect == EFFECT_RAINBOW_BOUNCE ||
        g_current_effect == EFFECT_RAINBOW_BOUNCE_BREATH ||
        g_current_effect == EFFECT_RAINBOW_BOUNCE_REVERSE) {
        static uint8_t bounce_tick = 0;
        bounce_tick++;
        if (bounce_tick > 120) {
            g_effect_config.direction *= -1;
            bounce_tick = 0;
        }
    }
}

uint32_t Effects_CalculateLED(int ledno) {
    if (ledno < 0 || ledno >= NR_LEDS) return 0;
    return effect_table[g_current_effect](ledno);
}
