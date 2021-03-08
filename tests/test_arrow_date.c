/*
 * Copyright (c) 2020 Snowflake Computing, Inc. All rights reserved.
 */
#include <string.h>
#include "utils/test_setup.h"

/**
 * Utility struct to help with building test cases.
 * c1, c2 and c3 refer to the columns in the test table, t, used throughout this test set.
 *
 * - c1: The positional index of the test case.
 * - c2: A pointer to an input number value to test.
 *       Note: the number is represented as a string to use arbitrary precision and scale.
 * - c2_out: A pointer to the char buffer to store the value retrieved from the DB.
 * - c2_len: The length of the output value.
 * - c2_is_null: A flag indicating whether the input number value is NULL or not.
 * - error_code: The error code that a test case is expected to fail with.
 *     - 100040 for "Date '___' is not recognized"
 */
typedef struct test_case_to_string {
    const int64 c1;
    const char * c2;
    const size_t c2_len;
    const char * c2_out;
    const sf_bool c2_is_null;
    SF_STATUS error_code;
} TEST_CASE_TO_STRING;

/**
 * Test set containing all individual test cases to run.
 */
void test_arrow_date(void** unused) {
    // Test cases.
    const char * empty         = "";
    // Format 1: YYYY-MM-DD
    const char * f1_trash       = "ABCD-EF-GH";
    const char * f1_neg         = "-1970-01-01";
    const char * f1_min         = "0000-01-01";
    const char * f1_max         = "9999-12-31";
    const char * f1_epoch_eve   = "1969-12-31";
    const char * f1_epoch       = "1970-01-01";
    const char * f1_epochalypse = "2038-01-20";
    const char * f1_modern      = "2020-12-01";
    const char * f1_zero_day    = "1970-01-00";
    const char * f1_zero_month  = "1970-00-01";
    const char * f1_oob_day     = "1970-01-32";
    const char * f1_oob_month   = "1970-13-01";

    // All cases with an expected error code go at the end to avoid
    // fragmentation of test cases indices.
    TEST_CASE_TO_STRING test_cases[] = {
        { .c1 =  0, .c2 = NULL,           .c2_len = 0,  .c2_out = NULL,           .c2_is_null = SF_BOOLEAN_TRUE },
        { .c1 =  1, .c2 = f1_min,         .c2_len = 10, .c2_out = "0-01-01",      .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  2, .c2 = f1_max,         .c2_len = 10, .c2_out = f1_max,         .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  3, .c2 = f1_epoch_eve,   .c2_len = 10, .c2_out = f1_epoch_eve,   .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  4, .c2 = f1_epoch,       .c2_len = 10, .c2_out = f1_epoch,       .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  5, .c2 = f1_epochalypse, .c2_len = 10, .c2_out = f1_epochalypse, .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  6, .c2 = f1_modern,      .c2_len = 10, .c2_out = f1_modern,      .c2_is_null = SF_BOOLEAN_FALSE },
        { .c1 =  7, .c2 = empty,          .c2_len = 0,  .c2_out = "",             .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 =  8, .c2 = f1_trash,       .c2_len = 0,  .c2_out = "",             .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 =  9, .c2 = f1_neg,         .c2_len = 0,  .c2_out = "",             .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 = 10, .c2 = f1_zero_day,    .c2_len = 10, .c2_out = f1_zero_day,    .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 = 11, .c2 = f1_zero_month,  .c2_len = 10, .c2_out = f1_zero_month,  .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 = 12, .c2 = f1_oob_day,     .c2_len = 10, .c2_out = f1_oob_day,     .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 },
        { .c1 = 13, .c2 = f1_oob_month,   .c2_len = 10, .c2_out = f1_oob_month,   .c2_is_null = SF_BOOLEAN_FALSE, .error_code = 100040 }
    };

    SF_CONNECT *sf = setup_snowflake_connection();

    SF_STATUS status = snowflake_connect(sf);
    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sf->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);

    // Create a statement object and reuse it for all test cases.
    SF_STMT *sfstmt = snowflake_stmt(sf);

    // Configure this session to use Arrow format.
    status = snowflake_query(
        sfstmt,
        "alter session set C_API_QUERY_RESULT_FORMAT=ARROW_FORCE",
        0);
    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);

    // NOTE: The numeric type here should fit into int64.
    // Otherwise, it is taken as a float.
    status = snowflake_query(
        sfstmt,
        "create or replace table t (c1 int, c2 date)",
        0);
    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);

    status = snowflake_prepare(
        sfstmt,
        "insert into t(c1,c2) values(?,?)",
        0);

    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);

    // Loop over the test cases via index and insert them one by one.
    size_t i;
    size_t num_test_cases = sizeof(test_cases) / sizeof(TEST_CASE_TO_STRING);
    size_t num_successful_inserts = 0;

    for (i = 0; i < num_test_cases; i++) {
        TEST_CASE_TO_STRING tc = test_cases[i];

        // Bind input c1.
        SF_BIND_INPUT in_c1 = { 0 };
        in_c1.idx = 1;
        in_c1.name = NULL;
        in_c1.c_type = SF_C_TYPE_INT64;
        in_c1.value = (void*) &tc.c1;
        in_c1.len = sizeof(tc.c1);

        status = snowflake_bind_param(sfstmt, &in_c1);
        if (status != SF_STATUS_SUCCESS) {
            dump_error(&(sfstmt->error));
        }
        assert_int_equal(status, SF_STATUS_SUCCESS);

        // Bind input c2.
        SF_BIND_INPUT in_c2 = { 0 };
        in_c2.idx = 2;
        in_c2.name = NULL;
        in_c2.c_type = SF_C_TYPE_STRING;
        in_c2.value = (void*) tc.c2;
        in_c2.len = tc.c2_len;

        status = snowflake_bind_param(sfstmt, &in_c2);
        if (status != SF_STATUS_SUCCESS) {
            dump_error(&(sfstmt->error));
        }
        assert_int_equal(status, SF_STATUS_SUCCESS);

        // Execute.
        // If the error_code member is non-zero, then we expect the query to fail.
        // In that case, ensure the error code matches with what is expected.
        // Otherwise, proceed as normal.
        status = snowflake_execute(sfstmt);
        if (tc.error_code == SF_STATUS_SUCCESS) {
            if (status != SF_STATUS_SUCCESS) {
                dump_error(&(sfstmt->error));
            }
            ++num_successful_inserts;
        } else {
            SF_ERROR_STRUCT * err = snowflake_stmt_error(sfstmt);
            assert_int_equal(tc.error_code, err->error_code);
        }
    }

    // Query the table and check for correctness.
    status = snowflake_query(sfstmt, "select * from t", 0);
    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);
    assert_int_equal(snowflake_num_rows(sfstmt), num_successful_inserts);

    int64 c1 = 0;
    char *c2 = NULL;
    size_t c2_len = 0;
    size_t c2_max_size = 0;

    int64 curr_row;
    int64 last_read_row = 0;

    while ((status = snowflake_fetch(sfstmt)) == SF_STATUS_SUCCESS) {
        // Skip past column 1 as it does not contain meaningful test data.
        for (curr_row = last_read_row; curr_row < num_successful_inserts; ++curr_row) {
            snowflake_next(sfstmt);
        }

        // Test column 2.
        for (curr_row = last_read_row; curr_row < num_successful_inserts; ++curr_row) {
            // Valid if value copied to c2 matches the value in tc.c2.
            // c2_len and c2_max_size are unused.
            TEST_CASE_TO_STRING tc = test_cases[curr_row];
            if (tc.c2 != NULL) {
                snowflake_column_as_str(sfstmt, 2, &c2, &c2_len, &c2_max_size);
                snowflake_next(sfstmt);
                assert_string_equal(tc.c2_out, c2);
            } else {
                sf_bool c2_is_null;
                snowflake_column_is_null(sfstmt, 2, &c2_is_null);
                snowflake_next(sfstmt);
                assert_true(tc.c2_is_null == c2_is_null);
            }
        }

        last_read_row = curr_row;
    }

    // Clean-up.
    if (status != SF_STATUS_EOF) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_EOF);

    status = snowflake_query(sfstmt, "drop table if exists t", 0);
    if (status != SF_STATUS_SUCCESS) {
        dump_error(&(sfstmt->error));
    }
    assert_int_equal(status, SF_STATUS_SUCCESS);

    free(c2);
    c2 = NULL;
    snowflake_stmt_term(sfstmt);
    snowflake_term(sf);
}

int main(void) {
    initialize_test(SF_BOOLEAN_FALSE);
    const struct CMUnitTest tests[] = { cmocka_unit_test(test_arrow_date) };
    int ret = cmocka_run_group_tests(tests, NULL, NULL);
    snowflake_global_term();
    return ret;
}