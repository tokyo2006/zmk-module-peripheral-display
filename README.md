# zmk-module-peripheral-display

[中文文档](README.zh-CN.md)

A ZMK module that drives a **peripheral-side** Sharp LS013B7DH03
128×128 monochrome memory LCD, showing the central side's keyboard
status in real time. No separate dongle required.

Inspired by [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)'s
scanner mode and [zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)'s
UI layout.

## Features

- Layer name (with auto-scroll for long names)
- Modifier icons (Windows or Mac style, configurable), underlined while held
- HID indicators: Caps Lock / Num Lock, underlined while the host LED is on
- Dual battery (central + peripheral), each with a charge-level icon
- Output status: USB / BLE icons, a checkmark for HID-ready / bonded state,
  the active BLE profile slot number, and a bar marking which transport is
  actually selected
- WPM meter (optional layout)
- Bongo cat animation (reacts to typing)

## Installation

### 1. Add to your keyboard's `west.yml`

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

The `zmk-ls0xxvcom-driver` project is the display driver used by default
(see [Display driver](#display-driver) below). If you prefer the
Zephyr built-in driver instead, omit it and follow the alternative in
that section.

### 2. Add to `build.yaml`

```yaml
include:
  - board: eyelash_nano   # or nice_nano
    shield: your_peripheral_kb peripheral_lcd_ls013
```

### 3. Configure the display in your peripheral keyboard overlay

The reference overlay (`boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay`)
has all display wiring **commented out by design** so you don't get
GPIO conflicts with your keyboard matrix. Uncomment + customize:

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
    cs-gpios = <&gpio0 31 GPIO_ACTIVE_LOW>;   /* pick a free GPIO */
    ls013: ls013b7dh03@0 {
        compatible = "sharp,ls0xx-vcom";
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        /* VCOM inversion + DMA batching + native rotation */
        serial-vcom-inversion;
        serial-vcom-interval = <17>;
        idle-vcom-interval = <100>;
        dma-mode;
        rotate-180;
    };
};
```

> **Note on `rotate-180`:** the reference overlay enables it by default.
> It flips the image 180° in hardware. If your panel is physically wired
> the other way up, remove the `rotate-180;` line — otherwise the image
> will render upside down.

### 4. Add the split data channel node (required on **both** halves)

Status travels from central to peripheral over a dedicated BLE L2CAP
channel (ASDC — "arbitrary split data channel"), not GATT. It's enabled
purely by the presence of a devicetree node — there's no separate Kconfig
flag to flip. Add this node to **both** the central's and the
peripheral's overlay, with the **same** `channel-id` on each side:

```devicetree
/ {
    asdc0: asdc0 {
        compatible = "zmk,peripheral-display-asdc";
        channel-id = <1>;
        status = "okay";
    };
};
```

If this node is missing on either half, `CONFIG_ZMK_ARBITRARY_SPLIT_DATA_CHANNEL`
never turns on for that half and nothing will be sent/received — the
symptom is a peripheral screen stuck on "NO LINK" forever.

### 5. Configure Kconfig (required on both halves)

The display renders on the **peripheral** half, but the status it shows
is produced on the **central** half. Each half needs a couple of flags —
these are the most commonly missed bits.

#### Peripheral (the half carrying `peripheral_lcd_ls013`)

The shield's own `peripheral_lcd_ls013.conf` already turns on
`CONFIG_ZMK_DISPLAY`, `CONFIG_ZMK_PERIPHERAL_DISPLAY`, the receiver and
the widgets, and the ASDC node from step 4 turns on the L2CAP channel
itself — there's nothing else to add here.

#### Central (the half connected to the PC)

The central packs its state and sends it over the ASDC channel. It needs
the module gate plus the two subsystems the forward code reads:

```conf
# config/<central_kb>.conf
CONFIG_ZMK_PERIPHERAL_DISPLAY=y   # enables status forwarding
CONFIG_ZMK_WPM=y                  # forward code calls zmk_wpm_get_state()
CONFIG_ZMK_HID_INDICATORS=y       # forward code calls zmk_hid_indicators_get_current_profile()
```

> The central does **not** need a display of its own. Enabling
> `CONFIG_ZMK_PERIPHERAL_DISPLAY` on the central only turns on the
> `ZMK_PERIPHERAL_STATUS_FORWARD` path — it does not pull in `ZMK_DISPLAY`
> or LVGL (those are enabled on the peripheral by the shield's conf +
> `Kconfig.defconfig`).

#### Optional widget toggles

```conf
# config/<peripheral_kb>.conf
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM=n
CONFIG_ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE_MAC=y   # Mac modifier icons
```

See `Kconfig` for the full list of widget toggles.

## Display driver

The default driver is the third-party [`sharp,ls0xx-vcom`](https://github.com/tokyo2006/zmk-ls0xxvcom-driver),
which adds:

- **DMA batching** (`dma-mode`) — frames sent in a single hardware DMA
  transaction, cutting active CPU interrupt overhead by over 99%.
- **Hardware 180° rotation** (`rotate-180`) — flips output natively with
  near-zero overhead, avoiding software-rotation CPU/RAM cost.
- **Dual VCOM intervals** — fast while active (`serial-vcom-interval`,
  flicker-free) and slow while idle (`idle-vcom-interval`, saves battery).
- **Balanced VCOM inversion** — strict 50% duty cycle to prevent DC-bias
  capacitive buildup and permanent display damage.

The driver is configured purely in devicetree — there is no Kconfig
switch for it.

### Alternative: Zephyr built-in `sharp,ls0xx`

If you don't want the extra module dependency, omit
`zmk-ls0xxvcom-driver` from `west.yml` and use the Zephyr built-in
driver instead. Change the display node to:

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

ZMK main / Zephyr 4.x already supports VCOM inversion in this driver;
it simply lacks DMA batching, hardware rotation, and the dual VCOM
intervals of the third-party driver.

## Manual hardware test (no automation)

1. Flash central with a keyboard + peripheral display firmware.
2. Flash peripheral with eyelash_nano + ls013b7dh03.
3. Check on peripheral screen:
   - [ ] Layer name appears within 1 s of split-connect
   - [ ] Layer change updates <200 ms
   - [ ] Modifier icons + underline toggle on press
   - [ ] Bongo cat paws animate while typing
   - [ ] Both batteries render with correct % and icon fill level
   - [ ] CAP/NUM indicators + underline toggle with Caps/Num Lock
   - [ ] Output status changes on USB plug/unplug, selection bar tracks the
     active transport, BLE profile number matches the selected slot

## Known limitations

- nRF52840 only (eyelash_nano, nice!nano). nRF52832 (eyelash_nano_v2) NOT supported.
- No touch / no encoder — display-only.
- 1 peripheral ↔ 1 central (not multi-keyboard).
- ZMK main only (Zephyr 4.x). Not ZMK 0.3.

## Attribution

- Status struct (26 bytes) shape copied from
  [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)
  (MIT).
- Bongo cat animation frames, modifier/output-status/HID-ready icons, and
  the BLE profile number glyphs from
  [englmaxi/zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)
  (MIT). See `LICENSE-3RD-PARTY`.
- UI layout inspired by `zmk-dongle-display`. The battery icon is original
  artwork for this module.
- ASDC (arbitrary split data channel) transport integrated from
  [dmhuisma/zmk_arbitrary_split_data_channel](https://github.com/dmhuisma/zmk_arbitrary_split_data_channel)
  (MIT).

## License

MIT. See `LICENSE`.
