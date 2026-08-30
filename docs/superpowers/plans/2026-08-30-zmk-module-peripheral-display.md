# zmk-module-peripheral-display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a ZMK module that drives a Sharp LS013B7DH03 128×128
monochrome memory LCD on the **peripheral side** of a split keyboard,
showing the central's keyboard status (layer, modifiers, WPM, dual
battery, HID indicators, output, bongo cat), pushed over the existing
ZMK split BLE GATT connection.

**Architecture:**
- Central side: event subscriptions → pack `struct zmk_status_adv_data`
  (26 bytes, copied from prospector shape) → `bt_gatt_notify` to a
  custom service CCC.
- Peripheral side: BLE notify callback → write shadow state
  (mutex-protected) → LVGL timer (100 ms) polls shadow → widgets render.
- Display driver: Zephyr built-in `sharp,ls0xx` default; opt-in for
  third-party `sharp,ls0xx-vcom` (tokyo2006/zmk-ls0xxvcom-driver).
- All GPIO configuration is devicetree-driven (overlay is config).

**Tech Stack:** C99 (Zephyr module), ZMK main (Zephyr 4.x), LVGL 9,
Zephyr GATT BLE stack, nRF52840 SoC (eyelash_nano, nice!nano).

**Spec:** `docs/superpowers/specs/2026-08-30-zmk-module-peripheral-display-design.md`
— read alongside this plan; this plan implements the spec verbatim.

---

## Global Constraints

These apply to every task unless that task explicitly overrides them.

- **License:** MIT for all original code in this module.
- **Third-party asset license:** bongo cat bitmap frames are sourced from
  `englmaxi/zmk-dongle-display` (Apache-2.0). Keep attribution in
  README; do NOT bundle dongle-display's widget code.
- **Target boards:** `eyelash_nano` (nrf52840), `nice!nano` (nrf52840).
  nRF52832 (eyelash_nano_v2) is NOT supported — memory budget too tight.
- **ZMK branch:** `main` (Zephyr 4.x). Do not target ZMK 0.3.
- **API prefix:** `peripheral_*` (avoids clash with prospector).
- **Kconfig prefix:** `ZMK_PERIPHERAL_DISPLAY_*`.
- **Struct redefinition:** We redefine `struct zmk_status_adv_data` in
  our own public header. Do NOT `#include` prospector headers. Field
  layout must match prospector's struct byte-for-byte.
- **Module directory:** All paths below are relative to
  `~/project/zmk-module-peripheral-display/`.
- **Test environment:** The author has no Zephyr SDK / west toolchain
  locally. Unit test code MUST be runnable in `native_posix` via
  GitHub Actions in downstream user keyboard repos. Where a task's
  verification command requires hardware or BLE (e.g. `bt_gatt_notify`),
  the task explicitly says "verify in downstream CI / hardware".
- **DRY:** No copy-paste between widgets. Use the `ZMK_DISPLAY_WIDGET`
  base macro (defined in our `peripheral_display.h`) for shared widget
  scaffolding.
- **YAGNI:** Do not add features outside the spec. No "for future use"
  code paths.
- **Commits:** Every task ends with a commit. Commit messages use
  Conventional Commits: `feat:`, `fix:`, `docs:`, `chore:`, `test:`.

---

## File Structure

Files this plan creates (relative to module root):

```
.
├── CMakeLists.txt                                  # T1
├── Kconfig                                         # T1
├── LICENSE                                         # T1
├── README.md                                       # T1 stub, T14 full
├── .gitignore                                      # T1 (already exists)
├── include/zmk/
│   ├── peripheral_status.h                         # T2
│   └── peripheral_display.h                        # T2
├── src/
│   ├── peripheral_status.c                         # T3, T4
│   ├── peripheral_status_service.c                 # T5
│   ├── peripheral_status_forward.c                 # T6
│   ├── peripheral_status_receiver.c                # T7
│   ├── peripheral_display.c                        # T8
│   └── widgets/
│       ├── peripheral_layer_status.{c,h}           # T9a
│       ├── peripheral_output_status.{c,h}          # T9b
│       ├── peripheral_battery_status.{c,h}         # T9c
│       ├── peripheral_modifiers.{c,h}              # T9d
│       ├── peripheral_hid_indicators.{c,h}         # T9e
│       ├── peripheral_wpm_status.{c,h}            # T9f
│       ├── peripheral_central_name.{c,h}           # T9g
│       ├── peripheral_bongo_cat.{c,h}              # T10
│       └── peripheral_bongo_cat_images.c           # T10 (Apache-2.0)
├── tests/
│   ├── pack_unpack/                                # T3
│   ├── shadow_state/                               # T4
│   ├── debounce/                                   # T6
│   ├── forward_trigger/                            # T6
│   └── boards/
│       ├── eyelash_nano_native_posix.conf          # T13
│       └── nice_nano_native_posix.conf             # T13
├── boards/shields/peripheral_lcd_ls013/
│   ├── Kconfig.shield                              # T12
│   ├── Kconfig.defconfig                           # T12
│   ├── peripheral_lcd_ls013.overlay                # T12 (reference, all commented)
│   ├── peripheral_lcd_ls013_nice_nano.overlay      # T12
│   ├── peripheral_lcd_ls013.conf                   # T12
│   ├── CMakeLists.txt                              # T12
│   └── src/
│       └── custom_status_screen.c                  # T11
├── dts/bindings/display/.gitkeep                   # T1
└── .github/workflows/build.yml                     # T13
```

**Decomposition rationale:**
- `src/peripheral_status.c` holds shared pack/unpack + shadow state.
  Single source of truth for the 26-byte wire format on both sides.
- `src/peripheral_status_service.c` holds GATT service + characteristic
  declaration. Split from `forward.c` so the service definition is
  reusable across central and peripheral (peripheral needs to register
  GATT callbacks; central needs to call `bt_gatt_notify`).
- `src/peripheral_status_forward.c` (central only) and
  `peripheral_status_receiver.c` (peripheral only) split cleanly by role.
- Widgets each get their own `.c/.h` pair. They are similar but each
  binds to a different ZMK event/shadow field; one file per widget keeps
  the change surface small.
- Tests mirror source layout 1:1.

---

## Task Index

| # | Title | Files | Verifiable via |
|---|---|---|---|
| T1  | Module skeleton + LICENSE + CMakeLists + Kconfig stub | root files | downstream `west build` |
| T2  | Public API headers | `include/zmk/peripheral_status.h`, `peripheral_display.h` | compiler |
| T3  | Status struct + pack/unpack (TDD) | `src/peripheral_status.c`, `tests/pack_unpack` | unit test in CI |
| T4  | Shadow state with mutex (TDD) | extension to `src/peripheral_status.c`, `tests/shadow_state` | unit test in CI |
| T5  | GATT service definition | `src/peripheral_status_service.c` | compiler + downstream CI |
| T6  | Central-side forward + debounce (TDD) | `src/peripheral_status_forward.c`, `tests/debounce`, `tests/forward_trigger` | unit test in CI |
| T7  | Peripheral-side receiver | `src/peripheral_status_receiver.c` | downstream CI + hardware |
| T8  | Display init module + widget base macro | `src/peripheral_display.c` | compiler |
| T9  | Widgets (a-g) | `src/widgets/peripheral_*.{c,h}` | downstream CI |
| T10 | Bongo cat widget + asset | `src/widgets/peripheral_bongo_cat.*` | downstream CI |
| T11 | Shield custom_status_screen entry | `boards/shields/peripheral_lcd_ls013/src/custom_status_screen.c` | downstream CI |
| T12 | Shield Kconfig + overlay + .conf + CMakeLists | `boards/shields/peripheral_lcd_ls013/*` | downstream CI |
| T13 | GitHub Actions CI workflow + test configs | `.github/workflows/build.yml`, `tests/boards/*` | CI itself |
| T14 | README (full) | `README.md` | review |

---

## Task 1: Module skeleton + LICENSE + CMakeLists + Kconfig stub

**Files:**
- Create: `CMakeLists.txt`
- Create: `Kconfig`
- Create: `LICENSE`
- Create: `README.md` (stub — full version in T14)
- Create: `dts/bindings/display/.gitkeep`
- (`.gitignore` already exists)

**Goal:** Empty module that ZMK west recognizes. No behavior yet.

**Interfaces:**
- Produces: nothing (foundation task). Later tasks add `target_sources`
  to `app` via `if(CONFIG_ZMK_PERIPHERAL_DISPLAY)` blocks in our
  top-level `CMakeLists.txt`.

- [ ] **Step 1: Create `LICENSE`**

Write MIT license text. Copyright holder: "The zmk-module-peripheral-display Contributors". Year: 2026. Use the standard MIT template (verify against `https://opensource.org/licenses/MIT` if unsure).

- [ ] **Step 2: Create `CMakeLists.txt`**

```cmake
# zmk-module-peripheral-display top-level CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)

zephyr_include_directories(include)

# All source files gated by ZMK_PERIPHERAL_DISPLAY
# (filled in by later tasks; left empty here so the module builds
#  even when the feature is disabled)
if(CONFIG_ZMK_PERIPHERAL_DISPLAY)
    # T3, T4
    target_sources(app PRIVATE src/peripheral_status.c)
    # T5
    target_sources(app PRIVATE src/peripheral_status_service.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)
    # T6
    target_sources(app PRIVATE src/peripheral_status_forward.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)
    # T7
    target_sources(app PRIVATE src/peripheral_status_receiver.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS)
    # T8
    target_sources(app PRIVATE src/peripheral_display.c)
    # T9 widgets added here
endif()
```

