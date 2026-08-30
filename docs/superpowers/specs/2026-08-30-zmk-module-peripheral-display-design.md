# zmk-module-peripheral-display — Design Spec

**Status:** Draft (pre-review)
**Date:** 2026-08-30
**Author:** brainstorming session between user and assistant
**Repo path:** `~/project/zmk-module-peripheral-display`
**License:** MIT

---

## 1. Purpose & Background

A ZMK module that drives a peripheral-half display (Sharp LS013B7DH03
128×128 monochrome memory LCD, with room for ST7789V 240×280 color later)
on the **peripheral side** of a split ZMK keyboard, **without** requiring
a separate dongle/MCU.

The display shows the **central's** keyboard status (layer name, modifiers,
WPM, battery, output, HID indicators), pushed from the central over the
existing ZMK split BLE peripheral-central GATT connection.

Inspired by the **prospector-zmk-module** Scanner Mode (v2.2.2) which uses a
separate dongle as a BLE observer. This module reuses prospector's
`struct zmk_status_adv_data` (26-byte advertisement payload) as the
forward-data format and reuses its event-driven push idea, but flips the
direction and eliminates the dongle.

### 1.1 Why this exists

- Prospector scanner needs a 3rd MCU as a BLE observer dongle — extra cost,
  extra USB port, extra battery/charge cycle.
- The peripheral half of a split keyboard already has a screen-capable MCU
  (nRF52840 in eyelash_nano / nice!nano). It cannot act as a BLE observer
  (radio-role conflict with ZMK split peripheral role), but it can render
  data forwarded by the central over the existing split connection.
- Several keyboards already pair a peripheral with an OLED (e.g. jm17's
  SSD1306) — this module replaces that pattern with a higher-density
  128×128 memory LCD showing rich status info.

### 1.2 Non-goals

