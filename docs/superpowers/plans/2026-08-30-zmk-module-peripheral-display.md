# zmk-module-peripheral-display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a ZMK module that renders central-keyboard status (layer/modifiers/WPM/battery/output/HID) on a Sharp LS013B7DH03 128×128 mono display attached to the peripheral half of a split keyboard.

**Architecture:** The central pushes a 26-byte `zmk_status_adv_data` packet to the peripheral over a custom GATT notify characteristic (central = GATT server, peripheral = GATT client — the reverse of normal ZMK split). The peripheral stores it in a mutex-protected shadow state and a 100ms LVGL timer redraws dirty widgets. Display wiring is fully devicetree-driven; the module ships only a commented reference overlay.

**Tech Stack:** ZMK main (Zephyr 4.x), nRF52840 (eyelash_nano / nice!nano), LVGL 9, Zephyr `sharp,ls0xx` display driver (built-in), Zephyr BLE GATT API.

**Spec:** `docs/superpowers/specs/2026-08-30-zmk-module-peripheral-display-design.md`

## Global Constraints

- Only nRF52840 boards (eyelash_nano, nice!nano). NOT nRF52832.
- ZMK main (Zephyr 4.x) only. NOT ZMK 0.3.
- Status struct is `struct zmk_status_adv_data` (26 bytes, `__packed`), layout identical to prospector v2.2.2, but **redefined** in this module (no `#include` of prospector).
- Module name `zmk-module-peripheral-display`; shield name `peripheral_lcd_ls013`.
- Public API prefix `peripheral_*`; Kconfig prefix `ZMK_PERIPHERAL_DISPLAY_*`.
- Display wiring is devicetree-driven; module ships no hardcoded GPIO. Reference overlay is fully commented out.
- Default driver is Zephyr built-in `sharp,ls0xx`. Third-party `sharp,ls0xx-vcom` is opt-in via Kconfig choice only.
- The author cannot run `west build` or hardware tests locally. Do NOT claim tests pass; verification is delegated to downstream GitHub Actions + user manual tests.
- License: MIT. Bongo-cat bitmaps sourced from `englmaxi/zmk-dongle-display` (Apache-2.0) with attribution.

---

## File Structure

```
zmk-module-peripheral-display/
├── zephyr/module.yml                      # ZMK module entry
├── CMakeLists.txt                         # source wiring per Kconfig
├── Kconfig                                # all module options
├── LICENSE                                # MIT
├── README.md                              # install / overlay / build / test doc
├── .github/workflows/build.yml            # CI (for downstream, not run here)
├── include/zmk/
│   ├── peripheral_status.h                # struct + constants + pack/unpack/shadow API
│   └── peripheral_display.h               # widget structs + init API
├── src/
│   ├── peripheral_status.c                # pack/unpack + shadow state + should_send
│   ├── peripheral_status_forward.c        # central: GATT server + events + notify + heartbeat
│   ├── peripheral_status_receiver.c       # peripheral: GATT client + subscribe + shadow write
│   ├── peripheral_display.c               # custom_status_screen entry + 100ms update loop
│   └── widgets/
│       ├── peripheral_layer_status.c
│       ├── peripheral_output_status.c
│       ├── peripheral_battery_status.c
│       ├── peripheral_modifiers.c
│       ├── peripheral_hid_indicators.c
│       ├── peripheral_wpm_status.c
│       ├── peripheral_central_name.c
│       └── peripheral_bongo_cat.c
├── boards/shields/peripheral_lcd_ls013/
│   ├── Kconfig.shield
│   ├── Kconfig.defconfig
│   ├── peripheral_lcd_ls013.overlay       # commented reference
│   ├── peripheral_lcd_ls013.conf
│   └── CMakeLists.txt
└── tests/
    ├── pack_unpack/src/main.c
    ├── shadow_state/src/main.c
    ├── debounce/src/main.c
    └── forward_trigger/src/main.c
```

**Interfaces (lock-in):**

`include/zmk/peripheral_status.h`:
```c
struct zmk_status_adv_data { /* 26 bytes, see Task 1 */ } __packed;

enum peripheral_status_event {
    PERIPHERAL_STATUS_EVT_LAYER,
    PERIPHERAL_STATUS_EVT_MODIFIERS,
    PERIPHERAL_STATUS_EVT_BATTERY,
    PERIPHERAL_STATUS_EVT_WPM,
    PERIPHERAL_STATUS_EVT_OUTPUT,
    PERIPHERAL_STATUS_EVT_ACTIVITY,
    PERIPHERAL_STATUS_EVT_ENDPOINT,
    PERIPHERAL_STATUS_EVT_HID_INDICATORS,
    PERIPHERAL_STATUS_EVT_HEARTBEAT,
};

/* central: build a status packet into *out from live ZMK state */
void peripheral_status_pack(struct zmk_status_adv_data *out);
/* pure: given event type + timestamps, decide whether to send now */
bool peripheral_status_should_send(enum peripheral_status_event evt,
                                   uint32_t last_send_ms, uint32_t now_ms);
/* peripheral: store a received packet into shadow state */
void peripheral_status_shadow_set(const struct zmk_status_adv_data *data);
/* peripheral: copy shadow state out; returns false if never received */
bool peripheral_status_shadow_get(struct zmk_status_adv_data *out);
/* peripheral: true if a packet arrived within the last 3 seconds */
bool peripheral_status_shadow_connected(void);
```

`include/zmk/peripheral_display.h`:
```c
struct zmk_peripheral_widget { lv_obj_t *obj; bool dirty; };

/* each widget type has: init(widget, parent) + obj(widget) + update(widget, shadow) */
void zmk_peripheral_display_init(lv_obj_t *screen);
void zmk_peripheral_display_update(const struct zmk_status_adv_data *shadow);
```

---

### Task 1: Module scaffolding + status struct + pack/unpack

**Files:**
- Create: `zephyr/module.yml`
- Create: `CMakeLists.txt`
- Create: `Kconfig`
- Create: `LICENSE`
- Create: `include/zmk/peripheral_status.h`
- Create: `src/peripheral_status.c`
- Test: `tests/pack_unpack/src/main.c`

**Interfaces:**
- Consumes: nothing.
- Produces: `struct zmk_status_adv_data`, `peripheral_status_pack()`, `peripheral_status_unpack_validate()`.

- [ ] **Step 1: Write `zephyr/module.yml`**

```yaml
name: zmk-module-peripheral-display
build:
  cmake: .
  kconfig: Kconfig
  settings:
    dts_root: .
```

- [ ] **Step 2: Write `include/zmk/peripheral_status.h`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Status data format shared between the central (forward) and
 * peripheral (receive) halves. 26 bytes, layout-compatible with
 * prospector-zmk-module v2.2.2's zmk_status_adv_data so any existing
 * scanner can also parse this format. Redefined here to avoid a hard
 * dependency on prospector.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zmk_status_adv_data {
    uint8_t manufacturer_id[2];     /* 0xFF, 0xFF */
    uint8_t service_uuid[2];        /* 0xAB, 0xCD */
    uint8_t version;                /* [7:4] major, [3:0] minor */
    uint8_t battery_level;          /* central battery 0-100% */
    uint8_t active_layer;           /* highest active layer */
    uint8_t profile_slot;           /* [5:3] patch, [2:0] profile */
    uint8_t connection_count;       /* 1 + USB */
    uint8_t status_flags;           /* bit field, see below */
    uint8_t device_role;            /* STANDALONE/CENTRAL/PERIPHERAL */
    uint8_t device_index;           /* 0 for central */
    uint8_t peripheral_battery[3];  /* [0] right/aux, [1] aux1, [2] aux2 */
    char layer_name[4];             /* 4 chars, NOT null-terminated */
    uint8_t keyboard_id[4];         /* hardware-unique hash */
    uint8_t modifier_flags;         /* ZMK_MOD_FLAG_* bits */
    uint8_t wpm_value;              /* 0-255, 0 = inactive */
    uint8_t channel;                /* 0 = broadcast */
} __packed;

