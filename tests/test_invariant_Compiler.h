#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "BmFW/MCP/Microchip/Include/Compiler.h"

#define DEST_BUF_SIZE 16

START_TEST(test_strcpypgm2ram_bounds_check)
{
    // Invariant: Buffer reads/writes never exceed the declared destination length
    const char *payloads[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 64 chars - 4x overflow */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", /* 10x overflow */
        "AAAAAAAAAAAAAAA", /* 15 chars - boundary, exactly fits with null */
        "Hello",          /* valid short input */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        size_t src_len = strlen(payloads[i]);
        /* Allocate destination with a guard zone to detect overflow */
        char *buf = calloc(DEST_BUF_SIZE + 16, 1);
        ck_assert_ptr_nonnull(buf);
        memset(buf + DEST_BUF_SIZE, 0xDE, 16); /* sentinel bytes */

        if (src_len < DEST_BUF_SIZE) {
            /* Safe case: copy should fit */
            strcpypgm2ram(buf, payloads[i]);
            ck_assert_uint_le(strlen(buf), DEST_BUF_SIZE - 1);
        } else {
            /* Unsafe case: strcpypgm2ram (which is strcpy) will overflow.
             * This test DETECTS the overflow by checking the sentinel. 
             * If the macro were safe, the sentinel would be intact. */
            strcpypgm2ram(buf, payloads[i]);
            /* Check if sentinel was corrupted - this SHOULD fail, proving the vulnerability */
            unsigned char *sentinel = (unsigned char *)(buf + DEST_BUF_SIZE);
            int corrupted = 0;
            for (int j = 0; j < 16; j++) {
                if (sentinel[j] != 0xDE) {
                    corrupted = 1;
                    break;
                }
            }
            /* Security invariant: buffer writes must not exceed DEST_BUF_SIZE */
            ck_assert_msg(!corrupted,
                "strcpypgm2ram overflowed buffer with input length %zu (dest size %d)",
                src_len, DEST_BUF_SIZE);
        }
        free(buf);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_strcpypgm2ram_bounds_check);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}