/*
 * WS2812 灯效库 - 60 种彩虹效果实现
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

/* ==================== 彩虹基础类 (01-08) ==================== */

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

// 06 彩虹剧场追逐
static uint32_t eff_rainbow_theater(int i) {
    uint8_t group = (i + (g_frame_count / 4)) % 3;
    if (group != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 12, 255, g_effect_config.brightness);
}

// 07 彩虹擦除
static uint32_t eff_rainbow_wipe(int i) {
    uint8_t pos = (g_frame_count / 3) % (NR_LEDS * 2);
    uint8_t led_hue = (i * 256 / NR_LEDS) & 0xFF;
    if (pos < NR_LEDS) {
        return (i <= pos) ? EHSVtoHEX(led_hue, 255, g_effect_config.brightness) : 0;
    } else {
        return (i > (pos - NR_LEDS)) ? 0 : EHSVtoHEX(led_hue, 255, g_effect_config.brightness);
    }
}

// 08 彩虹波浪
static uint32_t eff_rainbow_wave(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    uint8_t wave_pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = sintable[wave_pos];
    val = (val >> 2) + 64;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹旋转类 (09-14) ==================== */

// 09 彩虹旋转 (色相随时间旋转)
static uint32_t eff_rainbow_rotate(int i) {
    uint8_t hue = g_effect_config.hue_offset + g_frame_count * 2;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 10 彩虹心跳 (双脉冲节奏)
static uint32_t eff_rainbow_heartbeat(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val;
    if (phase < 20) val = 255;
    else if (phase < 40) val = 100;
    else if (phase < 60) val = 200;
    else val = 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 11 彩虹波纹 (从两端向中心汇聚)
static uint32_t eff_rainbow_ripple(int i) {
    uint8_t pos1 = (g_frame_count / 3) % NR_LEDS;
    uint8_t pos2 = NR_LEDS - 1 - pos1;
    int dist1 = abs(i - pos1);
    int dist2 = abs(i - pos2);
    int dist = (dist1 < dist2) ? dist1 : dist2;
    if (dist > 4) return 0;
    uint8_t val = 255 - dist * 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 12 彩虹呼吸灯 (单色呼吸 + 色相缓慢旋转)
static uint32_t eff_rainbow_breathe_light(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    uint8_t hue = g_effect_config.hue_offset + g_frame_count / 8;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 13 彩虹流星雨 (多颗彩虹流星)
static uint32_t eff_rainbow_meteor_shower(int i) {
    uint8_t val = 0;
    for (int m = 0; m < 3; m++) {
        uint8_t head = ((g_frame_count / 2 * (7 + m * 5)) + m * 11) % NR_LEDS;
        int dist = (head - i + NR_LEDS) % NR_LEDS;
        if (dist < 6) {
            uint8_t v = 255 - dist * 42;
            if (v > val) val = v;
        }
    }
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 200, Mul8(g_effect_config.brightness, val));
}

// 14 彩虹漩涡 (螺旋状旋转)
static uint32_t eff_rainbow_vortex(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 12 + g_frame_count * 3;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

/* ==================== 彩虹闪烁类 (15-20) ==================== */

// 15 彩虹闪烁 (随机位置彩虹闪烁)
static uint32_t eff_rainbow_sparkle(int i) {
    uint8_t base_hue = g_effect_config.hue_offset + i * 8;
    if (prng() < 30) {
        return EHSVtoHEX(prng(), 255, g_effect_config.brightness);
    }
    return EHSVtoHEX(base_hue, 255, g_effect_config.brightness / 4);
}

// 16 彩虹追逐 (单色追逐 + 彩虹尾迹)
static uint32_t eff_rainbow_chase(int i) {
    uint8_t head = (g_frame_count / 3) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 6) return 0;
    uint8_t val = 255 - dist * 42;
    uint8_t hue = g_effect_config.hue_offset + dist * 20;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 17 彩虹渐变呼吸 (渐变色呼吸)
static uint32_t eff_rainbow_gradient_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    uint8_t hue = g_effect_config.hue_offset + i * (256 / NR_LEDS);
    return EHSVtoHEX(hue, 200, Mul8(g_effect_config.brightness, val));
}

// 18 彩虹脉冲波 (脉冲从一端到另一端)
static uint32_t eff_rainbow_pulse_wave(int i) {
    uint8_t pos = (g_frame_count / 2) % (NR_LEDS * 2);
    int dist;
    if (pos < NR_LEDS) {
        dist = abs(i - pos);
    } else {
        dist = abs(i - (NR_LEDS * 2 - 1 - pos));
    }
    if (dist > 3) return 0;
    uint8_t val = 255 - dist * 80;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 19 彩虹快闪 (快速闪烁)
static uint32_t eff_rainbow_strobe_fast(int i) {
    if ((g_frame_count & 0x03) != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, g_effect_config.brightness);
}

// 20 彩虹慢闪 (慢速闪烁)
static uint32_t eff_rainbow_strobe_slow(int i) {
    if ((g_frame_count & 0x0F) != 0) return 0;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, g_effect_config.brightness);
}

/* ==================== 彩虹流动类 (21-26) ==================== */

// 21 彩虹双向流 (两端同时向中心流动)
static uint32_t eff_rainbow_bidirectional(int i) {
    uint8_t pos1 = (g_frame_count / 3) % NR_LEDS;
    uint8_t pos2 = NR_LEDS - 1 - pos1;
    int dist1 = (pos1 - i + NR_LEDS) % NR_LEDS;
    int dist2 = (i - pos2 + NR_LEDS) % NR_LEDS;
    int dist = (dist1 < dist2) ? dist1 : dist2;
    if (dist > 6) return 0;
    uint8_t val = 255 - dist * 42;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 22 彩虹三段流 (三段彩虹同时移动)
static uint32_t eff_rainbow_three_segment(int i) {
    uint8_t seg = (i + (g_frame_count / 4)) % 3;
    uint8_t hue = g_effect_config.hue_offset + seg * 85;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 23 彩虹中心扩散 (从中心向外扩散)
static uint32_t eff_rainbow_center_spread(int i) {
    uint8_t center = NR_LEDS / 2;
    uint8_t spread = (g_frame_count / 3) % NR_LEDS;
    int dist = abs(i - center);
    if (dist > spread) return 0;
    uint8_t val = 255 - (spread - dist) * 16;
    if (val < 32) val = 0;
    return EHSVtoHEX(g_effect_config.hue_offset + dist * 10, 255, Mul8(g_effect_config.brightness, val));
}

// 24 彩虹边缘收缩 (从两端向中心收缩)
static uint32_t eff_rainbow_edge_collapse(int i) {
    uint8_t center = NR_LEDS / 2;
    int dist = abs(i - center);
    uint8_t collapse = (g_frame_count / 3) % center;
    if (dist > (center - collapse)) return 0;
    uint8_t val = 255 - dist * 16;
    if (val < 32) val = 0;
    return EHSVtoHEX(g_effect_config.hue_offset + dist * 10, 255, Mul8(g_effect_config.brightness, val));
}

// 25 彩虹对角流 (斜向流动效果)
static uint32_t eff_rainbow_diagonal(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * 8 + g_frame_count * 4;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 26 彩虹跳跃 (离散跳跃移动)
static uint32_t eff_rainbow_jump(int i) {
    uint8_t pos = ((g_frame_count / 4) * 3) % NR_LEDS;
    int dist = (pos - i + NR_LEDS) % NR_LEDS;
    if (dist > 5) return 0;
    uint8_t val = 255 - dist * 50;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹波形类 (27-32) ==================== */

// 27 彩虹滑行 (平滑移动)
static uint32_t eff_rainbow_slide(int i) {
    uint8_t pos = (g_frame_count / 2) % NR_LEDS;
    int dist = (pos - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 28 彩虹脉冲串 (连续脉冲)
static uint32_t eff_rainbow_pulse_train(int i) {
    uint8_t phase = (i * 8 + g_frame_count * 4) & 0xFF;
    uint8_t val = (phase < 128) ? 255 : 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 29 彩虹呼吸串 (连续呼吸)
static uint32_t eff_rainbow_breath_train(int i) {
    uint8_t phase = (i * 8 + g_frame_count * 2) & 0xFF;
    uint8_t val = (sintable[phase] >> 1) + 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 30 彩虹正弦波
static uint32_t eff_rainbow_sine(int i) {
    uint8_t pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = sintable[pos];
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 31 彩虹锯齿波
static uint32_t eff_rainbow_sawtooth(int i) {
    uint8_t pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = pos;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 32 彩虹方波
static uint32_t eff_rainbow_square(int i) {
    uint8_t pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = (pos < 128) ? 255 : 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹彗星类 (33-38) ==================== */

// 33 彩虹彗星 (彗星效果 + 彩虹尾迹)
static uint32_t eff_rainbow_comet(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 34 彩虹彗星拖尾 (长尾彗星)
static uint32_t eff_rainbow_comet_long(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 12) return 0;
    uint8_t val = 255 - dist * 21;
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 35 彩虹流星 (流星 + 彩虹尾迹)
static uint32_t eff_rainbow_meteor(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 8) return 0;
    uint8_t val = 255 - dist * 32;
    uint8_t hue = g_effect_config.hue_offset + dist * 20;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 36 彩虹流星拖尾 (长尾流星)
static uint32_t eff_rainbow_meteor_long(int i) {
    uint8_t head = (g_frame_count / 2) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 12) return 0;
    uint8_t val = 255 - dist * 21;
    uint8_t hue = g_effect_config.hue_offset + dist * 15;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, val));
}

// 37 彩虹双彗星 (两个彗星对向移动)
static uint32_t eff_rainbow_dual_comet(int i) {
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
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 38 彩虹三彗星 (三个彗星循环移动)
static uint32_t eff_rainbow_triple_comet(int i) {
    uint8_t val = 0;
    for (int m = 0; m < 3; m++) {
        uint8_t head = ((g_frame_count / 2 * (7 + m * 5)) + m * 11) % NR_LEDS;
        int dist = (head - i + NR_LEDS) % NR_LEDS;
        if (dist < 6) {
            uint8_t v = 255 - dist * 42;
            if (v > val) val = v;
        }
    }
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹呼吸变体 (39-44) ==================== */

// 39 彩虹脉冲呼吸 (脉冲 + 呼吸组合)
static uint32_t eff_rainbow_pulse_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = sintable[phase];
    if (val < 64) val = 32;
    else val = 255;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 40 彩虹闪烁呼吸 (闪烁 + 呼吸组合)
static uint32_t eff_rainbow_flicker_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t val = (sintable[phase] >> 1) + 64;
    if ((g_frame_count & 0x07) == 0) val = 255;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 41 彩虹追逐呼吸 (追逐 + 呼吸组合)
static uint32_t eff_rainbow_chase_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t head = (g_frame_count / 3) % NR_LEDS;
    int dist = (head - i + NR_LEDS) % NR_LEDS;
    if (dist > 6) return 0;
    uint8_t val = 255 - dist * 42;
    val = Mul8(val, breath);
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 42 彩虹波浪呼吸 (波浪 + 呼吸组合)
static uint32_t eff_rainbow_wave_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t wave_pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = sintable[wave_pos];
    val = Mul8(val, breath);
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 43 彩虹旋转呼吸 (旋转 + 呼吸组合)
static uint32_t eff_rainbow_rotate_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t hue = g_effect_config.hue_offset + g_frame_count * 2;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, breath));
}

// 44 彩虹扩散呼吸 (扩散 + 呼吸组合)
static uint32_t eff_rainbow_spread_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t center = NR_LEDS / 2;
    int dist = abs(i - center);
    uint8_t val = 255 - dist * 16;
    if (val < 32) val = 0;
    val = Mul8(val, breath);
    return EHSVtoHEX(g_effect_config.hue_offset + dist * 10, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹特殊类 (45-50) ==================== */

// 45 彩虹三角波
static uint32_t eff_rainbow_triangle(int i) {
    uint8_t pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t val = (pos < 128) ? pos * 2 : (255 - pos) * 2;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 46 彩虹随机亮度 (随机亮度变化)
static uint32_t eff_rainbow_random_bright(int i) {
    uint8_t val = 64 + (prng() >> 2);
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 47 彩虹渐变流动 (渐变色流动)
static uint32_t eff_rainbow_gradient_flow(int i) {
    uint8_t hue = g_effect_config.hue_offset + i * (256 / NR_LEDS) + g_frame_count * 2;
    return EHSVtoHEX(hue, 200, g_effect_config.brightness);
}

// 48 彩虹双螺旋 (双螺旋旋转)
static uint32_t eff_rainbow_double_helix(int i) {
    uint8_t hue1 = g_effect_config.hue_offset + i * 12 + g_frame_count * 2;
    uint8_t hue2 = g_effect_config.hue_offset + i * 12 + g_frame_count * 2 + 128;
    uint8_t hue = (i % 2 == 0) ? hue1 : hue2;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 49 彩虹三螺旋 (三螺旋旋转)
static uint32_t eff_rainbow_triple_helix(int i) {
    uint8_t seg = i % 3;
    uint8_t hue = g_effect_config.hue_offset + i * 12 + g_frame_count * 2 + seg * 85;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 50 彩虹收缩呼吸 (收缩 + 呼吸组合)
static uint32_t eff_rainbow_collapse_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t center = NR_LEDS / 2;
    int dist = abs(i - center);
    uint8_t collapse = (g_frame_count / 3) % center;
    if (dist > (center - collapse)) return 0;
    uint8_t val = 255 - dist * 16;
    if (val < 32) val = 0;
    val = Mul8(val, breath);
    return EHSVtoHEX(g_effect_config.hue_offset + dist * 10, 255, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹混合类 (51-56) ==================== */

// 51 彩虹混合呼吸 (多种效果混合)
static uint32_t eff_rainbow_mix_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t wave_pos = (i * 256 / NR_LEDS + g_frame_count * 3) & 0xFF;
    uint8_t wave = sintable[wave_pos];
    uint8_t val = Mul8(breath, wave);
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 52 彩虹混合流动 (多种流动混合)
static uint32_t eff_rainbow_mix_flow(int i) {
    uint8_t head1 = (g_frame_count / 2) % NR_LEDS;
    uint8_t head2 = NR_LEDS - 1 - head1;
    int dist1 = (head1 - i + NR_LEDS) % NR_LEDS;
    int dist2 = (i - head2 + NR_LEDS) % NR_LEDS;
    uint8_t val1 = (dist1 < 6) ? 255 - dist1 * 42 : 0;
    uint8_t val2 = (dist2 < 6) ? 255 - dist2 * 42 : 0;
    uint8_t val = (val1 > val2) ? val1 : val2;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 53 彩虹混合脉冲 (多种脉冲混合)
static uint32_t eff_rainbow_mix_pulse(int i) {
    uint8_t phase = (i * 8 + g_frame_count * 4) & 0xFF;
    uint8_t val1 = (phase < 128) ? 255 : 64;
    uint8_t val2 = sintable[phase];
    uint8_t val = (val1 + val2) / 2;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 54 彩虹混合闪烁 (多种闪烁混合)
static uint32_t eff_rainbow_mix_flicker(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    if ((g_frame_count & 0x07) == 0) breath = 255;
    uint8_t base_hue = g_effect_config.hue_offset + i * 8;
    if (prng() < 20) {
        return EHSVtoHEX(prng(), 255, Mul8(g_effect_config.brightness, breath));
    }
    return EHSVtoHEX(base_hue, 255, Mul8(g_effect_config.brightness / 4, breath));
}

// 55 彩虹混合旋转 (多种旋转混合)
static uint32_t eff_rainbow_mix_rotate(int i) {
    uint8_t hue1 = g_effect_config.hue_offset + g_frame_count * 2;
    uint8_t hue2 = g_effect_config.hue_offset + i * 12 + g_frame_count * 3;
    uint8_t hue = (i % 2 == 0) ? hue1 : hue2;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}

// 56 彩虹混合波浪 (多种波浪混合)
static uint32_t eff_rainbow_mix_wave(int i) {
    uint8_t wave1 = sintable[(i * 20 + g_frame_count * 3) & 0xFF];
    uint8_t wave2 = sintable[(i * 10 + g_frame_count * 5) & 0xFF];
    uint8_t combined = (wave1 + wave2) / 2;
    uint8_t val = (combined >> 2) + 64;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 200, Mul8(g_effect_config.brightness, val));
}

/* ==================== 彩虹终极类 (57-60) ==================== */

// 57 彩虹终极闪烁 (最复杂的闪烁效果)
static uint32_t eff_rainbow_ultimate_flicker(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath = (sintable[phase] >> 1) + 64;
    uint8_t wave = sintable[(i * 20 + g_frame_count * 3) & 0xFF];
    uint8_t val = Mul8(breath, wave);
    if ((g_frame_count & 0x03) == 0) val = 255;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 58 彩虹终极流动 (最复杂的流动效果)
static uint32_t eff_rainbow_ultimate_flow(int i) {
    uint8_t head1 = (g_frame_count / 2) % NR_LEDS;
    uint8_t head2 = NR_LEDS - 1 - head1;
    uint8_t head3 = (g_frame_count / 3) % NR_LEDS;
    int dist1 = (head1 - i + NR_LEDS) % NR_LEDS;
    int dist2 = (i - head2 + NR_LEDS) % NR_LEDS;
    int dist3 = (head3 - i + NR_LEDS) % NR_LEDS;
    uint8_t val1 = (dist1 < 6) ? 255 - dist1 * 42 : 0;
    uint8_t val2 = (dist2 < 6) ? 255 - dist2 * 42 : 0;
    uint8_t val3 = (dist3 < 6) ? 255 - dist3 * 42 : 0;
    uint8_t val = val1;
    if (val2 > val) val = val2;
    if (val3 > val) val = val3;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 59 彩虹终极脉冲 (最复杂的脉冲效果)
static uint32_t eff_rainbow_ultimate_pulse(int i) {
    uint8_t phase = (i * 8 + g_frame_count * 4) & 0xFF;
    uint8_t val1 = (phase < 128) ? 255 : 64;
    uint8_t val2 = sintable[phase];
    uint8_t val3 = (g_frame_count & 0x07) == 0 ? 255 : 128;
    uint8_t val = (val1 + val2 + val3) / 3;
    return EHSVtoHEX(g_effect_config.hue_offset + i * 8, 255, Mul8(g_effect_config.brightness, val));
}

// 60 彩虹终极呼吸 (最复杂的呼吸效果)
static uint32_t eff_rainbow_ultimate_breath(int i) {
    uint8_t phase = (uint8_t)(g_frame_count & 0xFF);
    uint8_t breath1 = (sintable[phase] >> 1) + 64;
    uint8_t breath2 = (sintable[(phase + 64) & 0xFF] >> 1) + 64;
    uint8_t breath = (breath1 + breath2) / 2;
    uint8_t hue = g_effect_config.hue_offset + i * 8 + g_frame_count / 4;
    return EHSVtoHEX(hue, 255, Mul8(g_effect_config.brightness, breath));
}

/* ==================== 效果分发表 ==================== */

typedef uint32_t (*EffectFunc)(int);

static const EffectFunc effect_table[EFFECT_COUNT] = {
    eff_rainbow_flow,           // 01
    eff_rainbow_flow_rev,       // 02
    eff_rainbow_breath,         // 03
    eff_rainbow_pulse,          // 04
    eff_rainbow_bounce,         // 05
    eff_rainbow_theater,        // 06
    eff_rainbow_wipe,           // 07
    eff_rainbow_wave,           // 08
    eff_rainbow_rotate,         // 09
    eff_rainbow_heartbeat,      // 10
    eff_rainbow_ripple,         // 11
    eff_rainbow_breathe_light,  // 12
    eff_rainbow_meteor_shower,  // 13
    eff_rainbow_vortex,         // 14
    eff_rainbow_sparkle,        // 15
    eff_rainbow_chase,          // 16
    eff_rainbow_gradient_breath,// 17
    eff_rainbow_pulse_wave,     // 18
    eff_rainbow_strobe_fast,    // 19
    eff_rainbow_strobe_slow,    // 20
    eff_rainbow_bidirectional,  // 21
    eff_rainbow_three_segment,  // 22
    eff_rainbow_center_spread,  // 23
    eff_rainbow_edge_collapse,  // 24
    eff_rainbow_diagonal,       // 25
    eff_rainbow_jump,           // 26
    eff_rainbow_slide,          // 27
    eff_rainbow_pulse_train,    // 28
    eff_rainbow_breath_train,   // 29
    eff_rainbow_sine,           // 30
    eff_rainbow_sawtooth,       // 31
    eff_rainbow_square,         // 32
    eff_rainbow_comet,          // 33
    eff_rainbow_comet_long,     // 34
    eff_rainbow_meteor,         // 35
    eff_rainbow_meteor_long,    // 36
    eff_rainbow_dual_comet,     // 37
    eff_rainbow_triple_comet,   // 38
    eff_rainbow_pulse_breath,   // 39
    eff_rainbow_flicker_breath, // 40
    eff_rainbow_chase_breath,   // 41
    eff_rainbow_wave_breath,    // 42
    eff_rainbow_rotate_breath,  // 43
    eff_rainbow_spread_breath,  // 44
    eff_rainbow_triangle,       // 45
    eff_rainbow_random_bright,  // 46
    eff_rainbow_gradient_flow,  // 47
    eff_rainbow_double_helix,   // 48
    eff_rainbow_triple_helix,   // 49
    eff_rainbow_collapse_breath,// 50
    eff_rainbow_mix_breath,     // 51
    eff_rainbow_mix_flow,       // 52
    eff_rainbow_mix_pulse,      // 53
    eff_rainbow_mix_flicker,    // 54
    eff_rainbow_mix_rotate,     // 55
    eff_rainbow_mix_wave,       // 56
    eff_rainbow_ultimate_flicker, // 57
    eff_rainbow_ultimate_flow,    // 58
    eff_rainbow_ultimate_pulse,   // 59
    eff_rainbow_ultimate_breath,  // 60
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
    if (g_current_effect == EFFECT_RAINBOW_BOUNCE) {
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
