# 开发文档

## 1. 开发环境

### 1.1 工具链

#### 交叉编译器
```bash
# Ubuntu/Debian
sudo apt install gcc-riscv64-unknown-elf
```

#### 烧录工具
```bash
# minichlink (推荐)
git clone https://github.com/cpq/minichlink
cd minichlink
make
```

### 1.2 SDK 依赖

```bash
git clone https://github.com/cpq/ch32fun.git ~/ch32fun
```

## 2. 项目结构

```
~/ledscape/
├── README.md
├── docs/
│   ├── requirements.md
│   └── development.md
└── src/
    ├── Makefile
    ├── funconfig.h
    ├── ledscape.c                  # 主程序入口
    ├── effects.h                   # 灯效接口定义
    ├── effects.c                   # 60 种灯效实现
    ├── ws2812b_dma_spi_led_driver.h
    └── color_utilities.h
```

## 3. 构建与烧录

### 3.1 正常编译

```bash
cd ~/ledscape/src
make
```

### 3.2 测试模式

```bash
# 只运行指定效果 (0-59)
make TEST_EFFECT=21    # 烛光摇曳
make TEST_EFFECT=15    # 彗星
make TEST_EFFECT=52    # 静态彩虹
```

测试模式会自动裁剪洗牌逻辑，节省 ~224B FLASH + 64B RAM。

### 3.3 烧录

```bash
make flash
```

### 3.4 清理

```bash
make clean
```

### 3.5 自定义参数

```bash
# 修改 LED 数量
make NR_LEDS=60

# 同时指定 LED 数量和测试效果
make NR_LEDS=60 TEST_EFFECT=5
```

## 4. 效果编号速查

| 编号 | 名称 | 类别 |
|------|------|------|
| 01 | 彩虹流水 | 彩虹 |
| 02 | 反向彩虹流水 | 彩虹 |
| 03 | 彩虹呼吸 | 彩虹 |
| 04 | 彩虹脉冲 | 彩虹 |
| 05 | 彩虹弹跳 | 彩虹 |
| 06 | 彩虹剧场追逐 | 彩虹 |
| 07 | 彩虹擦除 | 彩虹 |
| 08 | 彩虹波浪 | 彩虹 |
| 09 | 暖白呼吸 | 呼吸 |
| 10 | 冰蓝呼吸 | 呼吸 |
| 11 | 红色脉冲 | 脉冲 |
| 12 | 绿色脉冲 | 脉冲 |
| 13 | 蓝色脉冲 | 脉冲 |
| 14 | 彩色闪烁 | 闪烁 |
| 15 | 彗星 | 流动 |
| 16 | 反向彗星 | 流动 |
| 17 | 流星雨 | 流动 |
| 18 | 颜色擦除 | 流动 |
| 19 | 反向擦除 | 流动 |
| 20 | 剧场追逐 | 流动 |
| 21 | 烛光摇曳 | 自然 |
| 22 | 极光效果 | 自然 |
| 23 | 日落渐变 | 自然 |
| 24 | 海浪效果 | 自然 |
| 25 | 火焰闪烁 | 自然 |
| 26 | 随机闪烁 | 特殊 |
| 27 | 雪花效果 | 特殊 |
| 28 | 双色交替 | 特殊 |
| 29 | 三色旋转 | 特殊 |
| 30 | 渐变流水 | 特殊 |
| 31 | 彩虹螺旋 | 远距离 |
| 32 | 呼吸彩虹 | 远距离 |
| 33 | 彩虹彗星 | 远距离 |
| 34 | 烟花爆发 | 远距离 |
| 35 | 波浪爆发 | 远距离 |
| 36 | 颜色涟漪 | 远距离 |
| 37 | 脉冲环 | 远距离 |
| 38 | 双色渐变 | 远距离 |
| 39 | 彩虹流星 | 远距离 |
| 40 | 双彗星 | 远距离 |
| 41 | 暖色呼吸 | 远距离 |
| 42 | 冷色呼吸 | 远距离 |
| 43 | 彩虹频闪 | 远距离 |
| 44 | 颜色追逐 | 远距离 |
| 45 | 反向追逐 | 远距离 |
| 46 | 冷暖交替 | 远距离 |
| 47 | 彩虹淡入淡出 | 远距离 |
| 48 | 中心爆发 | 远距离 |
| 49 | 边缘发光 | 远距离 |
| 50 | 彩虹弹跳呼吸 | 远距离 |
| 51 | 双彩虹流动 | 补充 |
| 52 | 静态彩虹 | 补充 |
| 53 | 彩虹追逐 | 补充 |
| 54 | 暖光效果 | 补充 |
| 55 | 冷光效果 | 补充 |
| 56 | 反向彩虹螺旋 | 补充 |
| 57 | 彗星淡出 | 补充 |
| 58 | 交替脉冲 | 补充 |
| 59 | 呼吸脉冲 | 补充 |
| 60 | 反向彩虹弹跳 | 补充 |

## 5. 核心模块

### 5.1 主程序 (ledscape.c)

```c
int main(void) {
    SystemInit();
    Effects_Init();
    WS2812BDMAInit();

#if defined(TEST_EFFECT)
    Effects_SwitchTo(TEST_EFFECT);  // 测试模式：固定效果
#else
    shuffle();                       // 正常模式：初始洗牌
#endif

    while (1) {
        while (WS2812BLEDInUse);     // 等待 DMA 完成
        Effects_Update();             // 更新效果状态
        WS2812BDMAStart(NR_LEDS);    // 启动新帧
        Delay_Ms(16);                 // ~60 FPS
    }
}
```

### 5.2 灯效接口 (effects.h)

```c
// 初始化
void Effects_Init(void);

// 切换效果
void Effects_SwitchTo(EffectType effect);

// 更新状态 (每帧调用)
void Effects_Update(void);

// 计算 LED 颜色
uint32_t Effects_CalculateLED(int ledno);
```

### 5.3 WS2812B 驱动

```c
// 初始化
WS2812BDMAInit();

// 启动传输
WS2812BDMAStart(NR_LEDS);

// 状态查询
while (WS2812BLEDInUse);  // 等待完成
```

## 6. 添加新效果

### 6.1 在 effects.h 中添加枚举

```c
typedef enum {
    // ... 现有效果 ...
    EFFECT_MY_NEW_EFFECT,    // 新效果
    EFFECT_COUNT
} EffectType;
```

### 6.2 在 effects.c 中实现

```c
static uint32_t eff_my_new_effect(int i) {
    // i: LED 索引 (0-NR_LEDS)
    // 返回: 0x00RRGGBB 颜色值
    uint8_t hue = g_effect_config.hue_offset + i * 8;
    return EHSVtoHEX(hue, 255, g_effect_config.brightness);
}
```

### 6.3 添加到分发表

```c
static const EffectFunc effect_table[EFFECT_COUNT] = {
    // ... 现有效果 ...
    eff_my_new_effect,
};
```

## 7. 资源占用

| 效果数 | FLASH | RAM | 剩余 FLASH |
|--------|-------|-----|-----------|
| 10 | 3.0KB | 164B | 13KB |
| 30 | 4.7KB | 204B | 11.5KB |
| 50 | 6.6KB | 224B | 9.6KB |
| 60 | 7.3KB | 232B | 8.7KB |

Flash 还剩 8.7KB，可继续添加约 20-30 个效果。

## 8. 参考资源

- [CH32V003 数据手册](http://www.wch-ic.com/downloads/CH32V003DS0_PDF.html)
- [ch32fun SDK](https://github.com/cpq/ch32fun)
- [WS2812B 协议规范](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