- Not a dongle. Not a BLE observer. Not a generic display driver (uses
  Zephyr's built-in `sharp,ls0xx` binding).
- Not a fork of prospector-zmk-module. No code is reused; only the
  26-byte status struct shape and the event-driven update pattern are
  conceptually borrowed.
- Not a fork of zmk-dongle-display. Its widgets subscribe to local ZMK
  events that the peripheral doesn't see (keymap API is central-only).
  We write our own peripheral-side widgets.
- Not multi-keyboard monitoring. 1 peripheral ↔ 1 central.
- Not for nRF52832 (eyelash_nano_v2). Memory budget is too tight.

---

## 2. Scope

### 2.1 In scope (v0.1.0)

- nRF52840 boards: **eyelash_nano**, **nice!nano**
- Display: **Sharp LS013B7DH03** (128×128 mono) — primary
- ZMK **main** branch (Zephyr 4.x) only
- Display driver: Zephyr built-in `sharp,ls0xx` by default; opt-in for
  third-party `sharp,ls0xx-vcom` (tokyo2006/zmk-ls0xxvcom-driver fork)
- Split connection: standard ZMK split peripheral-central BLE
- One reference shield: `peripheral_lcd_ls013`
- 6 widgets: layer, modifiers, output, battery, HID indicators, WPM
- 3 layouts: Default / Minimal / WPM-focus
- Status struct format: reuse prospector's `zmk_status_adv_data` (26 bytes)

### 2.2 Deferred (post v0.1.0)

- ST7789V 240×280 color display support (`peripheral_lcd_st7789v`)
- Touch panel (CST816S) — only meaningful for ST7789V
- nrf52832 support (eyelash_nano_v2) — needs lighter widget impl
- Multi-keyboard display (already 1:1, future expansion)
- ZMK Studio integration
- ZMK 0.3 support

---

## 3. Architecture

### 3.1 Data flow diagram

```
┌─────────────────────────────┐      ┌─────────────────────────────┐
│  CENTRAL  (主手，eyelash_nano/nice_nano)
│  ├─ ZMK 键盘矩阵             │      │  PERIPHERAL  (副手)
│  ├─ Keymap + 层状态          │      │  ├─ ZMK 键盘矩阵
│  ├─ 已有 prospector 广播 │ BLE │  ├─ 已有 ZMK split 外设连接
│  │  (src/status_advertisement │ 通告│  │
│  │   .c，保留不动)           │ ←─→ │  ├─ 【新】status receiver
│  └─ 【新】status_forward     │      │  │  订阅来自 central 的 notify
│     ├─ 订阅 ZMK events │      │  │   把 26 字节 zmk_status_adv_data
│     ├─ 组装 zmk_status_adv   │      │  │   写进本地 shadow state
│     │  _data (26字节)         │      │  │
│     └─ 通过自定义 GATT        │      │  ├─ 【新】display widgets
│        characteristic notify │      │  │   从 shadow state 读取并渲染
│       推给 peripheral       │      │  │
└─────────────────────────────┘      │  └─ 【新】ls013b7dh03 渲染 └─────────────────────────────┘
```

### 3.2 Forward path (central)

```
[ZMK event] ──→ peripheral_status_forward_update() (debounced)
   │                    │
   │                    ├─ 重新打包 zmk_status_adv_data (26 字节)
   │                    │   ├─ layer (zmk_keymap_highest_layer_active)
   │                    │   ├─ battery (zmk_battery_state_of_charge)
   │                    │   ├─ wpm (zmk_wpm_get_state)
   │                    │   ├─ modifiers (zmk_hid_get_explicit_mods)
   │                    │   ├─ profile (zmk_ble_active_profile_index)
   │                    │   ├─ output (zmk_endpoints_selected)
   │                    │   ├─ peripheral_battery[] (zmk_split_get_peripheral_battery)
   │                    │   ├─ status_flags (caps word / charging / usb / ble)
   │                    │   └─ keyboard_id (HWINFO)
   │                    │
   │                    └─ bt_gatt_notify()  ──→ peripheral CCC
   │
   └── 1Hz 定时器兜底: 若 1 秒内无事件触发, 强制发一次心跳包
```

### 3.3 Receive path (peripheral)

```
[BLE notify 回调] ──→ peripheral_status_receiver_cb()
                          │
                          ├─ 解析 26 字节 → struct zmk_status_adv_data
                          ├─ 写入 shadow state (k_mutex 保护)
                          └─ LVGL timer (100ms) 轮询:
                                  ├─ 比较 shadow 与上次快照
                                  ├─ 变化字段 → 仅重绘对应 widget
                                  └─ LVGL dirty region 更新
```

### 3.4 Why GATT notify, not ZMK built-in split events

- ZMK split services are peripheral → central (key events, peripheral battery).
- We need central → peripheral (status data). No built-in mechanism.
- Independent custom GATT service avoids interfering with ZMK split stack.
- We reuse prospector's `zmk_status_adv_data` 26-byte struct so any future
  scanner can also read this format.

### 3.5 Trigger events & debounce

| 事件 | 触发 | Debounce | Reason |
|---|---|---|---|
| `zmk_layer_state_changed` | 用户切层 | 立即 | 用户感知敏感 |
| `zmk_modifiers_state_changed` | 按 modifier | 立即 | 立即 |
| `zmk_battery_state_changed` | 系统 | 30s | 变化慢 |
| `zmk_wpm_state_changed` | WPM 状态机 | 200ms | WPM 动画 |
| `zmk_output_selected_changed` | USB/BLE 切 | 立即 | 立即 |
| `zmk_activity_state_changed` | 活跃/空闲 | 立即 | 立即 |
| `zmk_endpoint_selection_changed` | endpoint 切 | 立即 | 立即 |
| `zmk_hid_indicators_state_changed` | CAPS/NUM/SCROLL | 立即 | 立即 |
| `zmk_position_state_changed` | 按键按下 | **不触发** | 由 WPM 派生 |
| 1Hz 心跳 | 定时 | — | 兜底 |

### 3.6 Bandwidth budget

- Per-packet: 26 bytes payload + ~7 bytes ATT header ≈ 33 bytes
- Per-event rate: layer/endpoint ~1Hz, modifier ~5Hz in heavy typing,
  WPM debounced to 200ms (5Hz)
- Worst-case event-driven peak: ~10 Hz (modifier + WPM concurrently) = ~330 B/s
- Plus 1 Hz heartbeat: +33 B/s
- **Total peak ≈ 360 B/s**, well below BLE notify throughput (~10 KB/s)
- Radio time: <0.1% — no perceptible impact on keystroke latency

---

## 4. Module structure

```
zmk-module-peripheral-display/
├── .github/
│   └── workflows/
│       └── build.yml                # CI: eyelash_nano + nice_nano matrix
│                                     # (note: author doesn't run CI locally;
│                                      #  this is for downstream users)
├── CMakeLists.txt
├── Kconfig
├── README.md
├── LICENSE                          # MIT
├── dts/
│   └── bindings/display/            # empty; uses Zephyr built-in binding
├── include/
│   └── zmk/
│       ├── peripheral_status.h      # struct + public API
│       └── peripheral_display.h     # widget init API
├── src/
│   ├── peripheral_status.c          # shared: pack/unpack + shadow state
│   ├── peripheral_status_forward.c  # central side
│   ├── peripheral_status_receiver.c # peripheral side
│   ├── peripheral_display.c         # LVGL widget init
│   └── widgets/
│       ├── peripheral_layer_status.{c,h}
│       ├── peripheral_modifiers.{c,h}
│       ├── peripheral_wpm_status.{c,h}
│       ├── peripheral_output_status.{c,h}
│       ├── peripheral_battery_status.{c,h}
│       ├── peripheral_hid_indicators.{c,h}
│       └── peripheral_central_name.{c,h}
├── tests/
│   ├── pack_unpack/
│   ├── shadow_state/
│   ├── debounce/
│   ├── forward_trigger/
│   └── boards/eyelash_nano_native_posix.conf
│       nice_nano_native_posix.conf
└── boards/
    └── shields/
        └── peripheral_lcd_ls013/
            ├── Kconfig.shield
            ├── Kconfig.defconfig
            ├── peripheral_lcd_ls013.overlay      # reference (xiao_ble pinout)
            ├── peripheral_lcd_ls013_nice_nano.overlay  # nice!nano pinout
            ├── peripheral_lcd_ls013.conf
            ├── boards/eyelash_nano.overlay       # board-specific (if needed)
            │   nice_nano.overlay
            └── CMakeLists.txt
```

### 4.1 Naming conventions

- Module: `zmk-module-peripheral-display`
- Shield: `peripheral_lcd_ls013` (display model bound; stackable with
  any keyboard shield via `shield:` list in build.yaml)
- Public API prefix: `peripheral_*` (avoids clash with prospector)
- Kconfig prefix: `ZMK_PERIPHERAL_DISPLAY_*`

---

## 5. Data format

### 5.1 Status struct (26 bytes, reused from prospector)

```c
struct zmk_status_adv_data {
    uint8_t manufacturer_id[2];     // 0xFF, 0xFF
    uint8_t service_uuid[2];        // 0xAB, 0xCD (Prospector UUID)
    uint8_t version;
    uint8_t battery_level;
    uint8_t active_layer;
    uint8_t profile_slot;
    uint8_t connection_count;
    uint8_t status_flags;
    uint8_t device_role;
    uint8_t device_index;
    uint8_t peripheral_battery[3];
    char layer_name[4];
    uint8_t keyboard_id[4];
    uint8_t modifier_flags;
    uint8_t wpm_value;
    uint8_t channel;
} __packed;
```

This struct is **redefined** in our module (not #include'd from prospector)
to avoid coupling. The format is identical by design.

### 5.2 GATT service UUIDs

```c
// Service: peripheral status forward
#define PERIPHERAL_STATUS_SERVICE_UUID  BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7b, 0x5e, 0xab, 0xcd)

// Characteristic: notify-only, 26 bytes payload
#define PERIPHERAL_STATUS_CHRC_UUID     BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7c, 0x5e, 0xab, 0xcd)
```

---

## 6. Display driver strategy

### 6.1 Default: Zephyr built-in `sharp,ls0xx`

- Compatible: `"sharp,ls0xx"`
- Built into Zephyr 4.x; zero extra deps
- Supports `serial-vcom-inversion` (required for LCD safety)
- Mono 1-bit rendering

### 6.2 Opt-in: third-party `sharp,ls0xx-vcom`

- Compatible: `"sharp,ls0xx-vcom"`
- From `tokyo2006/zmk-ls0xxvcom-driver` (user's fork of
  MickiusMousius/zmk-ls0xxvcom-driver)
- Adds: DMA batching (-99% CPU IRQ), hardware 180° rotation, dual VCOM
  intervals (active 17ms / idle 100ms), strict 50% duty cycle
- User must add to their west.yml manually
- We expose a Kconfig choice; users opt in by enabling the choice AND
  having the third-party module in their west.yml

### 6.3 Kconfig choice

```kconfig
choice ZMK_PERIPHERAL_DISPLAY_DRIVER
    prompt "LS013B7DH03 driver backend"
    default ZMK_PERIPHERAL_DISPLAY_DRIVER_BUILTIN
    help
      Select between Zephyr built-in sharp,ls0xx (zero deps) and the
      third-party sharp,ls0xx-vcom (DMA + rotation + dual VCOM).
      If you pick the third-party option you must also add
      tokyo2006/zmk-ls0xxvcom-driver to your west.yml.

config ZMK_PERIPHERAL_DISPLAY_DRIVER_BUILTIN
    bool "Zephyr built-in sharp,ls0xx"
    help
      No additional module dependencies required. Adequate for
      prototypes and low-volume use.

config ZMK_PERIPHERAL_DISPLAY_DRIVER_LS0XXVCOM
    bool "Third-party sharp,ls0xx-vcom (DMA + rotation + dual VCOM)"
    select ZMK_PERIPHERAL_DISPLAY_LS013_VCOM
    help
      Better performance, lower power. Requires
      tokyo2006/zmk-ls0xxvcom-driver in west.yml.
endchoice
```

### 6.4 Driver compatibility check

We do NOT call any driver-specific API directly. Widgets use Zephyr's
`struct display_driver_api` vtable. The driver choice affects only
the devicetree binding the user writes, not our C code.

---

## 7. UI design

### 7.1 Layouts

| Config | 内容 | 场景 |
|---|---|---|
| `LAYOUT_DEFAULT` (default) | 层名 + WPM + modifier + 输出 + 电量 | 通用 |
| `LAYOUT_MINIMAL` | 仅层名 + 输出 + 电量 | 低功耗 |
| `LAYOUT_WPM_FOCUS` | 大字 WPM + 层名 + modifier | 练习打字 |

### 7.2 Default layout (128×128 mono)

```
┌──────────────────────────┐
│ ▣ BLE  80%  QWERTY       │ ← 顶栏: output + battery + layer name
│                          │
│       WPM: 42            │ ← 中部: WPM（仅 active 时显示）
│                          │
│ ⌃⇧⌥⌘           🔋 78%    │ ← 底部: modifiers + battery
│                          │
└──────────────────────────┘
```

### 7.3 Widget list

| Widget | Data source | Default |
|---|---|---|
| `peripheral_layer_status` | `active_layer` + `layer_name[4]` | enabled |
| `peripheral_output_status` | `status_flags` USB/BLE bits | enabled |
| `peripheral_battery_status` | `battery_level` (central) + own battery | enabled |
| `peripheral_modifiers` | `modifier_flags` | enabled |
| `peripheral_hid_indicators` | `status_flags` CAPS_WORD bit | enabled |
| `peripheral_wpm_status` | `wpm_value` | opt-in |
| `peripheral_central_name` | `keyboard_id[4]` + stored name | opt-in |

### 7.4 Fonts

- Primary: `lv_font_unscii_8` (8 px, matches dongle-display)
- Large (WPM focus): `lv_font_unscii_16`
- No external font files (LVGL built-in only)

### 7.5 Memory budget

| Item | Size |
|---|---|
| Framebuffer (128×128 / 8) | 2 KB |
| LVGL vdb (100×100/8) | 1.25 KB |
| Widget object pool | ~8 KB |
| ZMK + LVGL base stack | ~32 KB |
| **Total estimate** | **~45 KB** |

nRF52840 has 256 KB RAM → comfortable headroom.
nRF52832 (eyelash_nano_v2) has 64 KB RAM → NOT supported.

### 7.6 Custom status screen entry

```c
// boards/shields/peripheral_lcd_ls013/*.c
#include <zmk/display.h>
#include "peripheral_display.h"

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    zmk_peripheral_display_init(screen);
    return screen;
}
```

---

## 8. Shield & GPIO design

### 8.1 Design principle: devicetree is the configuration

- The module does **NOT** bind specific GPIO numbers
- The module provides a **reference overlay** with comments explaining
  the SPI/GPIO nodes (all commented out by default)
- Users integrate into their existing peripheral keyboard shield by
  copying the relevant SPI display node and customizing GPIO numbers
- This avoids GPIO conflicts with existing keyboard shields

### 8.2 Integration patterns

**Pattern A: standalone reference shield** (validation / prototyping)
```yaml
# config/build.yaml
include:
  - board: eyelash_nano
    shield: peripheral_lcd_ls013
```

**Pattern B: stack onto existing peripheral keyboard shield** (production)
```yaml
# config/build.yaml
include:
  - board: eyelash_nano
    shield: my_peripheral_kb peripheral_lcd_ls013
```

```devicetree
/* config/boards/shields/my_peripheral_kb/my_peripheral_kb.overlay */
#include<dt-bindings/zmk/matrix_transform.h>

/ {
    chosen {
        zephyr,display = &ls013; /* enables our custom_status_screen */
    };
};

&spi1 {
    status = "okay";
    pinctrl-0 = <&spi1_default>;
    pinctrl-names = "default";
    cs-gpios = <&gpio0 31 GPIO_ACTIVE_LOW>; /* user picks free GPIO */
    ls013: ls013b7dh03@0 {
        compatible = "sharp,ls0xx";
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        /* Optional: serial-vcom-inversion; */
        /* Optional: disp-en-gpios = <...>; */
    };
};
```

### 8.3 Reference overlay content (commented out by default)

```devicetree
/* boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay */
/*
 * Reference overlay - all display wiring is commented out.
 * Users must uncomment + customize for their hardware.
 * GPIO numbers below are xiao_ble reference only.
 */

/ {
    chosen {
        zephyr,display = &ls013;
    };
};

// &spi1 {
//     status = "okay";
//     pinctrl-0 = <&spi1_default>;
//     pinctrl-names = "default";
//     cs-gpios = <&gpio0 9 GPIO_ACTIVE_LOW>; /* D9 */
//     ls013: ls013b7dh03@0 {
// compatible = "sharp,ls0xx";
//         reg = <0>;
// spi-max-frequency = <2000000>;
//         width = <128>;
//         height = <128>;
//         serial-vcom-inversion;
//     };
// };
```

### 8.4 Multi-board support

```
boards/shields/peripheral_lcd_ls013/
├── Kconfig.shield
├── Kconfig.defconfig
├── peripheral_lcd_ls013.overlay                # common (empty by design)
├── peripheral_lcd_ls013_eyelash_nano.overlay   # eyelash_nano pin reference
├── peripheral_lcd_ls013_nice_nano.overlay      # nice!nano pin reference
├── peripheral_lcd_ls013.conf
└── boards/
    ├── eyelash_nano.overlay
    └── nice_nano.overlay
```

ZMK shield's `boards/<board>.overlay` mechanism auto-picks the right
overlay per board.

---

## 9. Kconfig surface

```kconfig
# Top-level
config ZMK_PERIPHERAL_DISPLAY
    bool "Enable peripheral-side status display"
    default n
    select ZMK_DISPLAY
    select LVGL
    help      Enables the peripheral-side display module.

# Driver choice
choice ZMK_PERIPHERAL_DISPLAY_DRIVER
    default ZMK_PERIPHERAL_DISPLAY_DRIVER_BUILTIN
config ZMK_PERIPHERAL_DISPLAY_DRIVER_BUILTIN
    bool "Zephyr built-in sharp,ls0xx"
config ZMK_PERIPHERAL_DISPLAY_DRIVER_LS0XXVCOM
    bool "Third-party sharp,ls0xx-vcom"
    select ZMK_PERIPHERAL_DISPLAY_LS013_VCOM
endchoice

# Layout
config ZMK_PERIPHERAL_DISPLAY_LAYOUT
    int "Default layout"
    range 0 2
    default 0

# Widget toggles
config ZMK_PERIPHERAL_DISPLAY_WPM
    bool "Show WPM widget"
    default y

config ZMK_PERIPHERAL_DISPLAY_CENTRAL_NAME
    bool "Show central keyboard name"
    default n

# Central-side options
config ZMK_PERIPHERAL_STATUS_FORWARD
    bool "Enable central-side status forwarding"
    default y
    depends on ZMK_PERIPHERAL_DISPLAY && ZMK_SPLIT_ROLE_CENTRAL
```

---

## 10. Testing strategy

### 10.1 Test layers

| Layer | 内容 | Tool | Author can run? |
|---|---|---|---|
| L1 Build | 各 shield 在 eyelash_nano + nice_nano 上编译通过 | `west build` via downstream GitHub Actions | **NO** (author has no west env) |
| L2 Unit | pack/unpack / shadow / debounce / forward trigger | `native_posix` + ZMK test framework | **NO** (no west env) |
| L3 Hardware | 双板实测剧本 | Manual + USB serial logs | **NO** (no hardware) |
| L4 Render | ls013b7dh03 实际显示效果 | Photo / serial logs | **NO** (no hardware) |

### 10.2 Verification authority

- The module author **does NOT claim** tests pass.
- Build verification is delegated to **downstream user keyboard repos'**
  GitHub Actions (user creates GitHub repo for this module, then adds it
  to a test keyboard config repo and pushes to trigger Actions).
- Hardware verification is the **end user's responsibility**, performed
  against the manual script in the README.

### 10.3 Unit test scope

```
tests/
├── pack_unpack/
│   └── src/main.c          # 26-byte round-trip integrity
├── shadow_state/
│   └── src/main.c          # multi-thread concurrent read/write
├── debounce/
│   └── src/main.c          # 200ms / 30s / 1Hz timing
├── forward_trigger/
│   └── src/main.c          # simulate ZMK events, verify trigger frequency
└── boards/
    ├── eyelash_nano_native_posix.conf
    └── nice_nano_native_posix.conf
```

### 10.4 Manual integration test script (in README)

1. **Single-board self-test**: Flash `central+peripheral` dual-role firmware
   on xiao_ble (test only). Verify shadow state matches central state.
2. **Dual-board test**: Flash central (jm17) + peripheral (eyelash_nano +
   ls013b7dh03).
3. **Checklist**:
   - [ ] Boot within 5s shows "WAITING" placeholder on peripheral
   - [ ] Split connects → layer name displays within 1s
   - [ ] Layer switch: peripheral updates <200ms
   - [ ] Modifier hold: icon switches immediately
   - [ ] USB unplug: output status icon changes
   - [ ] WPM digit updates continuously
   - [ ] Disconnect central: peripheral shows "NO CENTRAL" within 2s
   - [ ] 1h soak: no memory leak (shadow size constant)

---

## 11. Risks

| ID | Risk | P | Impact | Mitigation |
|---|---|---|---|---|
| R1 | ls013b7dh03 SPI instability across batches | M | Display tearing | Lower SPI to 1MHz; add retry; keep dma-mode as advanced option |
| R2 | BLE notify bandwidth contention with ZMK split | L | Keystroke latency | 26B packets, debounced; peak ≈360 B/s; <0.1% radio time |
| R3 | GPIO conflict with peripheral keyboard matrix | M | Matrix dead | Devicetree is the config (Section 8); user must customize |
| R4 | Shadow state race condition | L | Tearing | k_mutex; unit-tested |
| R5 | VCOM not inverted → LCD damage | L (built-in 4.x) | Permanent damage | Default-on `serial-vcom-inversion`; Kconfig warn |
| R6 | nrf52832 memory insufficient | H | Won't run | **Not supported**; documented |
| R7 | Disconnect → blank screen confuses user | M | UX | "NO CENTRAL" placeholder |
| R8 | Naming clash with prospector | M | Confusion | Distinct module name + API prefix |
| R9 | dongle-display shield useless on peripheral | Confirmed N/A | — | Write our own widgets |
| R10 | Third-party driver fails to build on latest ZMK | M | Opt-in broken | Built-in is default; pin commit hash; Kconfig warn |
| R11 | Code pollution from prospector fork | L | Maintenance | Independent repo, zero code reuse |
| R12 | Users expect touch on mono display | L | Disappointment | README explicitly states no input |
| R13 | Build env claims cannot be verified | M | False success | Author explicitly does not claim pass; CI delegated |
| R14 | Hardware test cannot be automated | M | Late bug discovery | Manual script in README; checklist |

---

## 12. Out-of-scope / known limitations (README)

- Only nRF52840 supported (eyelash_nano, nice!nano). nRF52832 memory tight.
- No touch / no encoder (display-only).
- Shows **central** state, not own (by design).
- ZMK main only. Not ZMK 0.3.
- 1 peripheral ↔ 1 central. Not multi-keyboard.

---

## 13. Acceptance criteria for v0.1.0

- [ ] 5 unit test apps exist with native_posix configs
- [ ] Reference overlay exists for eyelash_nano + nice_nano
- [ ] README covers: install / build.yaml / overlay / Kconfig / layout /
       manual test script / known limitations
- [ ] LICENSE = MIT
- [ ] GitHub Actions workflow file exists (delegated to downstream
       keyboard repos for actual execution)
- [ ] Module pushed to GitHub
- [ ] At least one downstream keyboard config repo successfully builds
       using this module (verified by user running GitHub Actions)

---

## 14. Open questions

None. All decisions captured above.

---

## 15. References

- Prospector scanner (v2.2.2): https://github.com/t-ogura/zmk-config-prospector
  - `src/status_advertisement.c`, `src/status_scanner.c`,
    `include/zmk/status_advertisement.h`,
    `boards/shields/prospector_scanner/`
- ZMK display config: https://zmk.dev/docs/config/displays
- Zephyr LS0XX binding: `zephyr/dts/bindings/display/sharp,ls0xx.yaml`
- Zephyr LS0XX driver: `zephyr/drivers/display/ls0xx.c`
- tokyo2006/zmk-ls0xxvcom-driver: https://github.com/tokyo2006/zmk-ls0xxvcom-driver
- englmaxi/zmk-dongle-display (UI reference, NOT reused):
  https://github.com/englmaxi/zmk-dongle-display
- eyelash_nano board (v4.0 branch):
  https://github.com/a741725193/zmk-board-eyelash/tree/v4.0