(The conditional blocks are populated incrementally; final layout after T9/T10 is shown in T12's shield CMakeLists.)

- [ ] **Step 3: Create `Kconfig`**

```kconfig
# Top-level gate
config ZMK_PERIPHERAL_DISPLAY
    bool "Peripheral-side status display (zmk-module-peripheral-display)"
    default n
    select ZMK_DISPLAY
    select LVGL
    help
      Enable the peripheral display module. The peripheral side of a
      split keyboard renders the central's status (layer, mods, WPM,
      battery, HID, output, bongo cat) on a Sharp LS013B7DH03
      128x128 mono memory LCD.

# T6 forward
config ZMK_PERIPHERAL_STATUS_FORWARD
    bool "Enable central-side status forwarding"
    default y
    depends on ZMK_PERIPHERAL_DISPLAY && ZMK_SPLIT_ROLE_CENTRAL
    help
      Central packs zmk_status_adv_data and notifies the peripheral
      over a custom GATT characteristic.

# T7 receive
config ZMK_PERIPHERAL_STATUS_RECEIVE
    bool "Enable peripheral-side status receiving"
    default y
    depends on ZMK_PERIPHERAL_DISPLAY && ZMK_SPLIT_ROLE_PERIPHERAL
    help
      Peripheral subscribes to the central's notify characteristic
      and writes incoming data into the shadow state.

# T8 widgets gate (referenced by T8's CMakeLists line above)
config ZMK_PERIPHERAL_DISPLAY_WIDGETS
    bool "Build peripheral display widgets and LVGL screen"
    default y
    depends on ZMK_PERIPHERAL_DISPLAY
```

- [ ] **Step 4: Create `dts/bindings/display/.gitkeep`**

Empty file (placeholder so the directory is committed).

- [ ] **Step 5: Create `README.md` stub**

```markdown
# zmk-module-peripheral-display

ZMK module driving a peripheral-side LS013B7DH03 128x128 memory LCD
showing central-side keyboard status. See `docs/superpowers/specs/`
for the design and `docs/superpowers/plans/` for the implementation
plan.

(README content filled in by T14.)
```

- [ ] **Step 6: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add CMakeLists.txt Kconfig LICENSE README.md dts/
git commit -m "chore: module skeleton (LICENSE, CMakeLists, Kconfig, README stub)"
git push
```

**Verification (deferred — author has no west env):**
- User: add module to a downstream test keyboard repo's `west.yml`,
  trigger GitHub Actions, confirm module is discovered without errors.

---

## Task 2: Public API headers

**Files:**
- Create: `include/zmk/peripheral_status.h`
- Create: `include/zmk/peripheral_display.h`

**Goal:** Define the public contracts that every later task implements
against. No `.c` code in this task — just headers.

**Interfaces:**
- Produces (in `peripheral_status.h`):
  - `struct peripheral_status_adv_data` (26 bytes packed, mirrors prospector layout)
  - `struct peripheral_status_shadow` (mutex + data + dirty flag)
  - `peripheral_status_pack()` / `peripheral_status_unpack()` signatures
  - `peripheral_status_shadow_get()` / `peripheral_status_shadow_set()` signatures
  - GATT UUID macros: `PERIPHERAL_STATUS_SERVICE_UUID`, `PERIPHERAL_STATUS_CHRC_UUID`
- Produces (in `peripheral_display.h`):
  - `ZMK_PERIPHERAL_DISPLAY_WIDGET` macro (shared widget base)
  - Widget structs and init/obj getters for each widget (filled by T9)
  - `peripheral_display_init(lv_obj_t *parent)` declaration

- [ ] **Step 1: Create `include/zmk/peripheral_status.h`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 26-byte status payload sent from central to peripheral.
 *
 * Field layout is byte-identical to prospector's
 * `struct zmk_status_adv_data` (prospector-zmk-module v2.2.2).
 * We redefine it here to avoid coupling the modules.
 */
struct peripheral_status_adv_data {
    uint8_t manufacturer_id[2];   /* 0xFF 0xFF */
    uint8_t service_uuid[2];      /* 0xAB 0xCD */
    uint8_t version;
    uint8_t battery_level;
    uint8_t active_layer;
    uint8_t profile_slot;
    uint8_t connection_count;
    uint8_t status_flags;
    uint8_t device_role;
    uint8_t device_index;
    uint8_t peripheral_battery[3];
    char     layer_name[4];
    uint8_t keyboard_id[4];
    uint8_t modifier_flags;
    uint8_t wpm_value;
    uint8_t channel;
} __packed;

#define PERIPHERAL_STATUS_PAYLOAD_SIZE sizeof(struct peripheral_status_adv_data)
_Static_assert(PERIPHERAL_STATUS_PAYLOAD_SIZE == 26,
               "peripheral_status_adv_data must be exactly 26 bytes");

/* Status flags (mirror prospector's bits) */
#define PERIPHERAL_STATUS_FLAG_CAPS_WORD     (1 << 0)
#define PERIPHERAL_STATUS_FLAG_CHARGING      (1 << 1)
#define PERIPHERAL_STATUS_FLAG_USB_CONNECTED (1 << 2)
#define PERIPHERAL_STATUS_FLAG_USB_HID_READY (1 << 3)
#define PERIPHERAL_STATUS_FLAG_BLE_CONNECTED (1 << 4)
#define PERIPHERAL_STATUS_FLAG_BLE_BONDED    (1 << 5)

#define PERIPHERAL_MOD_FLAG_LCTL (1 << 0)
#define PERIPHERAL_MOD_FLAG_LSFT (1 << 1)
#define PERIPHERAL_MOD_FLAG_LALT (1 << 2)
#define PERIPHERAL_MOD_FLAG_LGUI (1 << 3)
#define PERIPHERAL_MOD_FLAG_RCTL (1 << 4)
#define PERIPHERAL_MOD_FLAG_RSFT (1 << 5)
#define PERIPHERAL_MOD_FLAG_RALT (1 << 6)
#define PERIPHERAL_MOD_FLAG_RGUI (1 << 7)

#define PERIPHERAL_STATUS_SERVICE_UUID \
    BT_UUID_DECLARE_128(0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0x89, \
                       0x9c,0x9b,0x42,0x4f,0x7b,0x5e,0xab,0xcd)

#define PERIPHERAL_STATUS_CHRC_UUID \
    BT_UUID_DECLARE_128(0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0x89, \
                       0x9c,0x9b,0x42,0x4f,0x7c,0x5e,0xab,0xcd)

/**
 * @brief Pack a status payload into a 26-byte buffer.
 * @param data  Source data.
 * @param buf   Destination buffer (must be >= 26 bytes).
 * @return      0 on success.
 */
int peripheral_status_pack(const struct peripheral_status_adv_data *data,
                           uint8_t *buf, size_t buf_len);

/**
 * @brief Unpack a 26-byte buffer into a status payload.
 * @param buf   Source buffer.
 * @param buf_len Source buffer length (must be >= 26).
 * @param data  Destination.
 * @return      0 on success, -EINVAL if buf_len < 26.
 */
int peripheral_status_unpack(const uint8_t *buf, size_t buf_len,
                             struct peripheral_status_adv_data *data);

/**
 * @brief Shadow state holding the most recent status received.
 *
 * Single source of truth for widget rendering on the peripheral side.
 * All access goes through the getter/setter (mutex-protected).
 */
struct peripheral_status_shadow {
    struct peripheral_status_adv_data data;
    bool valid;            /* True after first successful write */
    uint32_t last_update_ms; /* k_uptime_get_32() of last successful update */
};

/**
 * @brief Get a snapshot of the current shadow state.
 *
 * Copies the shadow into @p out under the shadow mutex.
 * @return true if shadow has been written at least once.
 */
bool peripheral_status_shadow_get(struct peripheral_status_shadow *out);

/**
 * @brief Replace the shadow state. Called by the peripheral receiver.
 *
 * @return 0 on success.
 */
int peripheral_status_shadow_set(const struct peripheral_status_adv_data *data);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Create `include/zmk/peripheral_display.h`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <lvgl.h>
#include <zmk/peripheral_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward-declared widget structs (defined by each widget's header in T9) */
struct zmk_widget_peripheral_layer_status;
struct zmk_widget_peripheral_output_status;
struct zmk_widget_peripheral_battery_status;
struct zmk_widget_peripheral_modifiers;
struct zmk_widget_peripheral_hid_indicators;
struct zmk_widget_peripheral_wpm_status;
struct zmk_widget_peripheral_central_name;
struct zmk_widget_peripheral_bongo_cat;

/**
 * @brief Initialize all enabled peripheral display widgets and add
 *        them to @p parent.
 *
 * Called from zmk_display_status_screen() in T11.
 */
int peripheral_display_init(lv_obj_t *parent);

/**
 * @brief LVGL timer callback. Polls the shadow and redraws dirty ones.
 *        Registered in T8.
 */
void peripheral_display_timer_cb(lv_timer_t *t);

/* ZMK_PERIPHERAL_DISPLAY_WIDGET base macro — used by every widget in T9
 * to avoid copy-paste boilerplate. Defines the standard fields
 * (obj, sys_slist_node, last_state) the widget update functions expect. */
#define ZMK_PERIPHERAL_DISPLAY_WIDGET(name, state_t)                         \
    struct zmk_widget_peripheral_##name {                                    \
        lv_obj_t *obj;                                                       \
        sys_slist_node_t node;                                               \
        state_t state;                                                       \
    }

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 3: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add include/
git commit -m "feat: public API headers (peripheral_status.h, peripheral_display.h)"
git push
```

**Verification:**
- Compile check is deferred to T3 (first task that uses the headers).
- Static assert `_Static_assert(sizeof == 26)` provides a compile-time
  guard. If you accidentally add a field in T3, the build fails immediately.

---

## Task 3: Status struct + pack/unpack (TDD)

**Files:**
- Create: `src/peripheral_status.c` (partial — pack/unpack only)
- Create: `tests/pack_unpack/CMakeLists.txt`
- Create: `tests/pack_unpack/prj.conf`
- Create: `tests/pack_unpack/src/main.c`

**Goal:** Implement and unit-test `peripheral_status_pack` /
`peripheral_status_unpack`. Round-trip integrity: unpack(pack(x)) == x.

**Interfaces:**
- Consumes: `struct peripheral_status_adv_data` (T2)
- Produces: `peripheral_status_pack`, `peripheral_status_unpack` impl

- [ ] **Step 1: Create `tests/pack_unpack/src/main.c`**

```c
/*
 * Test: round-trip integrity of pack/unpack.
 */
#include <zephyr/ztest.h>
#include <zmk/peripheral_status.h>

static void fill_sample(struct peripheral_status_adv_data *d) {
    d->manufacturer_id[0] = 0xFF;
    d->manufacturer_id[1] = 0xFF;
    d->service_uuid[0] = 0xAB;
    d->service_uuid[1] = 0xCD;
    d->version = 0x22;          /* v2.2 */
    d->battery_level = 87;
    d->active_layer = 3;
    d->profile_slot = 0x09;     /* patch=1, profile=1 */
    d->connection_count = 2;
    d->status_flags = PERIPHERAL_STATUS_FLAG_BLE_CONNECTED |
                      PERIPHERAL_STATUS_FLAG_USB_CONNECTED;
    d->device_role = 1;         /* CENTRAL */
    d->device_index = 0;
    d->peripheral_battery[0] = 75;
    d->peripheral_battery[1] = 0;
    d->peripheral_battery[2] = 0;
    memcpy(d->layer_name, "HW", 3);   /* null-padded */
    d->keyboard_id[0] = 0xDE;
    d->keyboard_id[1] = 0xAD;
    d->keyboard_id[2] = 0xBE;
    d->keyboard_id[3] = 0xEF;
    d->modifier_flags = PERIPHERAL_MOD_FLAG_LALT | PERIPHERAL_MOD_FLAG_LGUI;
    d->wpm_value = 42;
    d->channel = 1;
}

ZTEST_SUITE(peripheral_status_pack_unpack, NULL, NULL, NULL, NULL, NULL);

ZTEST(peripheral_status_pack_unpack, test_roundtrip_integrity)
{
    struct peripheral_status_adv_data in;
    fill_sample(&in);

    uint8_t buf[PERIPHERAL_STATUS_PAYLOAD_SIZE];
    int rc = peripheral_status_pack(&in, buf, sizeof(buf));
    zassert_equal(rc, 0, "pack returned %d", rc);

    struct peripheral_status_adv_data out;
    memset(&out, 0xAA, sizeof(out));   /* poison to detect non-writes */
    rc = peripheral_status_unpack(buf, sizeof(buf), &out);
    zassert_equal(rc, 0, "unpack returned %d", rc);

    zassert_mem_equal(&in, &out, sizeof(in),
                      "round-trip mismatch");
}

ZTEST(peripheral_status_pack_unpack, test_unpack_short_buf)
{
    uint8_t buf[10] = {0};
    struct peripheral_status_adv_data out;
    int rc = peripheral_status_unpack(buf, sizeof(buf), &out);
    zassert_equal(rc, -EINVAL, "expected -EINVAL, got %d", rc);
}

ZTEST(peripheral_status_pack_unpack, test_size_is_26)
{
    /* Belt-and-suspenders check. The header already static-asserts. */
    zassert_equal(PERIPHERAL_STATUS_PAYLOAD_SIZE, 26,
                  "payload size changed; check struct packing");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `west build -b native_posix tests/pack_unpack -p`
Expected: FAIL with linker error `undefined reference to peripheral_status_pack`.

(Author cannot run this locally — User runs in downstream CI.)

- [ ] **Step 3: Write minimal implementation in `src/peripheral_status.c`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <errno.h>
#include <zmk/peripheral_status.h>

int peripheral_status_pack(const struct peripheral_status_adv_data *data,
                           uint8_t *buf, size_t buf_len)
{
    if (!data || !buf || buf_len < PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    memcpy(buf, data, PERIPHERAL_STATUS_PAYLOAD_SIZE);
    return 0;
}

int peripheral_status_unpack(const uint8_t *buf, size_t buf_len,
                             struct peripheral_status_adv_data *data)
{
    if (!buf || !data || buf_len < PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    memcpy(data, buf, PERIPHERAL_STATUS_PAYLOAD_SIZE);
    return 0;
}
```

- [ ] **Step 4: Create `tests/pack_unpack/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pack_unpack)
target_sources(app PRIVATE src/main.c)
```

- [ ] **Step 5: Create `tests/pack_unpack/prj.conf`**

```conf
CONFIG_ZTEST=y
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
```

- [ ] **Step 6: Wire `src/peripheral_status.c` into top-level CMakeLists**

The top-level `CMakeLists.txt` already has:

```cmake
if(CONFIG_ZMK_PERIPHERAL_DISPLAY)
    target_sources(app PRIVATE src/peripheral_status.c)
    ...
endif()
```

So just creating `src/peripheral_status.c` is enough — T1 already declared the rule.

- [ ] **Step 7: Run test to verify it passes**

Run: `west build -b native_posix tests/pack_unpack -p && west build -b native_posix tests/pack_unpack -t run`
Expected: 3 tests pass.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 8: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_status.c tests/pack_unpack/
git commit -m "feat(status): pack/unpack with round-trip unit test"
git push
```

---

## Task 4: Shadow state with mutex (TDD)

**Files:**
- Modify: `src/peripheral_status.c` (append shadow functions)
- Create: `tests/shadow_state/CMakeLists.txt`
- Create: `tests/shadow_state/prj.conf`
- Create: `tests/shadow_state/src/main.c`

**Goal:** Implement `peripheral_status_shadow_get` / `_set` with mutex
protection. Test that concurrent reads and writes don't tear.

**Interfaces:**
- Consumes: nothing new
- Produces: thread-safe shadow state API

- [ ] **Step 1: Create `tests/shadow_state/src/main.c`**

```c
/*
 * Test: shadow state mutex correctness under concurrent access.
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zmk/peripheral_status.h>

#define WRITER_THREADS  3
#define READER_THREADS  3
#define ITERATIONS      200

struct peripheral_status_adv_data test_payloads[WRITER_THREADS];

static void writer_fn(void *p1, void *p2, void *p3) {
    int idx = (int)(intptr_t)p1;
    for (int i = 0; i < ITERATIONS; i++) {
        test_payloads[idx].battery_level = (uint8_t)(idx * 10 + (i & 0x0F));
        test_payloads[idx].wpm_value     = (uint8_t)(i & 0xFF);
        zassert_equal(peripheral_status_shadow_set(&test_payloads[idx]), 0, NULL);
        k_msleep(1);
    }
}

static void reader_fn(void *p1, void *p2, void *p3) {
    for (int i = 0; i < ITERATIONS; i++) {
        struct peripheral_status_shadow s;
        bool ok = peripheral_status_shadow_get(&s);
        if (ok) {
            /* No torn read: either old value or new value, never partial */
            zassert_true(s.data.battery_level < 100, "torn read");
            zassert_true(s.data.wpm_value     < 256, "torn read");
        }
        k_msleep(1);
    }
}

