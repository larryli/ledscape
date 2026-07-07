# LEDScape

基于 CH32V003 微控制器的风筝 LED 灯效控制器。

## 简介

利用 CH32V003 的 SPI + DMA 外设，以极低的 CPU 占用（~6%）驱动 WS2812B RGB LED 灯带，内置 60 种远距离优化灯效，上电自动洗牌随机播放。专为风筝载荷设计，总重量约 35g。

## 硬件

| 组件 | 规格 |
|------|------|
| 主控 | CH32V003 TSSOP-20 (RISC-V, 48MHz) |
| 灯带 | WS2812B × 30 颗，1 米 |
| 电池 | LiPo 601010 400mAh (3.7V, ~9g) |
| 数据引脚 | PC6 (SPI1 MOSI) |
| 供电方式 | 3.7V 锂电池直供 |
| 总重量 | ~35g |

## 特性

| 项目 | 参数 |
|------|------|
| 灯效数量 | 60 种 |
| 切换方式 | 洗牌随机，每效果 3 秒 |
| 驱动方式 | SPI + DMA 双缓冲 |
| CPU 占用 | ~6% |
| 帧率 | ~60 FPS |
| Flash 占用 | 7.3KB (44%) |
| RAM 占用 | 232B (11%) |
| 续航 | ~50 分钟 (混合播放) |

## 构建

```bash
cd src

# 正常编译 (60 个效果随机切换)
make

# 测试指定效果 (0-59)
make TEST_EFFECT=21    # 烛光摇曳
make TEST_EFFECT=15    # 彗星

# 烧录
make flash

# 清理
make clean
```

## 灯效列表 (60 种)

| 编号 | 名称 | 类别 |
|------|------|------|
| 01-08 | 彩虹流水/呼吸/脉冲/弹跳/剧场/擦除/波浪 | 彩虹 |
| 09-14 | 暖白/冰蓝呼吸、红/绿/蓝脉冲、彩色闪烁 | 呼吸/脉冲 |
| 15-20 | 彗星/流星雨/擦除/剧场追逐 | 流动 |
| 21-25 | 烛光/极光/日落/海浪/火焰 | 自然 |
| 26-30 | 闪烁/雪花/双色/三色/渐变 | 特殊 |
| 31-50 | 螺旋/彗星彩虹/烟花/涟漪/脉冲环等 | 远距离 |
| 51-60 | 双彩虹/静态彩虹/暖光/冷光等 | 补充 |

## 项目结构

```
ledscape/
├── README.md
├── LICENSE
├── .github/workflows/build.yml
├── docs/
│   ├── requirements.md       # 需求文档
│   ├── hardware.md           # 硬件设计
│   └── development.md        # 开发文档
└── src/
    ├── Makefile
    ├── funconfig.h
    ├── ledscape.c            # 主程序入口
    ├── effects.h             # 灯效接口
    ├── effects.c             # 60 种灯效实现
    ├── ws2812b_dma_spi_led_driver.h
    └── color_utilities.h
```

## 依赖

- **工具链**: riscv64-unknown-elf-gcc
- **SDK**: ch32fun
- **烧录器**: minichlink / WCH-LinkE

## 许可证

MIT License - Copyright (c) 2026 Larry Li
