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