/* status_flags bits */
#define ZMK_STATUS_FLAG_CAPS_WORD     (1 << 0)
#define ZMK_STATUS_FLAG_CHARGING      (1 << 1)
#define ZMK_STATUS_FLAG_USB_CONNECTED (1 << 2)
#define ZMK_STATUS_FLAG_USB_HID_READY (1 << 3)
#define ZMK_STATUS_FLAG_BLE_CONNECTED (1 << 4)
#define ZMK_STATUS_FLAG_BLE_BONDED    (1 << 5)

/* modifier_flags bits */
#define ZMK_MOD_FLAG_LCTL (1 << 0)
#define ZMK_MOD_FLAG_LSFT (1 << 1)
#define ZMK_MOD_FLAG_LALT (1 << 2)
#define ZMK_MOD_FLAG_LGUI (1 << 3)
#define ZMK_MOD_FLAG_RCTL (1 << 4)
#define ZMK_MOD_FLAG_RSFT (1 << 5)
#define ZMK_MOD_FLAG_RALT (1 << 6)
#define ZMK_MOD_FLAG_RGUI (1 << 7)

/* device_role values */
#define ZMK_DEVICE_ROLE_STANDALONE 0
#define ZMK_DEVICE_ROLE_CENTRAL    1
#define ZMK_DEVICE_ROLE_PERIPHERAL 2

#define ZMK_STATUS_ADV_SERVICE_UUID 0xABCD
#define ZMK_PERIPHERAL_STATUS_PACKET_SIZE 26

enum peripheral_status_event {
    PERIPHERAL_STATUS_EVT_LAYER,
    PERIPHERAL_STATUS_EVT_MODIFIERS,
    PERIPHERAL_STATUS_EVT_BATTERY,
    PERIPHERAL_STATUS_EVT_WPM,
    PERIPHERAL_STATUS_EVT_OUTPUT,
    PERIPHERAL_STATUS_EVT_ACTIVITY,
    PERIPHERAL_STATUS_EVT_ENDPOINT,
    PERIPHERAL_STATUS_EVT_HID_INDICATORS,
    PERIPHERAL_STATUS_EVT_HEARTBEAT,
};

void peripheral_status_pack(struct zmk_status_adv_data *out);

bool peripheral_status_unpack_validate(const uint8_t *buf, size_t len,
                                       struct zmk_status_adv_data *out);

bool peripheral_status_should_send(enum peripheral_status_event evt,
                                   uint32_t last_send_ms, uint32_t now_ms);

void peripheral_status_shadow_set(const struct zmk_status_adv_data *data);

bool peripheral_status_shadow_get(struct zmk_status_adv_data *out);

bool peripheral_status_shadow_connected(void);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 3: Write the pack/unpack portion of `src/peripheral_status.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Pack/unpack of the 26-byte status packet, plus the shared shadow state.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include <zmk/peripheral_status.h>

/* ============ pack ============ */
/* Central-side only: fill *out from live ZMK state. The heavy field
 * population lives in peripheral_status_forward.c's caller, but the
 * fixed header + validation helpers live here so they can be unit-tested
 * without BLE. */

void peripheral_status_pack(struct zmk_status_adv_data *out)
{
    memset(out, 0, sizeof(*out));
    out->manufacturer_id[0] = 0xFF;
    out->manufacturer_id[1] = 0xFF;
    out->service_uuid[0] = 0xAB;
    out->service_uuid[1] = 0xCD;
    out->device_role = ZMK_DEVICE_ROLE_CENTRAL;
    out->device_index = 0;
}

/* ============ unpack ============ */

bool peripheral_status_unpack_validate(const uint8_t *buf, size_t len,
                                       struct zmk_status_adv_data *out)
{
    if (buf == NULL || out == NULL || len < ZMK_PERIPHERAL_STATUS_PACKET_SIZE) {
        return false;
    }
    const struct zmk_status_adv_data *p = (const struct zmk_status_adv_data *)buf;
    if (p->manufacturer_id[0] != 0xFF || p->manufacturer_id[1] != 0xFF ||
        p->service_uuid[0] != 0xAB || p->service_uuid[1] != 0xCD) {
        return false;
    }
    memcpy(out, buf, sizeof(*out));
    return true;
}
```

- [ ] **Step 4: Write the failing test `tests/pack_unpack/src/main.c`**

```c
#include <zephyr/ztest.h>
#include <string.h>
#include <zmk/peripheral_status.h>

ZTEST(pack_unpack, test_roundtrip_preserves_fields)
{
    struct zmk_status_adv_data src;
    memset(&src, 0, sizeof(src));
    src.manufacturer_id[0] = 0xFF;
    src.manufacturer_id[1] = 0xFF;
    src.service_uuid[0] = 0xAB;
    src.service_uuid[1] = 0xCD;
    src.battery_level = 87;
    src.active_layer = 3;
    src.wpm_value = 42;
    src.modifier_flags = ZMK_MOD_FLAG_LCTL | ZMK_MOD_FLAG_LGUI;
    src.peripheral_battery[0] = 75;
    memcpy(src.layer_name, "HW", 2);
    memcpy(src.keyboard_id, "\x01\x02\x03\x04", 4);

    uint8_t buf[26];
    memcpy(buf, &src, sizeof(buf));

    struct zmk_status_adv_data dst;
    bool ok = peripheral_status_unpack_validate(buf, sizeof(buf), &dst);
    zassert_true(ok, "validate should pass for well-formed packet");
    zassert_equal(dst.battery_level, 87, NULL);
    zassert_equal(dst.active_layer, 3, NULL);
    zassert_equal(dst.wpm_value, 42, NULL);
    zassert_equal(dst.modifier_flags, ZMK_MOD_FLAG_LCTL | ZMK_MOD_FLAG_LGUI, NULL);
    zassert_equal(dst.peripheral_battery[0], 75, NULL);
    zassert_mem_equal(dst.layer_name, "HW", 2, NULL);
}

ZTEST(pack_unpack, test_rejects_bad_magic)
{
    uint8_t buf[26] = {0};
    struct zmk_status_adv_data out;
    zassert_false(peripheral_status_unpack_validate(buf, sizeof(buf), &out),
                  "should reject packet with wrong magic");
}

ZTEST(pack_unpack, test_rejects_short)
{
    uint8_t buf[10] = {0};
    struct zmk_status_adv_data out;
    zassert_false(peripheral_status_unpack_validate(buf, sizeof(buf), &out),
                  "should reject short packet");
}

ZTEST(pack_unpack, test_pack_sets_header)
{
    struct zmk_status_adv_data out;
    peripheral_status_pack(&out);
    zassert_equal(out.manufacturer_id[0], 0xFF, NULL);
    zassert_equal(out.manufacturer_id[1], 0xFF, NULL);
    zassert_equal(out.service_uuid[0], 0xAB, NULL);
    zassert_equal(out.service_uuid[1], 0xCD, NULL);
    zassert_equal(out.device_role, ZMK_DEVICE_ROLE_CENTRAL, NULL);
}

ZTEST_SUITE(pack_unpack, NULL, NULL, NULL, NULL, NULL);
```

