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
git clone https://github.com/cnlohr/ch32fun.git ~/ch32fun
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
    ├── effects.c                   # 60 种彩虹灯效实现
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
make TEST_EFFECT=0     # 彩虹流水
make TEST_EFFECT=3     # 彩虹呼吸
make TEST_EFFECT=11    # 彩虹波纹
make TEST_EFFECT=33    # 彩虹彗星
make TEST_EFFECT=57    # 彩虹终极闪烁
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
| 01-08 | 彩虹流水/呼吸/脉冲/弹跳/剧场/擦除/波浪 | 基础 |
| 09-14 | 旋转/心跳/波纹/呼吸灯/流星雨/漩涡 | 旋转 |
| 15-20 | 闪烁/追逐/渐变呼吸/脉冲波/快闪/慢闪 | 闪烁 |
| 21-26 | 双向流/三段流/中心扩散/边缘收缩/对角流/跳跃 | 流动 |
| 27-32 | 滑行/脉冲串/呼吸串/正弦波/锯齿波/方波 | 波形 |
| 33-38 | 彩虹彗星/长尾彗星/彩虹流星/长尾流星/双彗星/三彗星 | 彗星 |
| 39-44 | 脉冲呼吸/闪烁呼吸/追逐呼吸/波浪呼吸/旋转呼吸/扩散呼吸 | 呼吸 |
| 45-50 | 三角波/随机亮度/渐变流动/双螺旋/三螺旋/收缩呼吸 | 特殊 |
| 51-56 | 混合呼吸/混合流动/混合脉冲/混合闪烁/混合旋转/混合波浪 | 混合 |
| 57-60 | 终极闪烁/终极流动/终极脉冲/终极呼吸 | 终极 |

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

#### DMALEDS 与第一颗灯异常问题

ch32fun 的 SPI+DMA 驱动使用循环 DMA 缓冲区，ISR 在半传输/传输完成中断中重填缓冲区。**当缓冲区刚好装满整个帧数据时，ISR 在边界处与 DMA 传输产生竞态条件，导致第一颗灯颜色异常。**

根因：CH32V003 的 DMA1_IT_HT3 和 DMA1_IT_TC3 标志实际含义与预期相反（驱动源码注释已标注），导致 ISR 重填时机与 DMA 读取位置冲突。

**修复规则**：`DMALEDS` 必须满足 `DMALEDS / 2 > NR_LEDS + WS2812B_RESET_PERIOD`，留出 ≥2 槽余量。

| NR_LEDS | DMALEDS | 缓冲区槽位 | 实际占用 | 第一颗灯 |
|---------|---------|-----------|---------|---------|
| 8 | 32 | 16 | 10 (8+2) | 正常 |
| 30 | 32 | 16 | 32 (30+2) | **异常** (满载竞态) |
| 30 | 64 | 32 | 32 (30+2) | **异常** (满载竞态) |
| 30 | 68 | 34 | 32 (30+2) | 正常 |

**结论**：30 颗灯需 `DMALEDS=68`。RAM 从 328B 增至 ~520B（25%），仍在 2KB 限制内。

#### 编译注意事项

`NR_LEDS` 通过 Makefile 命令行传递时，**必须与烧录在同一条 make 命令中**，否则第二次 make 会使用默认值：

```bash
# 正确：一条命令
make NR_LEDS=8 flash

# 错误：分开执行会重新编译为默认 30 颗
make NR_LEDS=8
make flash  # ← 这里编译的是 NR_LEDS=30
```

## 6. 添加新效果

### 6.1 在 effects.h 中添加枚举

```c
typedef enum {
    // ... 现有 60 种彩虹效果 ...
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
| 60 (彩虹类) | 8.7KB | 544B | 7.3KB |

RAM 包含 DMALEDS=68 缓冲区（~384B）+ 效果状态。Flash 还剩 7.3KB，可继续添加效果。

## 8. 参考资源

- [CH32V003 数据手册](http://www.wch-ic.com/downloads/CH32V003DS0_PDF.html)
- [ch32fun SDK](https://github.com/cnlohr/ch32fun)
- [WS2812B 协议规范](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)