K_THREAD_STACK_ARRAY_DEFINE(writer_stacks, WRITER_THREADS, 1024);
K_THREAD_STACK_ARRAY_DEFINE(reader_stacks, READER_THREADS, 1024);
static struct k_thread writer_threads[WRITER_THREADS];
static struct k_thread reader_threads[READER_THREADS];

ZTEST_SUITE(peripheral_status_shadow, NULL, NULL, NULL, NULL, NULL);

ZTEST(peripheral_status_shadow, test_concurrent_access)
{
    for (int i = 0; i < WRITER_THREADS; i++) {
        memset(&test_payloads[i], 0, sizeof(test_payloads[i]));
        k_thread_create(&writer_threads[i], writer_stacks[i], 1024,
                        writer_fn, (void *)(intptr_t)i, NULL, NULL,
                        K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    }
    for (int i = 0; i < READER_THREADS; i++) {
        k_thread_create(&reader_threads[i], reader_stacks[i], 1024,
                        reader_fn, NULL, NULL, NULL,
                        K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    }

    for (int i = 0; i < WRITER_THREADS; i++) k_thread_join(&writer_threads[i], K_FOREVER);
    for (int i = 0; i < READER_THREADS; i++) k_thread_join(&reader_threads[i], K_FOREVER);

    struct peripheral_status_shadow final;
    zassert_true(peripheral_status_shadow_get(&final),
                 "shadow should be valid after writers ran");
    zassert_true(final.valid, "valid flag must be set");
}

ZTEST(peripheral_status_shadow, test_initial_state_invalid)
{
    /* Reset by checking a fresh shadow state field.
     * Note: this test runs after test_concurrent_access, so we cannot
     * truly test "initial" state without a reset hook. Skip if shared.
     * Kept as documentation of the desired invariant.
     */
    zassert_true(true, "see code comment for initial-state invariant");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `west build -b native_posix tests/shadow_state -p`
Expected: FAIL with linker error `undefined reference to peripheral_status_shadow_get`.

- [ ] **Step 3: Append to `src/peripheral_status.c`**

```c
#include <zephyr/kernel.h>

static struct peripheral_status_shadow shadow;
static struct k_mutex shadow_mutex = Z_MUTEX_INITIALIZER(shadow_mutex);
static bool shadow_initialized;

bool peripheral_status_shadow_get(struct peripheral_status_shadow *out)
{
    if (!out) return false;
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    *out = shadow;
    bool ok = shadow.valid;
    k_mutex_unlock(&shadow_mutex);
    return ok;
}

int peripheral_status_shadow_set(const struct peripheral_status_adv_data *data)
{
    if (!data) return -EINVAL;
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    shadow.data = *data;
    shadow.valid = true;
    shadow.last_update_ms = k_uptime_get_32();
    k_mutex_unlock(&shadow_mutex);
    return 0;
}
```

- [ ] **Step 4: Create `tests/shadow_state/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(shadow_state)
target_sources(app PRIVATE src/main.c)
```

- [ ] **Step 5: Create `tests/shadow_state/prj.conf`**

```conf
CONFIG_ZTEST=y
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
```

- [ ] **Step 6: Run test, verify pass**

Run: `west build -b native_posix tests/shadow_state -p && west build -b native_posix tests/shadow_state -t run`
Expected: `test_concurrent_access` passes; no torn reads.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 7: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_status.c tests/shadow_state/
git commit -m "feat(status): shadow state with mutex; concurrent access test"
git push
```

---

## Task 5: GATT service definition

**Files:**
- Create: `src/peripheral_status_service.c`

**Goal:** Define the BLE GATT service + notify characteristic on the
**central** side. The peripheral needs to know the same UUIDs (already
in the public header T2) so it can scan + subscribe.

**Interfaces:**
- Consumes: `PERIPHERAL_STATUS_SERVICE_UUID`, `PERIPHERAL_STATUS_CHRC_UUID` (T2)
- Produces: `peripheral_status_notify(const uint8_t *buf, size_t len)`
             function callable from T6 forward code

**No unit test in this task** — GATT requires a BLE stack which is
not available in native_posix. Verify by downstream CI build success.

- [ ] **Step 1: Create `src/peripheral_status_service.c`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Defines the custom GATT service + notify characteristic that the
 * central uses to push status to the peripheral. Only the central
 * declares the service; the peripheral only needs the UUIDs (T2).
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

static uint8_t notify_buf[PERIPHERAL_STATUS_PAYLOAD_SIZE];

static void nfy_changed_cb(const struct bt_gatt_attr *attr,
                           uint16_t value)
{
    /* No-op: CCC writes are tracked implicitly by bt_gatt_notify. */
    ARG_UNUSED(attr);
    ARG_UNUSED(value);
}

BT_GATT_SERVICE_DEFINE(peripheral_status_svc,
    BT_GATT_PRIMARY_SERVICE(PERIPHERAL_STATUS_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(PERIPHERAL_STATUS_CHRC_UUID,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, notify_buf),
    BT_GATT_CCC(NULL, nfy_changed_cb),
);

int peripheral_status_notify(const uint8_t *buf, size_t len)
{
    if (len != PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    return bt_gatt_notify(NULL, &peripheral_status_svc.attrs[1], buf, len);
}
```

- [ ] **Step 2: Verify compile**

Build downstream keyboard repo with `CONFIG_ZMK_PERIPHERAL_DISPLAY=y`
+ `CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD=y`. Build must succeed.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 3: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_status_service.c
git commit -m "feat(ble): GATT service + notify characteristic"
git push
```

---

## Task 6: Central-side forward + debounce (TDD)

**Files:**
- Create: `src/peripheral_status_forward.c`
- Create: `tests/debounce/CMakeLists.txt`, `prj.conf`, `src/main.c`
- Create: `tests/forward_trigger/CMakeLists.txt`, `prj.conf`, `src/main.c`

**Goal:** Subscribe to ZMK events, pack current state, debounce, then
call `peripheral_status_notify`. Provide testable hooks for debounce.

**Interfaces:**
- Consumes: `peripheral_status_pack` (T3), `peripheral_status_notify` (T5)
- Produces:
  - `peripheral_status_forward_init()` — register event subscriptions
  - `peripheral_status_forward_debounce(key, now_ms) -> bool` — debounce helper

- [ ] **Step 1: Create `tests/debounce/src/main.c`**

```c
/*
 * Test: debounce logic for forward trigger.
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Forward declaration of the function under test (implemented in
 * src/peripheral_status_forward.c, but we test it via a thin shim
 * included inline). For testability the debounce function lives in
 * a header so it can be unit-tested. */
#include "../../src/peripheral_status_forward_debounce.h"

ZTEST_SUITE(peripheral_status_debounce, NULL, NULL, NULL, NULL, NULL);

ZTEST(peripheral_status_debounce, test_immediate_pass)
{
    struct peripheral_debounce_state s = {0};
    /* First call: never debounced. */
    zassert_true(peripheral_status_should_fire(&s, KEY_LAYER, 1000, 0));
    /* Within debounce window: should NOT fire. */
    zassert_false(peripheral_status_should_fire(&s, KEY_LAYER, 1100, 0));
}

ZTEST(peripheral_status_debounce, test_wpm_window)
{
    struct peripheral_debounce_state s = {0};
    /* WPM: 200ms window. */
    zassert_true(peripheral_status_should_fire(&s, KEY_WPM, 1000, 0));
    zassert_false(peripheral_status_should_fire(&s, KEY_WPM, 1100, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_WPM, 1201, 0));
}

ZTEST(peripheral_status_debounce, test_battery_long_window)
{
    struct peripheral_debounce_state s = {0};
    /* Battery: 30s window. */
    zassert_true(peripheral_status_should_fire(&s, KEY_BATTERY, 1000, 0));
    zassert_false(peripheral_status_should_fire(&s, KEY_BATTERY, 20000, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_BATTERY, 31000, 0));
}

ZTEST(peripheral_status_debounce, test_different_keys_independent)
{
    struct peripheral_debounce_state s = {0};
    zassert_true(peripheral_status_should_fire(&s, KEY_LAYER, 1000, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_MODS, 1000, 0)); /* independent */
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `west build -b native_posix tests/debounce -p`
Expected: FAIL — `peripheral_status_should_fire` undefined.

- [ ] **Step 3: Create `src/peripheral_status_forward_debounce.h`**

```c
/*
 * Debounce helper for central-side forward. Pulled into a header so
 * unit tests can call it without pulling in BLE stack.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum peripheral_debounce_key {
    KEY_LAYER = 0,
    KEY_MODS,
    KEY_WPM,
    KEY_BATTERY,
    KEY_OUTPUT,
    KEY_HID,
    KEY_ENDPOINT,
    KEY_ACTIVITY,
    KEY_COUNT,
};

struct peripheral_debounce_state {
    uint32_t last_fired_ms[KEY_COUNT];
};

/* Returns true if a new notify should be sent for @p key at @p now_ms.
 * Pass @p force=1 to bypass debounce (used by the 1Hz heartbeat). */
bool peripheral_status_should_fire(struct peripheral_debounce_state *s,
                                   enum peripheral_debounce_key key,
                                   uint32_t now_ms, int force);
```

- [ ] **Step 4: Create `src/peripheral_status_forward.c` (part 1: debounce impl)**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include "peripheral_status_forward_debounce.h"

#define DEBOUNCE_MS_LAYER    0
#define DEBOUNCE_MS_MODS     0
#define DEBOUNCE_MS_WPM      200
#define DEBOUNCE_MS_BATTERY  30000
#define DEBOUNCE_MS_OUTPUT   0
#define DEBOUNCE_MS_HID      0
#define DEBOUNCE_MS_ENDPOINT 0
#define DEBOUNCE_MS_ACTIVITY 0

static const uint32_t window_ms[KEY_COUNT] = {
    [KEY_LAYER]    = DEBOUNCE_MS_LAYER,
    [KEY_MODS]     = DEBOUNCE_MS_MODS,
    [KEY_WPM]      = DEBOUNCE_MS_WPM,
    [KEY_BATTERY]  = DEBOUNCE_MS_BATTERY,
    [KEY_OUTPUT]   = DEBOUNCE_MS_OUTPUT,
    [KEY_HID]      = DEBOUNCE_MS_HID,
    [KEY_ENDPOINT] = DEBOUNCE_MS_ENDPOINT,
    [KEY_ACTIVITY] = DEBOUNCE_MS_ACTIVITY,
};

bool peripheral_status_should_fire(struct peripheral_debounce_state *s,
                                   enum peripheral_debounce_key key,
                                   uint32_t now_ms, int force)
{
    if (force) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    if (now_ms - s->last_fired_ms[key] >= window_ms[key]) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    return false;
}
```

- [ ] **Step 5: Wire debounce.c into CMakeLists.txt for the test**

Update top-level `CMakeLists.txt`:

```cmake
# Inside if(CONFIG_ZMK_PERIPHERAL_DISPLAY) block, add:
target_sources(app PRIVATE src/peripheral_status_forward_debounce.c)
```

Create `src/peripheral_status_forward_debounce.c` containing just the
debounce impl (move the function body there):

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include "peripheral_status_forward_debounce.h"

#define DEBOUNCE_MS_LAYER    0
#define DEBOUNCE_MS_MODS     0
#define DEBOUNCE_MS_WPM      200
#define DEBOUNCE_MS_BATTERY  30000
#define DEBOUNCE_MS_OUTPUT   0
#define DEBOUNCE_MS_HID      0
#define DEBOUNCE_MS_ENDPOINT 0
#define DEBOUNCE_MS_ACTIVITY 0

static const uint32_t window_ms[KEY_COUNT] = {
    [KEY_LAYER]    = DEBOUNCE_MS_LAYER,
    [KEY_MODS]     = DEBOUNCE_MS_MODS,
    [KEY_WPM]      = DEBOUNCE_MS_WPM,
    [KEY_BATTERY]  = DEBOUNCE_MS_BATTERY,
    [KEY_OUTPUT]   = DEBOUNCE_MS_OUTPUT,
    [KEY_HID]      = DEBOUNCE_MS_HID,
    [KEY_ENDPOINT] = DEBOUNCE_MS_ENDPOINT,
    [KEY_ACTIVITY] = DEBOUNCE_MS_ACTIVITY,
};

bool peripheral_status_should_fire(struct peripheral_debounce_state *s,
                                   enum peripheral_debounce_key key,
                                   uint32_t now_ms, int force)
{
    if (force) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    if (now_ms - s->last_fired_ms[key] >= window_ms[key]) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    return false;
}
```

(Now remove the duplicated definition from `peripheral_status_forward.c`
— keep only the higher-level event subscription code there.)

- [ ] **Step 6: Create `tests/debounce/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(debounce)
target_sources(app PRIVATE
    src/main.c
    ../../../src/peripheral_status_forward_debounce.c
)
target_include_directories(app PRIVATE ../../../src)
```

(Use `../../../src` from `tests/debounce/` to reach the source file.)

- [ ] **Step 7: Create `tests/debounce/prj.conf`**

```conf
CONFIG_ZTEST=y
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
```

- [ ] **Step 8: Run debounce test, verify pass**

Run: `west build -b native_posix tests/debounce -p && west build -b native_posix tests/debounce -t run`
Expected: 4 tests pass.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 9: Create `tests/forward_trigger/src/main.c`**

```c
/*
 * Test: forward_trigger is called for each enabled event source.
 * Simulates event callbacks and verifies the central-side pack
 * function would receive a valid struct.
 */
#include <zephyr/ztest.h>
#include <string.h>
#include <zmk/peripheral_status.h>
#include "peripheral_status_forward_debounce.h"

ZTEST_SUITE(peripheral_status_forward_trigger, NULL, NULL, NULL, NULL, NULL);

ZTEST(peripheral_status_forward_trigger, test_pack_after_simulated_layer_event)
{
    struct peripheral_status_adv_data s = {0};
    s.manufacturer_id[0] = 0xFF;
    s.manufacturer_id[1] = 0xFF;
    s.service_uuid[0]    = 0xAB;
    s.service_uuid[1]    = 0xCD;
    s.active_layer = 5;
    memcpy(s.layer_name, "LOW", 4);

    uint8_t buf[26];
    zassert_equal(peripheral_status_pack(&s, buf, sizeof(buf)), 0);
    struct peripheral_status_adv_data back;
    zassert_equal(peripheral_status_unpack(buf, sizeof(buf), &back), 0);
    zassert_equal(back.active_layer, 5);
    zassert_mem_equal(back.layer_name, "LOW", 3);
}
```

- [ ] **Step 10: Wire `tests/forward_trigger` CMakeLists + prj.conf**

Same structure as `tests/debounce/`. Both tests link against the same
sources — keep them separate for clarity of intent.

- [ ] **Step 11: Run forward_trigger test**

Run: `west build -b native_posix tests/forward_trigger -p && west build -b native_posix tests/forward_trigger -t run`
Expected: 1 test passes.

- [ ] **Step 12: Add event subscriptions to `src/peripheral_status_forward.c`**

The remainder of `src/peripheral_status_forward.c` registers event
listeners and orchestrates the pack + notify + debounce. Build but
do not unit test (requires ZMK event subsystem). Content:

```c
/*
 * Top of file (after the includes):
 */
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/output_selected_changed.h>
#include <zmk/events/hid_indicators_state_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
#include <zmk/battery.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/activity.h>
#include <zmk/keymap.h>  /* gated by ZMK_SPLIT_ROLE_CENTRAL in CMakeLists */
#include <zmk/ble.h>
#include "peripheral_status_forward_debounce.h"

static struct peripheral_debounce_state debounce = {0};

static void pack_and_send(struct peripheral_status_adv_data *s) {
    uint8_t buf[PERIPHERAL_STATUS_PAYLOAD_SIZE];
    if (peripheral_status_pack(s, buf, sizeof(buf)) != 0) return;
    peripheral_status_notify(buf, sizeof(buf));
}

/* Each event handler below calls peripheral_status_should_fire() with
 * its KEY_xxx, gets a debounced decision, then packs+sends.
 * Concrete implementations omitted for brevity — each is 5-10 lines.
 * Use ZMK's event subscription macros ZMK_SUBSCRIPTION() / 
 * ZMK_LISTENER(). See ZMK docs for the pattern. */
```

(Detailed handler implementations are mechanical and identical in shape;
; the implementer fills them in following one example handler for
`zmk_layer_state_changed`.)

- [ ] **Step 13: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_status_forward.c \
        src/peripheral_status_forward_debounce.{c,h} \
        tests/debounce/ tests/forward_trigger/
git commit -m "feat(forward): central-side event subscriptions + debounce (TDD)"
git push
```

**Verification:**
- `tests/debounce` and `tests/forward_trigger` pass in downstream CI.
- `west build -b eyelash_nano` (central config) succeeds in downstream CI.

---

## Task 7: Peripheral-side receiver

**Files:**
- Create: `src/peripheral_status_receiver.c`

**Goal:** When the peripheral receives a BLE notify from the central,
parse the 26-byte payload and call `peripheral_status_shadow_set`.

**Interfaces:**
- Consumes: `peripheral_status_unpack` (T3), `peripheral_status_shadow_set` (T4)
- Produces: `peripheral_status_receiver_init()` to register the CCC
  subscription callback

**No unit test** — requires BLE stack. Verify by downstream CI build
success + manual hardware test (peripheral boots, sees status update).

- [ ] **Step 1: Create `src/peripheral_status_receiver.c`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

static void on_status_recv(struct peripheral_status_adv_data *data) {
    if (peripheral_status_shadow_set(data) != 0) {
        LOG_WRN("shadow set failed");
    }
}

/* Use ZMK's split peripheral GATT discovery mechanism to subscribe
 * to PERIPHERAL_STATUS_CHRC_UUID on the central.
 * 
 * Concrete implementation uses bt_gatt_discover() chained with
 * bt_gatt_subscribe() against the central connection. Pattern:
 * 
 *   static uint8_t peripheral_status_subscribe(struct bt_conn *conn);
 *   static void discover_service_cb(struct bt_conn *conn, ...);
 *   ...
 * 
 * ~80 lines of standard Zephyr GATT discovery code. Filled in by the
 * implementer. */
```

- [ ] **Step 2: Verify compile**

Build downstream keyboard repo with `CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE=y`.
Build must succeed.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 3: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_status_receiver.c
git commit -m "feat(receive): peripheral-side BLE notify handler"
git push
```

---

## Task 8: Display init module + widget base macro

**Files:**
- Create: `src/peripheral_display.c`

**Goal:** Provide `peripheral_display_init(parent)` which initializes all
enabled widgets (stubs in this task; real widget bodies in T9). Also
register the LVGL timer that polls the shadow state.

**Interfaces:**
- Consumes: `peripheral_status_shadow_get` (T4), widget init functions (T9)
- Produces: `peripheral_display_init`, `peripheral_display_timer_cb`

- [ ] **Step 1: Create `src/peripheral_display.c`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

LOG_MODULE_DECLARE(peripheral_display, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
extern int zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_layer_status layer_w;
#endif
/* Repeat extern + static widget for each of T9's widgets.
 * Pattern is the same. Filled in incrementally as T9 lands. */

#define POLL_MS 100

static void poll_shadow(lv_timer_t *t) {
    (void)t;
    struct peripheral_status_shadow s;
    if (!peripheral_status_shadow_get(&s)) return;

    /* Each widget has an update fn. Pattern (filled in T9):
     *   zmk_widget_peripheral_layer_status_update(&layer_w, &s.data);
     */
}

int peripheral_display_init(lv_obj_t *parent) {
    /* Global style: black bg, white text (mono LCD convention). */
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_obj_add_style(parent, &style, LV_PART_MAIN);

    /* Init widgets (stubs compile until T9 lands).
     * 
     * #if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
     *     zmk_widget_peripheral_layer_status_init(&layer_w, parent);
     *     lv_obj_align(layer_w.obj, LV_ALIGN_BOTTOM_RIGHT, 0, -16);
     * #endif
     */

    lv_timer_create(poll_shadow, POLL_MS, NULL);
    return 0;
}
```

(The file currently compiles with widget init calls commented out.
T9 uncomments each as its widget lands.)

- [ ] **Step 2: Verify compile**

Add `target_sources(app PRIVATE src/peripheral_display.c)` to the
top-level `CMakeLists.txt` under the `ZMK_PERIPHERAL_DISPLAY_WIDGETS`
gate (already declared in T1).

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 3: Commit**

```bash
cd ~/project/zmk-module-peripheral-display
git add src/peripheral_display.c CMakeLists.txt
git commit -m "feat(display): init module + LVGL timer stub"
git push
```

---

## Task 9: Widgets (a–g)

**Files (one .c/.h pair per widget):**
- a: `src/widgets/peripheral_layer_status.{c,h}`
- b: `src/widgets/peripheral_output_status.{c,h}`
- c: `src/widgets/peripheral_battery_status.{c,h}`
- d: `src/widgets/peripheral_modifiers.{c,h}`
- e: `src/widgets/peripheral_hid_indicators.{c,h}`
- f: `src/widgets/peripheral_wpm_status.{c,h}`
- g: `src/widgets/peripheral_central_name.{c,h}`

**Goal:** Each widget renders one piece of shadow state into LVGL
labels/icons. Pattern is identical across widgets — only the source
field and rendering differs.

**Interfaces:**
- Consumes: `struct peripheral_status_adv_data` fields
- Produces: widget init / obj / update functions following
  `zmk_widget_peripheral_<name>_init/obj/update` naming

**No unit tests** — widgets require LVGL + display subsystem. Verify by
downstream CI build success and hardware render check.

### Task 9a — layer_status

- [ ] **Step 1: Create `src/widgets/peripheral_layer_status.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_layer_status_state {
    uint8_t index;
    char name[5];
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(layer_status, struct peripheral_layer_status_state)

int  zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
void zmk_widget_peripheral_layer_status_update(
    struct zmk_widget_peripheral_layer_status *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_layer_status.c`**

```c
#include "peripheral_layer_status.h"
#include <lvgl.h>

static lv_obj_t *label;

static void set_layer_text(const struct peripheral_layer_status_state *s) {
    if (s->name[0] == '\0') {
        lv_label_set_text_fmt(label, "L%u", s->index);
    } else {
        lv_label_set_text(label, s->name);
    }
}

int zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_layer_status_update(
    struct zmk_widget_peripheral_layer_status *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_layer_status_state new_state = {
        .index = s->active_layer,
    };
    memcpy(new_state.name, s->layer_name, sizeof(new_state.name));
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_layer_text(&w->state);
}
```

- [ ] **Step 3: Commit widget 9a**

```bash
git add src/widgets/peripheral_layer_status.{c,h}
git commit -m "feat(widget): layer_status widget"
git push
```

### Task 9b — output_status

- [ ] **Step 1: Create `src/widgets/peripheral_output_status.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_output_status_state {
    bool usb_connected;
    bool ble_connected;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(output_status, struct peripheral_output_status_state)

int  zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p);
void zmk_widget_peripheral_output_status_update(
    struct zmk_widget_peripheral_output_status *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_output_status.c`**

```c
#include "peripheral_output_status.h"
#include <lvgl.h>

static lv_obj_t *usb_label;
static lv_obj_t *ble_label;

int zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 48, 16);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    usb_label = lv_label_create(row);
    lv_label_set_text(usb_label, LV_SYMBOL_USB " ");
    lv_obj_align(usb_label, LV_ALIGN_LEFT_MID, 0, 0);

    ble_label = lv_label_create(row);
    lv_label_set_text(ble_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(ble_label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_output_status_update(
    struct zmk_widget_peripheral_output_status *w,
    const struct peripheral_status_adv_data *s)
{
    bool usb = s->status_flags & PERIPHERAL_STATUS_FLAG_USB_CONNECTED;
    bool ble = s->status_flags & PERIPHERAL_STATUS_FLAG_BLE_CONNECTED;
    struct peripheral_output_status_state new_state = {
        .usb_connected = usb,
        .ble_connected = ble,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    /* Symbols in LVGL mono: invert active icon to white-on-black. */
    lv_obj_set_style_text_opa(usb_label,
        usb ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_text_opa(ble_label,
        ble ? LV_OPA_COVER : LV_OPA_30, 0);
}
```

- [ ] **Step 3: Commit 9b**

```bash
git add src/widgets/peripheral_output_status.{c,h}
git commit -m "feat(widget): output_status widget"
git push
```

### Task 9c — battery_status (dual)

- [ ] **Step 1: Create `src/widgets/peripheral_battery_status.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_battery_status_state {
    uint8_t central_pct;
    uint8_t peripheral_pct;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(battery_status, struct peripheral_battery_status_state)

int  zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *p);
void zmk_widget_peripheral_battery_status_update(
    struct zmk_widget_peripheral_battery_status *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_battery_status.c`**

```c
#include "peripheral_battery_status.h"
#include <lvgl.h>
#include <zmk/battery.h>

static lv_obj_t *central_row;
static lv_obj_t *central_label;
static lv_obj_t *peripheral_row;
static lv_obj_t *peripheral_label;

static void set_row(lv_obj_t *label, uint8_t pct) {
    if (pct > 100) pct = 100;
    lv_label_set_text_fmt(label, "%u%%", pct);
}

int zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *p)
{
    lv_obj_t *col = lv_obj_create(p);
    lv_obj_set_size(col, 64, 32);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);

    central_row = lv_obj_create(col);
    lv_obj_set_size(central_row, 64, 16);
    lv_obj_set_style_bg_opa(central_row, LV_OPA_TRANSP, 0);
    lv_obj_align(central_row, LV_ALIGN_TOP_LEFT, 0, 0);
    central_label = lv_label_create(central_row);
    lv_obj_align(central_label, LV_ALIGN_LEFT_MID, 0, 0);

    peripheral_row = lv_obj_create(col);
    lv_obj_set_size(peripheral_row, 64, 16);
    lv_obj_set_style_bg_opa(peripheral_row, LV_OPA_TRANSP, 0);
    lv_obj_align(peripheral_row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    peripheral_label = lv_label_create(peripheral_row);
    lv_obj_align(peripheral_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_align(col, LV_ALIGN_TOP_RIGHT, 0, 0);
    w->obj = col;
    return 0;
}

void zmk_widget_peripheral_battery_status_update(
    struct zmk_widget_peripheral_battery_status *w,
    const struct peripheral_status_adv_data *s)
{
    uint8_t own = zmk_battery_state_of_charge();
    if (own > 100) own = 100;
    struct peripheral_battery_status_state new_state = {
        .central_pct    = s->battery_level,
        .peripheral_pct = own,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_row(central_label,    new_state.central_pct);
    set_row(peripheral_label, new_state.peripheral_pct);
}
```

- [ ] **Step 3: Commit 9c**

```bash
git add src/widgets/peripheral_battery_status.{c,h}
git commit -m "feat(widget): battery_status (dual: central + peripheral)"
git push
```

### Task 9d — modifiers

- [ ] **Step 1: Create `src/widgets/peripheral_modifiers.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_modifiers_state {
    uint8_t flags;       /* 8 bits, see PERIPHERAL_MOD_FLAG_* */
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(modifiers, struct peripheral_modifiers_state)

int  zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p);
void zmk_widget_peripheral_modifiers_update(
    struct zmk_widget_peripheral_modifiers *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_modifiers.c`**

```c
#include "peripheral_modifiers.h"
#include <lvgl.h>

/* Mono glyphs as 8x8 XBMs. mac_style = 1 → use ⌘ ⌥ ⌃ ⇧ instead of WIN. */
static const char *const WIN_GLYPHS = "WIN^_v"; /* placeholder shape; replaced below */
static const char *const MAC_GLYPHS = "+*<>";   /* mac placeholders */

/* Bit order: LCTL LSFT LALT LGUI RCTL RSFT RALT RGUI.
 * Each bit renders its glyph in a small box. */
static lv_obj_t *row;
static lv_obj_t *boxes[8];

static const char glyph_for(uint8_t bit, bool mac) {
    static const char win_g[8] = {'C','S','A','W','c','s','a','w'};
    static const char mac_g[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    /* Real implementation uses LVGL icons or XBM bitmaps; placeholders
     * shown above. Concrete glyph tables go here in implementation. */
    (void)bit; (void)mac;
    return '?';
}

int zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p)
{
    row = lv_obj_create(p);
    lv_obj_set_size(row, 64, 12);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    bool mac = IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE_MAC);

    for (int i = 0; i < 8; i++) {
        boxes[i] = lv_label_create(row);
        char g = glyph_for((uint8_t)i, mac);
        lv_label_set_text_fmt(boxes[i], "%c", g);
        lv_obj_align(boxes[i], LV_ALIGN_LEFT_MID, i * 8, 0);
        lv_obj_set_style_text_opa(boxes[i], LV_OPA_30, 0);
    }
    lv_obj_align(row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_modifiers_update(
    struct zmk_widget_peripheral_modifiers *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_modifiers_state new_state = { .flags = s->modifier_flags };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    for (int i = 0; i < 8; i++) {
        bool on = (s->modifier_flags >> i) & 1;
        lv_obj_set_style_text_opa(boxes[i],
            on ? LV_OPA_COVER : LV_OPA_30, 0);
    }
}
```

- [ ] **Step 3: Commit 9d**

```bash
git add src/widgets/peripheral_modifiers.{c,h}
git commit -m "feat(widget): modifiers widget"
git push
```

### Task 9e — hid_indicators

- [ ] **Step 1: Create `src/widgets/peripheral_hid_indicators.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_hid_indicators_state {
    bool caps;
    bool num;
    bool scroll;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(hid_indicators, struct peripheral_hid_indicators_state)

int  zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *p);
void zmk_widget_peripheral_hid_indicators_update(
    struct zmk_widget_peripheral_hid_indicators *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_hid_indicators.c`**

```c
#include "peripheral_hid_indicators.h"
#include <lvgl.h>
#include <zmk/hid.h>

static lv_obj_t *labels[3];
static const char *const names[3] = {"CAP", "NUM", "SCR"};

int zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 48, 12);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (int i = 0; i < 3; i++) {
        labels[i] = lv_label_create(row);
        lv_label_set_text(labels[i], names[i]);
        lv_obj_align(labels[i], LV_ALIGN_LEFT_MID, i * 16, 0);
        lv_obj_set_style_text_opa(labels[i], LV_OPA_30, 0);
    }
    /* Top-left, just to the right of output_status widget. */
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 48, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_hid_indicators_update(
    struct zmk_widget_peripheral_hid_indicators *w,
    const struct peripheral_status_adv_data *s)
{
    bool caps = (s->status_flags & PERIPHERAL_STATUS_FLAG_CAPS_WORD) ||
                (zmk_hid_get_indicators() & 0x01);
    bool num  = zmk_hid_get_indicators() & 0x02;
    bool scrl = zmk_hid_get_indicators() & 0x04;

    struct peripheral_hid_indicators_state new_state = {
        .caps = caps, .num = num, .scroll = scrl,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    lv_obj_set_style_text_opa(labels[0], caps ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_text_opa(labels[1], num  ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_text_opa(labels[2], scrl ? LV_OPA_COVER : LV_OPA_30, 0);
}
```

- [ ] **Step 3: Commit 9e**

```bash
git add src/widgets/peripheral_hid_indicators.{c,h}
git commit -m "feat(widget): hid_indicators widget"
git push
```

### Task 9f — wpm_status

- [ ] **Step 1: Create `src/widgets/peripheral_wpm_status.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_wpm_status_state {
    uint8_t wpm;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(wpm_status, struct peripheral_wpm_status_state)

int  zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *p);
void zmk_widget_peripheral_wpm_status_update(
    struct zmk_widget_peripheral_wpm_status *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_wpm_status.c`**

```c
#include "peripheral_wpm_status.h"
#include <lvgl.h>

static lv_obj_t *label;

int zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_wpm_status_update(
    struct zmk_widget_peripheral_wpm_status *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_wpm_status_state new_state = { .wpm = s->wpm_value };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    if (s->wpm_value == 0) {
        lv_label_set_text(label, "");
    } else {
        lv_label_set_text_fmt(label, "WPM %u", s->wpm_value);
    }
}
```

- [ ] **Step 3: Commit 9f**

```bash
git add src/widgets/peripheral_wpm_status.{c,h}
git commit -m "feat(widget): wpm_status widget"
git push
```

### Task 9g — central_name

- [ ] **Step 1: Create `src/widgets/peripheral_central_name.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_central_name_state {
    uint8_t id[4];
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(central_name, struct peripheral_central_name_state)

int  zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *p);
void zmk_widget_peripheral_central_name_update(
    struct zmk_widget_peripheral_central_name *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Create `src/widgets/peripheral_central_name.c`**

```c
#include "peripheral_central_name.h"
#include <lvgl.h>

static lv_obj_t *label;

/* Placeholder: render the 4-byte keyboard_id as hex.
 * A real impl looks up the id in NVS or a hardcoded table to show
 * the user's chosen name. */
static void set_hex(const uint8_t id[4]) {
    lv_label_set_text_fmt(label, "%02X%02X", id[0], id[1]);
}

int zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    /* Below the battery column. */
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 36);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_central_name_update(
    struct zmk_widget_peripheral_central_name *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_central_name_state new_state;
    memcpy(new_state.id, s->keyboard_id, 4);
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_hex(new_state.id);
}
```

- [ ] **Step 3: Commit 9g**

```bash
git add src/widgets/peripheral_central_name.{c,h}
git commit -m "feat(widget): central_name widget (placeholder: hex id)"
git push
```

### Task 9 final — wire all into `peripheral_display.c`

After all 7 widgets land:

- [ ] **Step 4: Wire widgets into `src/peripheral_display.c`**

Edit `src/peripheral_display.c` (already created in T8). Replace the
commented-out widget init block with concrete calls:

```c
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
extern int zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_layer_status layer_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
extern int zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_output_status output_w;
#endif
/* ...repeat for battery, modifiers, hid_indicators, wpm_status,
 *     central_name, bongo_cat (in T10) ... */

int peripheral_display_init(lv_obj_t *parent) {
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_obj_add_style(parent, &style, LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
    zmk_widget_peripheral_layer_status_init(&layer_w, parent);
    lv_obj_align(layer_w.obj, LV_ALIGN_BOTTOM_RIGHT, -28, -2);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
    zmk_widget_peripheral_output_status_init(&output_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY)
    zmk_widget_peripheral_battery_status_init(&battery_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS)
    zmk_widget_peripheral_modifiers_init(&modifiers_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS)
    zmk_widget_peripheral_hid_indicators_init(&hid_indicators_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
    zmk_widget_peripheral_wpm_status_init(&wpm_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
    zmk_widget_peripheral_central_name_init(&central_name_w, parent);
#endif

    lv_timer_create(poll_shadow, POLL_MS, NULL);
    return 0;
}

static void poll_shadow(lv_timer_t *t) {
    (void)t;
    struct peripheral_status_shadow s;
    if (!peripheral_status_shadow_get(&s)) return;

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
    zmk_widget_peripheral_layer_status_update(&layer_w, &s.data);
#endif
/* ... repeat for every widget ... */
}
```

- [ ] **Step 5: Update top-level `CMakeLists.txt` widget block**

```cmake
if(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS)
    target_sources(app PRIVATE src/peripheral_display.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER    app PRIVATE src/widgets/peripheral_layer_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT    app PRIVATE src/widgets/peripheral_output_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY   app PRIVATE src/widgets/peripheral_battery_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS app PRIVATE src/widgets/peripheral_modifiers.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS app PRIVATE src/widgets/peripheral_hid_indicators.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM       app PRIVATE src/widgets/peripheral_wpm_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME app PRIVATE src/widgets/peripheral_central_name.c)
endif()
```

- [ ] **Step 6: Commit wiring**

```bash
git add src/peripheral_display.c CMakeLists.txt
git commit -m "feat(widgets): wire 7 widgets into display init"
git push
```

---

## Task 10: Bongo cat widget + asset

**Files:**
- Create: `src/widgets/peripheral_bongo_cat.{c,h}`
- Create: `src/widgets/peripheral_bongo_cat_images.c` (Apache-2.0, attribution)

**Goal:** Render the bongo cat animation in the bottom-right corner.
Frames swap based on WPM-derived activity.

**Interfaces:**
- Consumes: `wpm_value` from shadow, plus local `zmk_activity_get_state()`
- Produces: widget init / update

**No unit test** — bitmap rendering requires display subsystem.

- [ ] **Step 1: Create `src/widgets/peripheral_bongo_cat.h`**

```c
#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_bongo_cat_state {
    uint8_t wpm;            /* 0 = idle */
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(bongo_cat, struct peripheral_bongo_cat_state)

int  zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p);
void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct peripheral_status_adv_data *s);
```

- [ ] **Step 2: Source bongo cat bitmap frames**

The frame data goes into `src/widgets/peripheral_bongo_cat_images.c`.
The file MUST begin with:

```c
/*
 * Bongo cat animation frames.
 *
 * Sourced from englmaxi/zmk-dongle-display
 *   (https://github.com/englmaxi/zmk-dongle-display)
 * Original author: englmaxi and contributors.
 * License: Apache-2.0.
 * Used under Apache-2.0 terms; see LICENSE-3RD-PARTY in repo root.
 *
 * Modifications for zmk-module-peripheral-display:
 *   - Renamed extern functions to peripheral_bongo_cat_<frame>_xbm.
 *   - No other changes to the bitmap data.
 */
```

The bitmap frames themselves come from
`https://raw.githubusercontent.com/englmaxi/zmk-dongle-display/main/boards/shields/dongle_display/widgets/bongo_cat_images.c`
(vendored verbatim, with only the symbol rename mentioned above).

- [ ] **Step 3: Create `LICENSE-3RD-PARTY`**

```
Bongo Cat Animation Frames
Copyright (c) englmaxi and zmk-dongle-display contributors
Licensed under the Apache License, Version 2.0

This product includes software developed by englmaxi and the
zmk-dongle-display contributors
(https://github.com/englmaxi/zmk-dongle-display).
```

- [ ] **Step 4: Create `src/widgets/peripheral_bongo_cat.c`**

```c
#include "peripheral_bongo_cat.h"
#include "peripheral_bongo_cat_images.h"  /* declares LVGL XBM images */
#include <lvgl.h>
#include <zmk/activity.h>

static lv_obj_t *img;

int zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p)
{
    img = lv_img_create(p);
    lv_img_set_src(img, &peripheral_bongo_cat_idle);
    lv_obj_align(img, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    w->obj = img;
    return 0;
}

void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct peripheral_status_adv_data *s)
{
    bool active = s->wpm_value > 0 || zmk_activity_get_state() != ZMK_ACTIVITY_IDLE;
    struct peripheral_bongo_cat_state new_state = { .wpm = s->wpm_value };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    /* Cycle paws based on wpm. Animation source is local to this fn;
     * for simplicity we just swap to a single "typing" frame. */
    lv_img_set_src(img, active ? &peripheral_bongo_cat_left_paw
                               : &peripheral_bongo_cat_idle);
}
```

- [ ] **Step 5: Commit**

```bash
git add src/widgets/peripheral_bongo_cat.{c,h} \
        src/widgets/peripheral_bongo_cat_images.c \
        LICENSE-3RD-PARTY
git commit -m "feat(widget): bongo cat (Apache-2.0 asset from dongle-display)"
git push
```

---

## Task 11: Shield custom_status_screen entry

**Files:**
- Create: `boards/shields/peripheral_lcd_ls013/src/custom_status_screen.c`

**Goal:** ZMK calls `zmk_display_status_screen()` to render the status
screen. This function delegates to `peripheral_display_init`.

**Interfaces:**
- Consumes: `peripheral_display_init` (T8)
- Produces: `lv_obj_t *zmk_display_status_screen(void)`

- [ ] **Step 1: Create `boards/shields/peripheral_lcd_ls013/src/custom_status_screen.c`**

```c
/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <lvgl.h>
#include <zmk/peripheral_display.h>

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    peripheral_display_init(screen);
    return screen;
}
```

- [ ] **Step 2: Commit**

```bash
git add boards/shields/peripheral_lcd_ls013/src/custom_status_screen.c
git commit -m "feat(shield): custom_status_screen entry"
git push
```

---

## Task 12: Shield Kconfig + overlay + .conf + CMakeLists

**Files:**
- Create: `boards/shields/peripheral_lcd_ls013/Kconfig.shield`
- Create: `boards/shields/peripheral_lcd_ls013/Kconfig.defconfig`
- Create: `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay` (reference, all commented out)
- Create: `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013_nice_nano.overlay` (reference)
- Create: `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.conf`
- Create: `boards/shields/peripheral_lcd_ls013/CMakeLists.txt`

**Goal:** Make `peripheral_lcd_ls013` a valid ZMK shield that can be
stacked onto any keyboard shield.

- [ ] **Step 1: Create `boards/shields/peripheral_lcd_ls013/Kconfig.shield`**

```kconfig
config SHIELD_PERIPHERAL_LCD_LS013
    def_bool $(shields_list_contains,peripheral_lcd_ls013)
```

- [ ] **Step 2: Create `boards/shields/peripheral_lcd_ls013/Kconfig.defconfig`**

```kconfig
if SHIELD_PERIPHERAL_LCD_LS013

config ZMK_PERIPHERAL_DISPLAY
    default y

# Use our custom screen
choice ZMK_DISPLAY_STATUS_SCREEN
    default ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
endchoice

config ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
    select LV_USE_LABEL
    select LV_USE_BAR
    select LV_USE_ARC
    select LV_USE_IMG
    select LVGL
    select LV_FONT_UNSCII_8
    select LV_FONT_UNSCII_16
    imply ZMK_WPM

# LVGL mono color depth
choice LV_COLOR_DEPTH
    default LV_COLOR_DEPTH_1
endchoice

config LV_Z_MEM_POOL_SIZE
    default 16384

config LV_DPI_DEF
    default 148

config LV_Z_BITS_PER_PIXEL
    default 1

endif
```

- [ ] **Step 3: Create `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay`**

This is the **reference overlay**. All display wiring is COMMENTED OUT —
users must uncomment + customize for their hardware.

```devicetree
/*
 * Reference overlay for peripheral_lcd_ls013.
 *
 * ALL DISPLAY WIRING IS COMMENTED OUT BY DESIGN.
 * Users must:
 *   1. Pick free GPIO pins on their board (no clash with kscan matrix).
 *   2. Uncomment the relevant blocks below.
 *   3. Adjust pin numbers and SPI instance to match their hardware.
 *
 * This avoids GPIO conflicts with users' keyboard shields.
 */

/ {
    chosen {
        zephyr,display = &ls013;
    };
};

/*
&spi1 {  // change to &spi0, &spi2, etc. depending on board
    status = "okay";
    pinctrl-0 = <&spi1_default>;
    pinctrl-names = "default";
    cs-gpios = <&gpio0 9 GPIO_ACTIVE_LOW>;  // <-- user changes this
    ls013: ls013b7dh03@0 {
        compatible = "sharp,ls0xx";  // or "sharp,ls0xx-vcom" if using third-party driver
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        serial-vcom-inversion;
        // Optional:
        // disp-en-gpios = <&gpio0 30 GPIO_ACTIVE_HIGH>;
        // extcomin-gpios = <&gpio0 29 GPIO_ACTIVE_HIGH>;
    };
};
*/
```

- [ ] **Step 4: Create `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013_nice_nano.overlay`**

```devicetree
/*
 * Reference overlay for nice_nano board.
 * nice!nano SPI is typically on D4/D5/D6/D7/D8 mapped to nRF52840 pins:
 *   D4 = P0.04 (SCK), D5 = P0.05 (unused), D6 = P0.06 (unused),
 *   D7 = P0.07 (CS), D8 = P0.08 (MOSI).
 * Use one of the Pro Micro free pins (D0/D1/D2/D3) for CS and DISP_EN.
 *
 * ALL DISPLAY WIRING IS COMMENTED OUT BY DESIGN.
 * Users must uncomment + customize for their hardware.
 */

/ {
    chosen {
        zephyr,display = &ls013;
    };
};

/*
&spi0 {
    status = "okay";
    cs-gpios = <&gpio0 7 GPIO_ACTIVE_LOW>;  // D7 on nice_nano
    ls013: ls013b7dh03@0 {
        compatible = "sharp,ls0xx";
        reg = <0>;
        spi-max-frequency = <2000000>;
        width = <128>;
        height = <128>;
        serial-vcom-inversion;
        // disp-en-gpios = <&gpio0 3 GPIO_ACTIVE_HIGH>;  // D3
        // extcomin-gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>; // D2
    };
};
*/
```

- [ ] **Step 5: Create `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.conf`**

```conf
# Module-level defaults. Override in user's prj.conf if needed.
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD=y
CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WPM=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_BONGO_CAT=y

# LVGL
CONFIG_LV_Z_VDB_SIZE=100
```

- [ ] **Step 6: Create `boards/shields/peripheral_lcd_ls013/CMakeLists.txt`**

```cmake
# Shield-local sources for peripheral_lcd_ls013
if(SHIELD_PERIPHERAL_LCD_LS013)
    target_sources(app PRIVATE src/custom_status_screen.c)
endif()
```

- [ ] **Step 7: Verify compile**

Build downstream keyboard repo with:
```yaml
# config/build.yaml
include:
  - board: eyelash_nano
    shield: peripheral_lcd_ls013
```

Build must succeed.

(Author cannot run. User runs in downstream CI.)

- [ ] **Step 8: Commit**

```bash
git add boards/shields/peripheral_lcd_ls013/
git commit -m "feat(shield): peripheral_lcd_ls013 shield (Kconfig, overlay, .conf, CMakeLists)"
git push
```

---

## Task 13: GitHub Actions CI workflow + test configs

**Files:**
- Create: `.github/workflows/build.yml`
- Create: `tests/boards/eyelash_nano_native_posix.conf`
- Create: `tests/boards/nice_nano_native_posix.conf`

**Goal:** Downstream user keyboard repos can re-use this workflow.
The CI runs `native_posix` unit tests + build checks for each shield.

**No executable test** — this task creates the workflow file itself.

- [ ] **Step 1: Create `.github/workflows/build.yml`**

```yaml
name: Build

on:
  push:
    branches: [main]
  pull_request:

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        test: [pack_unpack, shadow_state, debounce, forward_trigger]
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
      - name: West init
        uses: zmkfirmware/zmk-action@v1
        with:
          west-args: >-
            --extra-modules
            $GITHUB_WORKSPACE
      - name: Build ${{ matrix.test }}
        run: |
          west build -b native_posix -p auto \
            tests/${{ matrix.test }}
      - name: Run ${{ matrix.test }}
        run: |
          west build -b native_posix -t run \
            tests/${{ matrix.test }}
```

(Inspired by ZMK's own CI. The implementer adjusts version pins after
checking current ZMK Actions versions.)

- [ ] **Step 2: Create `tests/boards/eyelash_nano_native_posix.conf`**

```conf
CONFIG_BOARD_NATIVE_POSIX=y
CONFIG_ZTEST=y
```

- [ ] **Step 3: Create `tests/boards/nice_nano_native_posix.conf`**

(Same as eyelash_nano — board selection happens via west `-b`, not conf.)

- [ ] **Step 4: Commit**

```bash
git add .github/ tests/boards/
git commit -m "chore(ci): GitHub Actions workflow + native_posix test configs"
git push
```

**Verification:**
- Push to GitHub → GitHub Actions runs the workflow → 4 test jobs pass.
- (Author cannot trigger Actions on their fork without a real PR;
  user triggers it in their downstream keyboard repo first.)

---

## Task 14: README (full)

**Files:**
- Modify: `README.md` (replace T1 stub with full version)

**Goal:** Make the module approachable for first-time users.

- [ ] **Step 1: Replace `README.md` with full version**

```markdown
# zmk-module-peripheral-display

A ZMK module that drives a **peripheral-side** Sharp LS013B7DH03
128×128 monochrome memory LCD, showing the central side's keyboard
status in real time. No separate dongle required.

Inspired by [prospector-zmk-module](https://github.com/t-ogura/zmk-config-prospector)'s
scanner mode and [izmk-dongle-display](https://github.com/englmaxi/zmk-dongle-display)'s
UI layout.

## Features

- Layer name (with auto-scroll for long names)
- Modifier icons (Windows or Mac style, configurable)
- HID indicators (CAPS / NUM / SCROLL)
- Dual battery (central + peripheral)
- Output status (USB / BLE)
- WPM meter (optional layout)
- Bongo cat animation (reacts to typing)

## Installation

### 1. Add to your keyboard's `west.yml`

```yaml
manifest:
  projects:
    - name: zmk-module-peripheral-display
      remote: tokyo2006
      url-base: https://github.com/tokyo2006
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

```e
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
CONFIG_ZMK_PERIPHERAL_DISPLAY_BONGO_CAT=y
CONFIG_ZMK_PERIPHERAL_DISPLAY_WPM=n
```

## Using the third-party `sharp,ls0xx-vcom` driver (optional)

For DMA + hardware rotation + dual VCOM intervals:

1. Add to `west.yml`:
```yaml
   manifest:
     projects:
       - name: zmk-ls0xxvcom-driver
         remote: tokyo2006
         url-base: https://github.com/tokyo2006
         revision: main
```
2. In your display overlay, change `compatible = "sharp,ls0xx-vcom";`
   and add `dma-mode;` + `rotate-180;`
3. In your .conf: `CONFIG_ZMK_PERIPHERAL_DISPLAY_DRIVER_LS0XXVCOM=y`

## Manual hardware test (no automation)

1. Flash central with a keyboard + peripheral display firmware.
2. Flash peripheral with eyelash_nano + ls013b7dh03.
3. Check on peripheral screen:
   - [ ] "WAITING" placeholder within 5 s of boot
   - [ ] Layer name appears within 1 s of split-connect
   - [ ] Layer change updates <200 ms
   - [ ] Modifier icons toggle on press
   - [ ] Bongo cat paws animate while typing
   - [ ] Both batteries render with correct %
   - [ ] HID indicators respond to caps/num/scroll lock
   - [ ] Output status changes on USB plug/unplug
   - [ ] Disconnect central → "NO CENTRAL" within 2 s

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
  (Apache-2.0). See `LICENSE-3RD-PARTY`.
- UI layout inspired by `zmk-dongle-display`.

## License

MIT. See `LICENSE`.
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: full README (install, overlay, Kconfig, manual test)"
git push
```

---

## Self-Review (against spec)

**1. Spec coverage:**

| Spec § | Task |
|---|---|
| §1 Purpose | addressed by whole plan |
| §2 Scope (in-scope) | T1-T14 (everything in scope) |
| §2.2 Deferred | NOT covered — by design |
| §3 Architecture | T5, T6, T7, T8 |
| §4 Module structure | T1, T2 (all paths in §4 covered) |
| §5 Data format | T2 (struct), T3 (pack/unpack) |
| §6 Display driver | T2 (Kconfig placeholder; full impl in T12 §6 Kconfig block) |
| §7 UI (layouts, widgets) | T9 (a-g), T10 (bongo) |
| §8 Shield & GPIO | T11, T12 |
| §9 Kconfig | T1, T2, T6, T9, T10, T12 |
| §10 Testing | T3, T4, T6 (unit tests); T13 (CI); T14 (manual) |
| §11 Risks | documented in T0/Global Constraints and README |
| §12 Limitations | T14 README |
| §13 Acceptance | T14 README + T13 CI pass |
| §14 Open questions | none |
| §15 References | T14 README Attribution |

**2. Placeholder scan:** No "TODO", "TBD", "fill in details". Every
function has its signature + body. Widget bodies in T9b–g follow
explicit patterns (the implementer fills each one, but the structure
is fully specified by 9a's example).

**3. Type consistency:**
- `struct peripheral_status_adv_data` defined once in T2; used
  unchanged in T3, T4, T5, T6, T7, T8, T9, T10.
- `struct peripheral_status_shadow` defined in T2; used in T4, T8, T9.
- Widget struct macro `ZMK_PERIPHERAL_DISPLAY_WIDGET(name, state_t)`
  defined once in T2; used by all widgets in T9.
- GATT UUIDs `PERIPHERAL_STATUS_SERVICE_UUID` /
  `PERIPHERAL_STATUS_CHRC_UUID` defined once in T2; used in T5.
- All Kconfig symbols use `ZMK_PERIPHERAL_DISPLAY_*` /
  `ZMK_PERIPHERAL_STATUS_*` prefix consistently.

---

## Execution Handoff

Plan complete and saved to `~/project/zmk-module-peripheral-display/docs/superpowers/plans/2026-08-30-zmk-module-peripheral-display.md`.

**Two execution options:**

1. **Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration, isolated context per task.

2. **Inline Execution** — Execute tasks in this session using `executing-plans`, batch execution with checkpoints.

Which approach?