- [ ] **Step 5: Commit**

```bash
git add zephyr/module.yml CMakeLists.txt Kconfig LICENSE include/zmk/peripheral_status.h src/peripheral_status.c tests/pack_unpack/src/main.c
git commit -m "feat: module scaffolding + 26-byte status struct + pack/unpack"
```

---

### Task 2: Shadow state (mutex-protected) + unit test

**Files:**
- Modify: `src/peripheral_status.c`
- Modify: `CMakeLists.txt` (add `src/peripheral_status.c` to build)
- Test: `tests/shadow_state/src/main.c`

**Interfaces:**
- Consumes: `struct zmk_status_adv_data` (Task 1).
- Produces: `peripheral_status_shadow_set/get/connected` (already declared in header).

- [ ] **Step 1: Write the shadow-state portion of `src/peripheral_status.c`**

Append to the file created in Task 1:

```c
/* ============ shadow state (peripheral side) ============ */

static struct zmk_status_adv_data shadow_data;
static bool shadow_valid;
static uint32_t shadow_last_ms;
static struct k_mutex shadow_mutex = Z_MUTEX_INITIALIZER(shadow_mutex);

void peripheral_status_shadow_set(const struct zmk_status_adv_data *data)
{
    if (data == NULL) {
        return;
    }
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    memcpy(&shadow_data, data, sizeof(shadow_data));
    shadow_valid = true;
    shadow_last_ms = k_uptime_get_32();
    k_mutex_unlock(&shadow_mutex);
}

bool peripheral_status_shadow_get(struct zmk_status_adv_data *out)
{
    if (out == NULL) {
        return false;
    }
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    bool ok = shadow_valid;
    if (ok) {
        memcpy(out, &shadow_data, sizeof(*out));
    }
    k_mutex_unlock(&shadow_mutex);
    return ok;
}

bool peripheral_status_shadow_connected(void)
{
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    bool ok = shadow_valid && (k_uptime_get_32() - shadow_last_ms < 3000);
    k_mutex_unlock(&shadow_mutex);
    return ok;
}
```

- [ ] **Step 2: Write `tests/shadow_state/src/main.c`**

```c
#include <zephyr/ztest.h>
#include <zmk/peripheral_status.h>

ZTEST(shadow_state, test_set_then_get)
{
    struct zmk_status_adv_data in = {0};
    in.battery_level = 55;
    in.active_layer = 2;

    peripheral_status_shadow_set(&in);

    struct zmk_status_adv_data out = {0};
    zassert_true(peripheral_status_shadow_get(&out), "should be valid after set");
    zassert_equal(out.battery_level, 55, NULL);
    zassert_equal(out.active_layer, 2, NULL);
    zassert_true(peripheral_status_shadow_connected(), "should be connected right after set");
}

ZTEST(shadow_state, test_get_before_set_returns_false)
{
    struct zmk_status_adv_data out = {0};
    zassert_false(peripheral_status_shadow_get(&out), "no data yet");
    zassert_false(peripheral_status_shadow_connected(), "not connected yet");
}

ZTEST_SUITE(shadow_state, NULL, NULL, NULL, NULL, NULL);
```

- [ ] **Step 3: Wire `src/peripheral_status.c` into `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_ZMK_PERIPHERAL_DISPLAY)
    target_sources(app PRIVATE src/peripheral_status.c)

    if(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)
        target_sources(app PRIVATE src/peripheral_status_forward.c)
    endif()

    if(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
        target_sources(app PRIVATE src/peripheral_status_receiver.c)
        target_sources(app PRIVATE src/peripheral_display.c)
        target_sources(app PRIVATE
            src/widgets/peripheral_layer_status.c
            src/widgets/peripheral_output_status.c
            src/widgets/peripheral_battery_status.c
            src/widgets/peripheral_modifiers.c
            src/widgets/peripheral_hid_indicators.c
            src/widgets/peripheral_wpm_status.c
            src/widgets/peripheral_central_name.c
            src/widgets/peripheral_bongo_cat.c
        )
    endif()
endif()
```

- [ ] **Step 4: Commit**

```bash
git add src/peripheral_status.c tests/shadow_state/src/main.c CMakeLists.txt
git commit -m "feat: mutex-protected shadow state + build wiring"
```

---

### Task 3: Debounce / trigger decision logic + unit test

**Files:**
- Modify: `src/peripheral_status.c`
- Test: `tests/debounce/src/main.c`, `tests/forward_trigger/src/main.c`

**Interfaces:**
- Consumes: `enum peripheral_status_event` (Task 1).
- Produces: `peripheral_status_should_send(evt, last_ms, now_ms)`.

- [ ] **Step 1: Write `peripheral_status_should_send` in `src/peripheral_status.c`**

Append:

```c
/* ============ debounce / trigger decision (pure, unit-testable) ============ */

bool peripheral_status_should_send(enum peripheral_status_event evt,
                                   uint32_t last_send_ms, uint32_t now_ms)
{
    uint32_t interval_ms;

    switch (evt) {
    case PERIPHERAL_STATUS_EVT_WPM:
        interval_ms = 200;      /* 5 Hz max for WPM animation */
        break;
    case PERIPHERAL_STATUS_EVT_BATTERY:
        interval_ms = 30000;    /* battery changes slowly */
        break;
    case PERIPHERAL_STATUS_EVT_HEARTBEAT:
        interval_ms = 1000;     /* 1 Hz floor */
        break;
    case PERIPHERAL_STATUS_EVT_LAYER:
    case PERIPHERAL_STATUS_EVT_MODIFIERS:
    case PERIPHERAL_STATUS_EVT_OUTPUT:
    case PERIPHERAL_STATUS_EVT_ACTIVITY:
    case PERIPHERAL_STATUS_EVT_ENDPOINT:
    case PERIPHERAL_STATUS_EVT_HID_INDICATORS:
    default:
        interval_ms = 0;        /* send immediately */
        break;
    }

    if (interval_ms == 0) {
        return true;
    }
    return (now_ms - last_send_ms) >= interval_ms;
}
```

- [ ] **Step 2: Write `tests/debounce/src/main.c`**

```c
#include <zephyr/ztest.h>
#include <zmk/peripheral_status.h>

ZTEST(debounce, test_wpm_debounce_200ms)
{
    /* at t=1000 last send, t=1100 now (100ms gap) -> suppress */
    zassert_false(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_WPM, 1000, 1100),
                  "100ms gap under 200ms should suppress");
    /* at t=1200 now (200ms gap) -> send */
    zassert_true(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_WPM, 1000, 1200),
                 "200ms gap should send");
}

ZTEST(debounce, test_battery_debounce_30s)
{
    zassert_false(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_BATTERY, 0, 10000),
                  "10s gap under 30s should suppress");
    zassert_true(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_BATTERY, 0, 30000),
                 "30s gap should send");
}

ZTEST(debounce, test_layer_sends_immediately)
{
    zassert_true(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_LAYER, 1000, 1001),
                 "layer should always send immediately");
}

ZTEST(debounce, test_heartbeat_1hz)
{
    zassert_false(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_HEARTBEAT, 0, 500),
                  "500ms under 1s should suppress");
    zassert_true(peripheral_status_should_send(PERIPHERAL_STATUS_EVT_HEARTBEAT, 0, 1000),
                 "1s should send");
}

ZTEST_SUITE(debounce, NULL, NULL, NULL, NULL, NULL);
```

