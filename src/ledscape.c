/*
 * CH32V003 WS2812 LEDScape 控制器
 *
 * 使用 SPI + DMA 驱动 WS2812B LED 灯带
 * 上电后自动洗牌随机切换灯效，每效果展示 3 秒
 *
 * 测试模式: make TEST_EFFECT=5  (只运行第5个效果)
 */

#include "ch32fun.h"
#include <stdio.h>

// 颜色顺序 (SK6812 用 WSGRB, WS2812B 用 WSRAW)
#define WSGRB

// 效果切换间隔: 3秒 × 60fps = 188帧
#define SWITCH_FRAMES 188

// WS2812B DMA 驱动
#define WS2812DMA_IMPLEMENTATION
#include "ws2812b_dma_spi_led_driver.h"

// 颜色工具函数
#include "color_utilities.h"

// 灯效算法
#include "effects.h"

/* ==================== 洗牌随机 ==================== */

#if !defined(TEST_EFFECT)

static uint8_t shuffle_order[EFFECT_COUNT];
static uint8_t shuffle_index;

static uint16_t rng_state = 0xBEEF;
static uint8_t rng(void) {
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return (uint8_t)(rng_state & 0xFF);
}

static void shuffle(void) {
    for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        shuffle_order[i] = i;
    }
    for (uint8_t i = EFFECT_COUNT - 1; i > 0; i--) {
        uint8_t j = rng() % (i + 1);
        uint8_t tmp = shuffle_order[i];
        shuffle_order[i] = shuffle_order[j];
        shuffle_order[j] = tmp;
    }
    shuffle_index = 0;
}

static void shuffle_next(void) {
    if (shuffle_index >= EFFECT_COUNT) {
        shuffle();
    }
    Effects_SwitchTo(shuffle_order[shuffle_index++]);
}

#endif /* !TEST_EFFECT */

/* WS2812B 颜色回调 (DMA ISR 中调用) */
uint32_t WS2812BLEDCallback(int ledno) {
    return Effects_CalculateLED(ledno);
}

/* 主程序 */
int main(void) {
    SystemInit();

    Effects_Init();
    WS2812BDMAInit();

#if defined(TEST_EFFECT)
    Effects_SwitchTo(TEST_EFFECT);
#else
    shuffle();
    uint16_t switch_counter = 0;
#endif

    while (1) {
        while (WS2812BLEDInUse);

        Effects_Update();
        WS2812BDMAStart(NR_LEDS);
        Delay_Ms(16);

#if !defined(TEST_EFFECT)
        if (++switch_counter >= SWITCH_FRAMES) {
            switch_counter = 0;
            shuffle_next();
        }
#endif
    }
}
