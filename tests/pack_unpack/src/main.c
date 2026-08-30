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