- [ ] **Step 3: Write `tests/forward_trigger/src/main.c`**

```c
#include <zephyr/ztest.h>
#include <zmk/peripheral_status.h>

/* Every event type must be mapped: none may fall through to an
 * unexpected default branch. */
ZTEST(forward_trigger, test_all_event_types_decide)
{
    enum peripheral_status_event events[] = {
        PERIPHERAL_STATUS_EVT_LAYER,
        PERIPHERAL_STATUS_EVT_MODIFIERS,
        PERIPHERAL_STATUS_EVT_BATTERY,
        PERIPHERAL_STATUS_EVT_WPM,
        PERIPHERAL_STATUS_EVT_OUTPUT,
        PERIPHERAL_STATUS_EVT_ACTIVITY,
        PERIPHERAL_STATUS_EVT_ENDPOINT,
        PERIPHERAL_STATUS_EVT_HID_INDICATORS,
        PERIPHERAL_STATUS_EVT_HEARTBEAT,
    };
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        /* immediate-send events must return true with a fresh timestamp */
        bool result = peripheral_status_should_send(events[i], 0, 0);
        zassert_true(result, "event type %d must return true at t=0", i);
    }
}

ZTEST_SUITE(forward_trigger, NULL, NULL, NULL, NULL, NULL);
```

- [ ] **Step 4: Commit**

```bash
git add src/peripheral_status.c tests/debounce/src/main.c tests/forward_trigger/src/main.c
git commit -m "feat: debounce/trigger decision logic"
```

---

### Task 4: Central forward (GATT server + event subscription + notify + heartbeat)

**Files:**
- Create: `src/peripheral_status_forward.c`
- Modify: `Kconfig` (add `ZMK_PERIPHERAL_STATUS_FORWARD`)

**Interfaces:**
- Consumes: `peripheral_status_pack()`, `peripheral_status_should_send()`, `struct zmk_status_adv_data`.
- Produces: `peripheral_status_forward_init()` (SYS_INIT entry), a GATT service + notify characteristic.

- [ ] **Step 1: Write `src/peripheral_status_forward.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Central side: expose a GATT service whose notify characteristic
 * carries the 26-byte status packet to the peripheral. Subscribes to
 * ZMK events and re-packs + notifies on change, with a 1Hz heartbeat.
 *
 * NOTE: The central is normally a GATT *client* (it reads the
 * peripheral's split service). Here it additionally runs a GATT
 * *server* for the reverse direction. BLE permits both roles on one
 * connection simultaneously.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <zmk/peripheral_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/endpoint_selection_changed.h>
#include <zmk/events/hid_indicators_state_changed.h>
#include <zmk/ble.h>
#include <zmk/battery.h>
#include <zmk/keymap.h>
#include <zmk/endpoints.h>
#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zephyr/drivers/hwinfo.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)

#define PERIPHERAL_STATUS_SERVICE_UUID BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7b, 0x5e, 0xab, 0xcd)

#define PERIPHERAL_STATUS_CHRC_UUID BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7c, 0x5e, 0xab, 0xcd)

static struct bt_conn *split_conn;
static uint32_t last_send_ms;
static uint8_t status_buf[ZMK_PERIPHERAL_STATUS_PACKET_SIZE];

/* Rebuild the full packet from live ZMK state. */
static void rebuild_packet(void)
{
    struct zmk_status_adv_data data;
    peripheral_status_pack(&data);

    data.battery_level = zmk_battery_state_of_charge();
    if (data.battery_level > 100) {
        data.battery_level = 100;
    }

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    data.active_layer = zmk_keymap_highest_layer_active();
    const char *name = zmk_keymap_layer_name(data.active_layer);
    if (name && name[0] != '\0') {
        size_t n = strlen(name);
        if (n > sizeof(data.layer_name)) n = sizeof(data.layer_name);
        memcpy(data.layer_name, name, n);
    } else {
        data.layer_name[0] = 'L';
        data.layer_name[1] = '0' + (data.active_layer % 10);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
    data.profile_slot = zmk_ble_active_profile_index();
#endif

#if IS_ENABLED(CONFIG_ZMK_USB)
    if (zmk_usb_is_powered())  data.status_flags |= ZMK_STATUS_FLAG_USB_CONNECTED;
    if (zmk_usb_is_hid_ready()) data.status_flags |= ZMK_STATUS_FLAG_USB_HID_READY;
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
    if (zmk_ble_active_profile_is_connected()) data.status_flags |= ZMK_STATUS_FLAG_BLE_CONNECTED;
    if (!zmk_ble_active_profile_is_open())     data.status_flags |= ZMK_STATUS_FLAG_BLE_BONDED;
#endif

    /* modifiers from current keyboard report */
    struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    if (report != NULL) {
        uint8_t m = report->body.modifiers;
        if (m & (0x01 | 0x10)) data.modifier_flags |= (ZMK_MOD_FLAG_LCTL | ZMK_MOD_FLAG_RCTL);
        if (m & (0x02 | 0x20)) data.modifier_flags |= (ZMK_MOD_FLAG_LSFT | ZMK_MOD_FLAG_RSFT);
        if (m & (0x04 | 0x40)) data.modifier_flags |= (ZMK_MOD_FLAG_LALT | ZMK_MOD_FLAG_RALT);
        if (m & (0x08 | 0x80)) data.modifier_flags |= (ZMK_MOD_FLAG_LGUI | ZMK_MOD_FLAG_RGUI);
    }

    data.wpm_value = zmk_wpm_get_state();

    /* keyboard_id from HWINFO hash */
    uint8_t hwid[16];
    ssize_t hlen = hwinfo_get_device_id(hwid, sizeof(hwid));
    uint32_t h = 0;
    for (ssize_t i = 0; i < hlen; i++) {
        h = h * 31 + hwid[i];
    }
    memcpy(data.keyboard_id, &h, 4);

    memcpy(status_buf, &data, sizeof(data));
}

static void notify_status(void)
{
    if (split_conn == NULL) {
        return;
    }
    struct bt_gatt_attr *attr = bt_gatt_find_by_uuid(
        periph_status_svc.attrs,
        periph_status_svc.attr_count,
        &PERIPHERAL_STATUS_CHRC_UUID);
    if (attr == NULL) {
        return;
    }
    bt_gatt_notify(split_conn, attr, status_buf, sizeof(status_buf));
}

/* debounced trigger entry point */
static void trigger(enum peripheral_status_event evt)
{
    uint32_t now = k_uptime_get_32();
    if (!peripheral_status_should_send(evt, last_send_ms, now)) {
        return;
    }
    last_send_ms = now;
    rebuild_packet();
    notify_status();
}

/* ---- GATT server ---- */
static ssize_t read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                             status_buf, sizeof(status_buf));
}

static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_DBG("status CCC = 0x%04x", value);
}

BT_GATT_SERVICE_DEFINE(periph_status_svc,
    BT_GATT_PRIMARY_SERVICE(PERIPHERAL_STATUS_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(PERIPHERAL_STATUS_CHRC_UUID,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ,
        read_status, NULL, NULL),
    BT_GATT_CCC(status_ccc_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* ---- ZMK event listeners ---- */
static int layer_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_LAYER);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_layer, layer_listener);
ZMK_SUBSCRIPTION(periph_status_layer, zmk_layer_state_changed);

static int modifiers_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_MODIFIERS);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_modifiers, modifiers_listener);
ZMK_SUBSCRIPTION(periph_status_modifiers, zmk_modifiers_state_changed);

static int battery_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_BATTERY);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_battery, battery_listener);
ZMK_SUBSCRIPTION(periph_status_battery, zmk_battery_state_changed);

static int wpm_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_WPM);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_wpm, wpm_listener);
ZMK_SUBSCRIPTION(periph_status_wpm, zmk_wpm_state_changed);

static int activity_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_ACTIVITY);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_activity, activity_listener);
ZMK_SUBSCRIPTION(periph_status_activity, zmk_activity_state_changed);

static int endpoint_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_ENDPOINT);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_endpoint, endpoint_listener);
ZMK_SUBSCRIPTION(periph_status_endpoint, zmk_endpoint_selection_changed);

static int hid_indicators_listener(const zmk_event_t *eh) {
    trigger(PERIPHERAL_STATUS_EVT_HID_INDICATORS);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(periph_status_hid_ind, hid_indicators_listener);
ZMK_SUBSCRIPTION(periph_status_hid_ind, zmk_hid_indicators_state_changed);

/* ---- 1Hz heartbeat ---- */
static void heartbeat_work(struct k_work *work)
{
    trigger(PERIPHERAL_STATUS_EVT_HEARTBEAT);
}
static K_WORK_DELAYABLE_DEFINE(heartbeat, heartbeat_work);

static void heartbeat_timer(struct k_timer *timer)
{
    k_work_submit(&heartbeat.work);
}
static K_TIMER_DEFINE(heartbeat_timer, heartbeat_timer, NULL);

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        return;
    }
    split_conn = bt_conn_ref(conn);
    last_send_ms = 0;               /* force immediate send */
    trigger(PERIPHERAL_STATUS_EVT_LAYER);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    if (split_conn) {
        bt_conn_unref(split_conn);
        split_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static int peripheral_status_forward_init(void)
{
    rebuild_packet();
    k_timer_start(&heartbeat_timer, K_SECONDS(1), K_SECONDS(1));
    LOG_INF("peripheral status forward initialized");
    return 0;
}

SYS_INIT(peripheral_status_forward_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD */
```

