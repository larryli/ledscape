#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

// 使用 HSI 内部振荡器 + PLL 2x = 48MHz
#define FUNCONF_USE_HSI 1
#define FUNCONF_USE_PLL 1
#define FUNCONF_SYSTEM_CORE_CLOCK 48000000

// SysTick 使用 HCLK/8 = 6MHz (默认)
#define FUNCONF_SYSTICK_USE_HCLK 0

// 启用调试串口
#define FUNCONF_USE_DEBUGPRINTF 1

// DMA 缓冲区大小，必须 >= NR_LEDS，且为 4 的倍数
// 实际槽位 = DMALEDS/2，需 > NR_LEDS + WS2812B_RESET_PERIOD(2)，留余量避免 ISR 竞态
#define DMALEDS 68

#endif
