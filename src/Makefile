# CH32V003 WS2812 彩虹流水灯控制器
# 基于 ch32fun SDK

# ch32fun SDK 路径
CH32FUN_DIR ?= $(HOME)/ch32fun

# 目标名称
TARGET := ledscape

# 目标 MCU
TARGET_MCU ?= CH32V003

# LED 数量
NR_LEDS ?= 30

# 测试模式: make TEST_EFFECT=5  (只运行第5个效果，0-59)
ifdef TEST_EFFECT
    EXTRA_CFLAGS = -DNR_LEDS=$(NR_LEDS) -DTEST_EFFECT=$(TEST_EFFECT)
else
    EXTRA_CFLAGS = -DNR_LEDS=$(NR_LEDS)
endif

# 额外源文件
ADDITIONAL_C_FILES = effects.c

# 包含 ch32fun 构建系统
include $(CH32FUN_DIR)/ch32fun/ch32fun.mk

clean : cv_clean