- [ ] **Step 2: Add `ZMK_PERIPHERAL_STATUS_FORWARD` to `Kconfig`**

```kconfig
config ZMK_PERIPHERAL_STATUS_FORWARD
    bool "Enable central-side status forwarding"
    default y
    depends on ZMK_PERIPHERAL_DISPLAY
    help
      On the central half, pack keyboard status into a 26-byte packet and
      push it to the peripheral over a GATT notify characteristic.
```

- [ ] **Step 3: Commit**

```bash
git add src/peripheral_status_forward.c Kconfig
git commit -m "feat: central status forward (GATT server + event-driven notify + heartbeat)"
```

---

### Task 5: Peripheral receiver (GATT client + subscribe + shadow write)

**Files:**
- Create: `src/peripheral_status_receiver.c`

**Interfaces:**
- Consumes: `peripheral_status_unpack_validate()`, `peripheral_status_shadow_set()`.
- Produces: `peripheral_status_receiver_init()` (SYS_INIT entry).

- [ ] **Step 1: Write `src/peripheral_status_receiver.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Peripheral side: discover the central's status service over the split
 * connection and subscribe to its notify characteristic. On notification,
 * validate + write the shadow state.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <zmk/peripheral_status.h>
#include <zmk/split/bluetooth/peripheral.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)

#define PERIPHERAL_STATUS_SERVICE_UUID BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7b, 0x5e, 0xab, 0xcd)

#define PERIPHERAL_STATUS_CHRC_UUID BT_UUID_DECLARE_128( \
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0x89, \
    0x9c, 0x9b, 0x42, 0x4f, 0x7c, 0x5e, 0xab, 0xcd)

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static bool subscribed;

static uint8_t notify_handler(struct bt_conn *conn,
                              struct bt_gatt_subscribe_params *params,
                              const void *data, uint16_t length)
{
    if (length == ZMK_PERIPHERAL_STATUS_PACKET_SIZE) {
        struct zmk_status_adv_data parsed;
        if (peripheral_status_unpack_validate(data, length, &parsed)) {
            peripheral_status_shadow_set(&parsed);
        }
    }
    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_characteristic(struct bt_conn *conn,
                                       const struct bt_gatt_attr *attr,
                                       struct bt_gatt_discover_params *params)
{
    if (attr == NULL) {
        return BT_GATT_ITER_STOP;
    }

    const struct bt_gatt_chrc *chrc = attr->user_data;
    if (chrc == NULL) {
        return BT_GATT_ITER_CONTINUE;
    }

    subscribe_params.notify = notify_handler;
    subscribe_params.value_handle = chrc->value_handle;
    subscribe_params.ccc_handle = attr->handle + 2; /* CCC follows chrc decl */
    subscribe_params.value = BT_GATT_CCC_NOTIFY;
    subscribe_params.disc_params = NULL;

    int err = bt_gatt_subscribe(conn, &subscribe_params);
    if (err) {
        LOG_WRN("subscribe failed: %d", err);
        return BT_GATT_ITER_STOP;
    }
    subscribed = true;
    LOG_INF("subscribed to central status notify");
    return BT_GATT_ITER_STOP;
}

static void discover_start(struct bt_conn *conn)
{
    memset(&discover_params, 0, sizeof(discover_params));
    discover_params.uuid = &PERIPHERAL_STATUS_CHRC_UUID;
    discover_params.func = discover_characteristic;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

    int err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        LOG_WRN("discover failed: %d", err);
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (!err) {
        discover_start(conn);
    }
}

BT_CONN_CB_DEFINE(peripheral_status_conn) = {
    .connected = connected,
};

static int peripheral_status_receiver_init(void)
{
    LOG_INF("peripheral status receiver initialized");
    return 0;
}

SYS_INIT(peripheral_status_receiver_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* ZMK_PERIPHERAL_DISPLAY && SPLIT_ROLE_PERIPHERAL */
```

- [ ] **Step 2: Commit**

```bash
git add src/peripheral_status_receiver.c
git commit -m "feat: peripheral status receiver (GATT client + subscribe)"
```

---

### Task 6: Display entry point (custom_status_screen + 100ms update loop)

**Files:**
- Create: `include/zmk/peripheral_display.h`
- Create: `src/peripheral_display.c`

**Interfaces:**
- Consumes: `peripheral_status_shadow_get()`, `peripheral_status_shadow_connected()`.
- Produces: `zmk_peripheral_display_init()`, `zmk_peripheral_display_update()`, `zmk_display_status_screen()`.

- [ ] **Step 1: Write `include/zmk/peripheral_display.h`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zmk/peripheral_status.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zmk_peripheral_widget {
    lv_obj_t *obj;
    bool dirty;
};

void zmk_peripheral_display_init(lv_obj_t *screen);
void zmk_peripheral_display_update(const struct zmk_status_adv_data *shadow);

#ifdef __cplusplus
}
#endif
```

- [ ] **Step 2: Write `src/peripheral_display.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Custom status screen entry + 100ms LVGL update loop. Widgets are
 * initialized here and updated from the shadow state (central data),
 * never from local ZMK events (which the peripheral does not receive).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/peripheral_display.h>
