# zmk-module-peripheral-display

A ZMK module that drives a **peripheral-side** Sharp LS013B7DH03
128×128 monochrome memory LCD, showing the central side's keyboard
status in real time. No separate dongle required.

Inspired by [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)'s
scanner mode and [zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)'s
UI layout.

## Features

- Layer name (with auto-scroll for long names)
- Modifier icons (Windows or Mac style, configurable)
- HID indicators (caps word)
- Dual battery (central + peripheral)
- Output status (USB / BLE)
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
```

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
        compatible = "sharp,ls0xx";
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        serial-vcom-inversion;
    };
};
```

### 4. Configure Kconfig (optional)

```conf
# your config/<board>.conf
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM=n
```

## Using the third-party `sharp,ls0xx-vcom` driver (optional)

For DMA + hardware rotation + dual VCOM intervals. The display driver
is configured purely in devicetree — there is no Kconfig switch for it.

1. Add the driver to `west.yml`:
```yaml
   manifest:
     remotes:
       - name: tokyo2006
         url-base: https://github.com/tokyo2006
     projects:
       - name: zmk-ls0xxvcom-driver
         remote: tokyo2006
         revision: main
```
2. In your display overlay, change `compatible = "sharp,ls0xx-vcom";`
   and add `dma-mode;` + `rotate-180;`

## Manual hardware test (no automation)

1. Flash central with a keyboard + peripheral display firmware.
2. Flash peripheral with eyelash_nano + ls013b7dh03.
3. Check on peripheral screen:
   - [ ] Layer name appears within 1 s of split-connect
   - [ ] Layer change updates <200 ms
   - [ ] Modifier icons toggle on press
   - [ ] Bongo cat paws animate while typing
   - [ ] Both batteries render with correct %
   - [ ] Caps-word indicator toggles with caps-word
   - [ ] Output status changes on USB plug/unplug

## Known limitations

- nRF52840 only (eyelash_nano, nice!nano). nRF52832 (eyelash_nano_v2) NOT supported.
- No touch / no encoder — display-only.
- 1 peripheral ↔ 1 central (not multi-keyboard).
- ZMK main only (Zephyr 4.x). Not ZMK 0.3.

## Attribution

- Status struct (26 bytes) shape copied from
  [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)
  (MIT).
- Bongo cat animation frames from
  [englmaxi/zmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)
  (MIT). See `LICENSE-3RD-PARTY`.
- UI layout inspired by `zmk-dongle-display`.

## License

MIT. See `LICENSE`.
