# zmk-module-peripheral-display

[English](README.md)

一个驱动 **peripheral（副手）侧** Sharp LS013B7DH03 128×128 单色记忆液晶屏
的 ZMK 模块，实时显示 central（主手）侧的键盘状态。无需额外的 dongle。

灵感来自 [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)
的 scanner 模式和 [zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)
的 UI 布局。

## 功能

- 层名（长名称自动滚动）
- 修饰键图标（Windows / Mac 风格，可配置）
- HID 指示灯（caps word）
- 双电池（central + peripheral）
- 输出状态（USB / BLE）
- WPM 速度表（可选布局）
- Bongo cat 动画（随打字触发）

## 安装

### 1. 添加到键盘的 `west.yml`

```yaml
manifest:
  remotes:
    - name: tokyo2006
      url-base: https://github.com/tokyo2006
  projects:
    - name: zmk-module-peripheral-display
      remote: tokyo2006
      revision: main
    - name: zmk-ls0xxvcom-driver
      remote: tokyo2006
      revision: main
```

`zmk-ls0xxvcom-driver` 是默认使用的显示驱动（见下文
[显示驱动](#显示驱动)）。如果你更想用 Zephyr 内置驱动，可以省略它，
并参照那一节里的替代方案。

### 2. 添加到 `build.yaml`

```yaml
include:
  - board: eyelash_nano   # 或 nice_nano
    shield: your_peripheral_kb peripheral_lcd_ls013
```

### 3. 在 peripheral 键盘的 overlay 里配置显示

参考 overlay（`boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay`）
里所有显示接线**默认都被注释掉了**，以免和你的键盘矩阵产生 GPIO 冲突。
取消注释并按需修改：

```devicetree
/* your/boards/shields/your_peripheral_kb/your_peripheral_kb.overlay */
#include <dt-bindings/zmk/matrix_transform.h>

/ {
    chosen {
        zephyr,display = &ls013;
    };
};

&spi1 {
    status = "okay";
    pinctrl-0 = <&spi1_default>;
    pinctrl-names = "default";
    cs-gpios = <&gpio0 31 GPIO_ACTIVE_LOW>;   /* 选一个空闲 GPIO */
    ls013: ls013b7dh03@0 {
        compatible = "sharp,ls0xx-vcom";
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        /* VCOM 翻转 + DMA 批量传输 + 硬件旋转 */
        serial-vcom-inversion;
        serial-vcom-interval = <17>;
        idle-vcom-interval = <100>;
        dma-mode;
        rotate-180;
    };
};
```

> **关于 `rotate-180` 的说明：** 参考 overlay 默认启用了它，会在硬件层面
> 把画面翻转 180°。如果你的屏幕物理方向装反了，删掉 `rotate-180;` 这一行
> —— 否则画面会上下颠倒。

### 4. 配置 Kconfig（左右手都需要）

显示画面渲染在 **peripheral** 侧，但它显示的状态是由 **central** 侧产生的。
每一侧都需要几个配置项 —— 这些是最容易被遗漏的地方。

#### Peripheral（携带 `peripheral_lcd_ls013` 的那一侧）

shield 自带的 `peripheral_lcd_ls013.conf` 已经开启了
`CONFIG_ZMK_DISPLAY`、`CONFIG_ZMK_PERIPHERAL_DISPLAY`、接收端和各个 widget。
你只需要自己再加**一个**配置：

```conf
# config/<peripheral_kb>.conf
CONFIG_BT_GATT_CLIENT=y
```

peripheral 需要通过 GATT 订阅 central 的状态特征值，因此必须以 GATT
*客户端*角色来构建 —— 否则链接时 `bt_gatt_discover` / `bt_gatt_subscribe`
会未定义。

#### Central（连接电脑的那一侧）

central 会打包自身状态并通知给 peripheral。它需要模块开关，以及转发代码
读取的两个子系统：

```conf
# config/<central_kb>.conf
CONFIG_ZMK_PERIPHERAL_DISPLAY=y   # 开启状态转发
CONFIG_ZMK_WPM=y                  # 转发代码会调用 zmk_wpm_get_state()
CONFIG_ZMK_HID_INDICATORS=y       # 转发代码会监听 zmk_hid_indicators_changed
```

> central **不需要**自己的显示。在 central 上开启
> `CONFIG_ZMK_PERIPHERAL_DISPLAY` 只会打开 `ZMK_PERIPHERAL_STATUS_FORWARD`
> 路径 —— 它不会引入 `ZMK_DISPLAY` 或 LVGL（那些是由 peripheral 侧的 shield
> conf + `Kconfig.defconfig` 开启的）。

#### 可选的 widget 开关

```conf
# config/<peripheral_kb>.conf
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM=n
CONFIG_ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE_MAC=y   # Mac 风格修饰键图标
```

完整 widget 开关列表见 `Kconfig`。

## 显示驱动

默认驱动是第三方 [`sharp,ls0xx-vcom`](https://github.com/tokyo2006/zmk-ls0xxvcom-driver)，
它额外提供了：

- **DMA 批量传输**（`dma-mode`）—— 整帧在单次硬件 DMA 事务中发送，
  把 CPU 活动中断开销降低 99% 以上。
- **硬件 180° 旋转**（`rotate-180`）—— 原生翻转输出，几乎零开销，
  避免了软件旋转带来的 CPU/RAM 成本。
- **双 VCOM 间隔** —— 屏幕激活时刷新快（`serial-vcom-interval`，无闪烁），
  空闲时刷新慢（`idle-vcom-interval`，省电）。
- **平衡的 VCOM 翻转** —— 严格的 50% 占空比，防止直流偏置导致的
  电容积聚和永久性屏幕损坏。

该驱动完全通过 devicetree 配置 —— 没有任何对应的 Kconfig 开关。

### 替代方案：Zephyr 内置 `sharp,ls0xx`

如果不想引入额外的模块依赖，从 `west.yml` 里去掉
`zmk-ls0xxvcom-driver`，改用 Zephyr 内置驱动。把显示节点改成：

```devicetree
ls013: ls013b7dh03@0 {
    compatible = "sharp,ls0xx";
    reg = <0>;
    spi-max-frequency = <2000000>;
    width = <128>;
    height = <128>;
    serial-vcom-inversion;
};
```

ZMK main / Zephyr 4.x 已经在该驱动里支持了 VCOM 翻转；它只是缺少第三方
驱动的 DMA 批量传输、硬件旋转和双 VCOM 间隔。

## 手动硬件测试（无自动化）

1. 给 central 刷入「键盘 + peripheral display」固件。
2. 给 peripheral 刷入 eyelash_nano + ls013b7dh03。
3. 在 peripheral 屏幕上检查：
   - [ ] split 连接后 1 秒内出现层名
   - [ ] 切层后 <200ms 内更新
   - [ ] 按下修饰键时图标切换
   - [ ] 打字时 Bongo cat 爪子动起来
   - [ ] 两块电池都渲染出正确的百分比
   - [ ] caps-word 触发时指示灯切换
   - [ ] USB 插拔时输出状态变化

## 已知限制

- 仅支持 nRF52840（eyelash_nano、nice!nano）。不支持 nRF52832（eyelash_nano_v2）。
- 无触摸、无编码器 —— 纯显示。
- 1 个 peripheral ↔ 1 个 central（不支持多键盘）。
- 仅支持 ZMK main（Zephyr 4.x）。不支持 ZMK 0.3。

## 致谢

- 状态结构体（26 字节）形状复制自
  [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)
  （MIT）。
- Bongo cat 动画帧来自
  [englmaxi/zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)
  （MIT）。见 `LICENSE-3RD-PARTY`。
- UI 布局灵感来自 `zmk-dongle-display`。

## 许可证

MIT。见 `LICENSE`。