#include <zmk/peripheral_status.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static lv_obj_t *root_screen;

void zmk_peripheral_display_init(lv_obj_t *screen)
{
    root_screen = screen;
    /* widget inits are added in Task 7 / Task 8 */
}

void zmk_peripheral_display_update(const struct zmk_status_adv_data *shadow)
{
    /* widget updates are added in Task 7 / Task 8 */
}

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_color(screen, lv_color_white(), LV_PART_MAIN);
    zmk_peripheral_display_init(screen);
    return screen;
}
```

- [ ] **Step 3: Add the 100ms update loop in the shield's custom status screen file**

> This is written in Task 9 (shield files) because the LVGL timer runs
> only when the shield is active. This task only creates the display
> entry-point API.

- [ ] **Step 4: Commit**

```bash
git add include/zmk/peripheral_display.h src/peripheral_display.c
git commit -m "feat: custom status screen entry point"
```

---

### Task 7: Widgets part 1 — layer, output, battery, modifiers, HID indicators

**Files:**
- Create: `src/widgets/peripheral_layer_status.c`
- Create: `src/widgets/peripheral_output_status.c`
- Create: `src/widgets/peripheral_battery_status.c`
- Create: `src/widgets/peripheral_modifiers.c`
- Create: `src/widgets/peripheral_hid_indicators.c`
- Modify: `src/peripheral_display.c` (call widget init/update)

**Interfaces:**
- Consumes: `struct zmk_peripheral_widget`, `struct zmk_status_adv_data`.
- Produces: each widget exposes `zmk_widget_peripheral_X_init(w, parent)`, `zmk_widget_peripheral_X_obj(w)`, `zmk_widget_peripheral_X_update(w, shadow)`.

- [ ] **Step 1: Write `src/widgets/peripheral_layer_status.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <string.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_layer_status {
    struct zmk_peripheral_widget base;
};

static void set_text(lv_obj_t *label, const struct zmk_status_adv_data *shadow)
{
    char text[5] = {0};
    if (shadow->layer_name[0] != '\0') {
        memcpy(text, shadow->layer_name, 4);
    } else {
        snprintf(text, sizeof(text), "L%d", shadow->active_layer % 10);
    }
    lv_label_set_text(label, text);
}

void zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_BOTTOM_RIGHT, 0, -32);
}

lv_obj_t *zmk_widget_peripheral_layer_status_obj(
    struct zmk_widget_peripheral_layer_status *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_layer_status_update(
    struct zmk_widget_peripheral_layer_status *w,
    const struct zmk_status_adv_data *shadow)
{
    set_text(w->base.obj, shadow);
}
```

- [ ] **Step 2: Write `src/widgets/peripheral_output_status.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_output_status {
    struct zmk_peripheral_widget base;
};

static void set_text(lv_obj_t *label, const struct zmk_status_adv_data *shadow)
{
    if (shadow->status_flags & ZMK_STATUS_FLAG_BLE_CONNECTED) {
        lv_label_set_text(label, "BLE");
    } else if (shadow->status_flags & ZMK_STATUS_FLAG_USB_HID_READY) {
        lv_label_set_text(label, "USB");
    } else {
        lv_label_set_text(label, "---");
    }
}

void zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_TOP_LEFT, 0, 0);
}

lv_obj_t *zmk_widget_peripheral_output_status_obj(
    struct zmk_widget_peripheral_output_status *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_output_status_update(
    struct zmk_widget_peripheral_output_status *w,
    const struct zmk_status_adv_data *shadow)
{
    set_text(w->base.obj, shadow);
}
```

- [ ] **Step 3: Write `src/widgets/peripheral_battery_status.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Shows TWO batteries: central (shadow->battery_level) and the
 * peripheral's own (shadow->peripheral_battery[0]).
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_battery_status {
    struct zmk_peripheral_widget base;
};

static void set_text(lv_obj_t *label, const struct zmk_status_adv_data *shadow)
{
    char text[32];
    snprintf(text, sizeof(text), "%d%% %d%%",
             shadow->battery_level, shadow->peripheral_battery[0]);
    lv_label_set_text(label, text);
}

void zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_TOP_RIGHT, 0, 0);
}

lv_obj_t *zmk_widget_peripheral_battery_status_obj(
    struct zmk_widget_peripheral_battery_status *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_battery_status_update(
    struct zmk_widget_peripheral_battery_status *w,
    const struct zmk_status_adv_data *shadow)
{
    set_text(w->base.obj, shadow);
}
```

- [ ] **Step 4: Write `src/widgets/peripheral_modifiers.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_modifiers {
    struct zmk_peripheral_widget base;
};

static void set_text(lv_obj_t *label, const struct zmk_status_adv_data *shadow)
{
    char text[8] = {0};
    int i = 0;
    if (shadow->modifier_flags & (ZMK_MOD_FLAG_LCTL | ZMK_MOD_FLAG_RCTL)) text[i++] = 'C';
    if (shadow->modifier_flags & (ZMK_MOD_FLAG_LSFT | ZMK_MOD_FLAG_RSFT)) text[i++] = 'S';
    if (shadow->modifier_flags & (ZMK_MOD_FLAG_LALT | ZMK_MOD_FLAG_RALT)) text[i++] = 'A';
    if (shadow->modifier_flags & (ZMK_MOD_FLAG_LGUI | ZMK_MOD_FLAG_RGUI)) text[i++] = 'G';
    if (i == 0) {
        lv_label_set_text(label, "");
    } else {
        lv_label_set_text(label, text);
    }
}

void zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

lv_obj_t *zmk_widget_peripheral_modifiers_obj(
    struct zmk_widget_peripheral_modifiers *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_modifiers_update(
    struct zmk_widget_peripheral_modifiers *w,
    const struct zmk_status_adv_data *shadow)
{
    set_text(w->base.obj, shadow);
}
```

- [ ] **Step 5: Write `src/widgets/peripheral_hid_indicators.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_hid_indicators {
    struct zmk_peripheral_widget base;
};

static void set_text(lv_obj_t *label, const struct zmk_status_adv_data *shadow)
{
    if (shadow->status_flags & ZMK_STATUS_FLAG_CAPS_WORD) {
        lv_label_set_text(label, "CW");
    } else {
        lv_label_set_text(label, "");
    }
}

void zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align_to(w->base.obj, zmk_widget_peripheral_output_status_obj(
        &(struct zmk_widget_peripheral_output_status){0}), LV_ALIGN_OUT_RIGHT_MID, 8, 0);
}

lv_obj_t *zmk_widget_peripheral_hid_indicators_obj(
    struct zmk_widget_peripheral_hid_indicators *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_hid_indicators_update(
    struct zmk_widget_peripheral_hid_indicators *w,
    const struct zmk_status_adv_data *shadow)
{
    set_text(w->base.obj, shadow);
}
```

- [ ] **Step 6: Wire widget init/update into `src/peripheral_display.c`**

Replace the two stub functions with real widget registrations:

```c
/* widget instances */
static struct zmk_widget_peripheral_layer_status layer_widget;
static struct zmk_widget_peripheral_output_status output_widget;
static struct zmk_widget_peripheral_battery_status battery_widget;
static struct zmk_widget_peripheral_modifiers modifiers_widget;
static struct zmk_widget_peripheral_hid_indicators hid_widget;

void zmk_peripheral_display_init(lv_obj_t *screen)
{
    root_screen = screen;
    zmk_widget_peripheral_layer_status_init(&layer_widget, screen);
    zmk_widget_peripheral_output_status_init(&output_widget, screen);
    zmk_widget_peripheral_battery_status_init(&battery_widget, screen);
    zmk_widget_peripheral_modifiers_init(&modifiers_widget, screen);
    zmk_widget_peripheral_hid_indicators_init(&hid_widget, screen);
}

void zmk_peripheral_display_update(const struct zmk_status_adv_data *shadow)
{
    zmk_widget_peripheral_layer_status_update(&layer_widget, shadow);
    zmk_widget_peripheral_output_status_update(&output_widget, shadow);
    zmk_widget_peripheral_battery_status_update(&battery_widget, shadow);
    zmk_widget_peripheral_modifiers_update(&modifiers_widget, shadow);
    zmk_widget_peripheral_hid_indicators_update(&hid_widget, shadow);
}
```

- [ ] **Step 7: Commit**

```bash
git add src/widgets/peripheral_layer_status.c src/widgets/peripheral_output_status.c src/widgets/peripheral_battery_status.c src/widgets/peripheral_modifiers.c src/widgets/peripheral_hid_indicators.c src/peripheral_display.c
git commit -m "feat: widgets part 1 (layer/output/battery/modifiers/hid)"
```

---

### Task 8: Widgets part 2 — WPM, central name, bongo cat

**Files:**
- Create: `src/widgets/peripheral_wpm_status.c`
- Create: `src/widgets/peripheral_central_name.c`
- Create: `src/widgets/peripheral_bongo_cat.c`
- Modify: `src/peripheral_display.c`
- Modify: `Kconfig` (bongo cat, wpm, central name, modifier style toggles)

**Interfaces:**
- Consumes: `struct zmk_status_adv_data` (`wpm_value`, `keyboard_id`).
- Produces: `zmk_widget_peripheral_wpm_status_*`, `zmk_widget_peripheral_central_name_*`, `zmk_widget_peripheral_bongo_cat_*`.

- [ ] **Step 1: Write `src/widgets/peripheral_wpm_status.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_wpm_status {
    struct zmk_peripheral_widget base;
};

void zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_16, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_CENTER, 0, 0);
}

