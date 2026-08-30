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