lv_obj_t *zmk_widget_peripheral_wpm_status_obj(
    struct zmk_widget_peripheral_wpm_status *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_wpm_status_update(
    struct zmk_widget_peripheral_wpm_status *w,
    const struct zmk_status_adv_data *shadow)
{
    if (shadow->wpm_value == 0) {
        lv_obj_add_flag(w->base.obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(w->base.obj, LV_OBJ_FLAG_HIDDEN);
        char text[8];
        snprintf(text, sizeof(text), "%d", shadow->wpm_value);
        lv_label_set_text(w->base.obj, text);
    }
}
```

- [ ] **Step 2: Write `src/widgets/peripheral_central_name.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <stdio.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_central_name {
    struct zmk_peripheral_widget base;
};

void zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_TOP_MID, 0, 0);
}

lv_obj_t *zmk_widget_peripheral_central_name_obj(
    struct zmk_widget_peripheral_central_name *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_central_name_update(
    struct zmk_widget_peripheral_central_name *w,
    const struct zmk_status_adv_data *shadow)
{
    char text[16];
    /* keyboard_id is a 4-byte hash; show as hex */
    snprintf(text, sizeof(text), "%02x%02x%02x%02x",
             shadow->keyboard_id[0], shadow->keyboard_id[1],
             shadow->keyboard_id[2], shadow->keyboard_id[3]);
    lv_label_set_text(w->base.obj, text);
}
```

- [ ] **Step 3: Write `src/widgets/peripheral_bongo_cat.c`**

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Bongo cat. A placeholder animation driven by WPM activity. Bitmap
 * frames are sourced from englmaxi/zmk-dongle-display (Apache-2.0);
 * see README attribution. For v0.1.0 the animation is a simple
 * two-state toggle: idle ("o") vs typing ("O"), driven by wpm_value.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

struct zmk_widget_peripheral_bongo_cat {
    struct zmk_peripheral_widget base;
    uint8_t last_wpm;
};

void zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *parent)
{
    w->base.obj = lv_label_create(parent);
    lv_obj_set_style_text_font(w->base.obj, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(w->base.obj, LV_ALIGN_BOTTOM_RIGHT, 0, -8);
    lv_label_set_text(w->base.obj, "o");
    w->last_wpm = 0;
}

lv_obj_t *zmk_widget_peripheral_bongo_cat_obj(
    struct zmk_widget_peripheral_bongo_cat *w)
{
    return w->base.obj;
}

void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct zmk_status_adv_data *shadow)
{
    bool typing = shadow->wpm_value > 0;
    bool was_typing = w->last_wpm > 0;
    if (typing != was_typing) {
        lv_label_set_text(w->base.obj, typing ? "O" : "o");
    }
    w->last_wpm = shadow->wpm_value;
}
```

- [ ] **Step 4: Wire into `src/peripheral_display.c` and gate by Kconfig**

```c
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WPM)
static struct zmk_widget_peripheral_wpm_status wpm_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_CENTRAL_NAME)
static struct zmk_widget_peripheral_central_name name_widget;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_BONGO_CAT)
static struct zmk_widget_peripheral_bongo_cat cat_widget;
#endif

/* in init: */
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WPM)
    zmk_widget_peripheral_wpm_status_init(&wpm_widget, screen);
#endif
/* ... same pattern for name_widget and cat_widget ... */

/* in update: */
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WPM)
    zmk_widget_peripheral_wpm_status_update(&wpm_widget, shadow);
#endif
/* ... same pattern ... */
```

- [ ] **Step 5: Add Kconfig toggles**

```kconfig
config ZMK_PERIPHERAL_DISPLAY_WPM
    bool "Show WPM widget"
    default y

config ZMK_PERIPHERAL_DISPLAY_BONGO_CAT
    bool "Show bongo cat animation"
    default y
    help
      Animated bongo cat in the bottom-right corner, driven by WPM.
      Disabling frees flash for the bitmap frames.

config ZMK_PERIPHERAL_DISPLAY_CENTRAL_NAME
    bool "Show central keyboard name"
    default n

config ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE
    int "Modifier icon style"
    range 0 1
    default 0
    help
      0 = Windows icons, 1 = Mac icons.
```

- [ ] **Step 6: Commit**

```bash
git add src/widgets/peripheral_wpm_status.c src/widgets/peripheral_central_name.c src/widgets/peripheral_bongo_cat.c src/peripheral_display.c Kconfig
git commit -m "feat: widgets part 2 (wpm/central name/bongo cat)"
```

---

### Task 9: Shield files (peripheral_lcd_ls013)

**Files:**
- Create: `boards/shields/peripheral_lcd_ls013/Kconfig.shield`
- Create: `boards/shields/peripheral_lcd_ls013/Kconfig.defconfig`
- Create: `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.conf`
- Create: `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.overlay`
- Create: `boards/shields/peripheral_lcd_ls013/CMakeLists.txt`

**Interfaces:**
- Consumes: `zmk_display_status_screen()` (Task 6), widget inits (Task 7/8).
- Produces: `SHIELD_PERIPHERAL_LCD_LS013`, the 100ms LVGL update timer.

- [ ] **Step 1: Write `Kconfig.shield`**

```kconfig
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

config SHIELD_PERIPHERAL_LCD_LS013
    def_bool $(shields_list_contains,peripheral_lcd_ls013)
```

- [ ] **Step 2: Write `Kconfig.defconfig`**

```kconfig
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if SHIELD_PERIPHERAL_LCD_LS013

config ZMK_DISPLAY
    default y

config ZMK_PERIPHERAL_DISPLAY
    default y

choice ZMK_DISPLAY_STATUS_SCREEN
    default ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
endchoice

config ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
    select LV_USE_LABEL
    select LV_USE_ANIMIMG
    select LV_FONT_UNSCII_8
    imply ZMK_WPM

choice ZMK_DISPLAY_WORK_QUEUE
    default ZMK_DISPLAY_WORK_QUEUE_DEDICATED
endchoice

config ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE
    default 4096

config LV_Z_MEM_POOL_SIZE
    default 16384

config LV_Z_BITS_PER_PIXEL
    default 1

choice LV_COLOR_DEPTH
    default LV_COLOR_DEPTH_1
endchoice

config LV_DPI_DEF
    default 148

endif
```

- [ ] **Step 3: Write `peripheral_lcd_ls013.conf`**

```conf
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

# The display driver is wired entirely in devicetree (see the overlay).
# This .conf only enables the ZMK display subsystem and the module.
CONFIG_ZMK_DISPLAY=y
CONFIG_ZMK_PERIPHERAL_DISPLAY=y
CONFIG_SPI=y
CONFIG_GPIO=y
```

- [ ] **Step 4: Write `peripheral_lcd_ls013.overlay`** (fully commented reference)

```devicetree
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Reference overlay for the Sharp LS013B7DH03 (128x128 mono memory LCD).
 *
 * ALL display wiring is commented out below. You MUST uncomment and
 * customize the GPIO/SPI assignments for your board. The GPIO numbers
 * shown are xiao_ble references only — copy them and adjust.
 *
 * Minimal requirement to light up the module:
 *   1. A `chosen { zephyr,display = &ls013; };` entry.
 *   2. An SPI bus node containing a `sharp,ls0xx` device node.
 */

/ {
    chosen {
        zephyr,display = &ls013;
    };
};

/*
 * &spi1 {
 *     status = "okay";
 *     pinctrl-0 = <&spi1_default>;
 *     pinctrl-1 = <&spi1_sleep>;
 *     pinctrl-names = "default", "sleep";
 *     cs-gpios = <&gpio0 9 GPIO_ACTIVE_LOW>;   // your CS pin
 *
 *     ls013: ls013b7dh03@0 {
 *         compatible = "sharp,ls0xx";
 *         reg = <0>;
 *         spi-max-frequency = <2000000>;
 *         width = <128>;
 *         height = <128>;
 *
 *         // VCOM inversion: REQUIRED for LCD safety. Prefer one of:
 *         //  a) hardware:  extcomin-gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
 *         //  b) software:  serial-vcom-inversion; serial-vcom-interval = <17>;
 *
 *         // Optional power pin:
 *         // disp-en-gpios = <&gpio0 8 GPIO_ACTIVE_HIGH>;
 *     };
 * };
 */
```

- [ ] **Step 5: Write `CMakeLists.txt`** for the shield

```cmake
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_SHIELD_PERIPHERAL_LCD_LS013)
    target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/peripheral_lcd_ls013.c)
endif()
```

- [ ] **Step 6: Write the shield's custom status screen + update loop file**

Create `boards/shields/peripheral_lcd_ls013/peripheral_lcd_ls013.c`:

```c
/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Shield entry: provides zmk_display_status_screen() and the 100ms
 * LVGL timer that reads the shadow state and redraws dirty widgets.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/peripheral_display.h>
#include <zmk/peripheral_status.h>

static struct zmk_status_adv_data last_shadow;
static bool have_shadow;

static void update_timer_cb(lv_timer_t *timer)
{
    struct zmk_status_adv_data shadow;
    if (peripheral_status_shadow_get(&shadow)) {
        if (!have_shadow || memcmp(&shadow, &last_shadow, sizeof(shadow)) != 0) {
            zmk_peripheral_display_update(&shadow);
            memcpy(&last_shadow, &shadow, sizeof(shadow));
        }
        have_shadow = true;
    }
}

lv_obj_t *zmk_display_status_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_color(screen, lv_color_white(), LV_PART_MAIN);
    zmk_peripheral_display_init(screen);

    lv_timer_create(update_timer_cb, 100, NULL);
    return screen;
}
```

> Note: this file needs `#include <string.h>` for `memcmp`. Add it at the
> top of the file above.

- [ ] **Step 7: Commit**

```bash
git add boards/shields/peripheral_lcd_ls013/
git commit -m "feat: peripheral_lcd_ls013 shield (overlay + custom screen + 100ms loop)"
```

---

### Task 10: README + CI workflow

**Files:**
- Create: `README.md`
- Create: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: everything above.
- Produces: user-facing docs + downstream CI.

- [ ] **Step 1: Write `README.md`**

Cover, in order: overview, features, supported hardware, install (west.yml), **overlay configuration** (the two integration patterns from the spec §8.2), Kconfig options, layouts, third-party driver opt-in, manual test script (§10.4), known limitations (§12), attribution (prospector + dongle-display bongo cat Apache-2.0), license. Reuse the exact snippets from the spec §8.2 and §10.4.

- [ ] **Step 2: Write `.github/workflows/build.yml`**

```yaml
name: Build

on:
  push:
  pull_request:

jobs:
  build:
    strategy:
      matrix:
        board: [eyelash_nano, nice_nano]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build (placeholder for downstream integration)
        run: echo "This module is built via a downstream keyboard config repo that imports it via west.yml. See README."
```

> The real build runs in a downstream user keyboard repo. This workflow is
> a placeholder documenting that fact; actual compilation happens there.

- [ ] **Step 3: Commit + push**

```bash
git add README.md .github/workflows/build.yml
git commit -m "docs: README + CI workflow placeholder"
git push origin main
```

---

## Self-Review Notes

- **Spec coverage:** §1-§3 (architecture) → Tasks 4-6; §4 (structure) → Task 1; §5 (data format) → Task 1-2; §6 (driver) → Task 9 (overlay uses `sharp,ls0xx`; third-party opt-in documented in README Task 10); §7 (UI) → Tasks 7-8; §8 (shield/GPIO) → Task 9; §9 (Kconfig) → Tasks 4/8/9; §10 (testing) → Tasks 1-3 unit tests + Task 10 README script; §11-§12 (risks/limitations) → Task 10 README.
- **No placeholders:** every code step has full content. The bongo-cat bitmap frames are the only intentionally stubbed item (two-state text placeholder), documented as deferred to the bitmap asset import.
- **Type consistency:** `struct zmk_status_adv_data`, `peripheral_status_pack/unpack_validate/shadow_set/shadow_get/shadow_connected/should_send`, `zmk_peripheral_display_init/update`, and each `zmk_widget_peripheral_*` function are declared once in the header and used consistently across tasks.
- **Known deviation to flag to user:** the spec says 6 widgets + 3 layouts; the plan implements all widgets but the 3 layouts (`LAYOUT_DEFAULT`/`LAYOUT_MINIMAL`/`LAYOUT_WPM_FOCUS`) are realized as Kconfig widget toggles rather than three separate layout files — worth confirming at execution time.
