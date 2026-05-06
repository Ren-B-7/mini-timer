#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/minunit.h"
#include "../src/include/stringlib.h"

int tests_run = 0;

static char* test_string_duplicate(void) {
    const char* original = "hello";
    char* duplicated = s_duplicate(original);
    mu_assert("strings not equal", s_cmp(original, duplicated) == 0);
    free(duplicated);
    return 0;
}

static char* test_string_reverse(void) {
    char* str = s_duplicate("hello");
    s_reverse(str);
    mu_assert("string not reversed correctly", s_cmp(str, "olleh") == 0);
    free(str);
    return 0;
}

static char* test_string_trim(void) {
    char* str = s_duplicate("  hello world  ");
    s_trim(str);
    mu_assert("string not trimmed correctly", s_cmp(str, "hello world") == 0);
    free(str);
    return 0;
}

static char* test_string_toupper(void) {
    char* str = s_duplicate("hello");
    s_toupper(str);
    mu_assert("string not converted to upper", s_cmp(str, "HELLO") == 0);
    free(str);
    return 0;
}

static char* all_tests(void) {
    mu_run_test(test_string_duplicate);
    mu_run_test(test_string_reverse);
    mu_run_test(test_string_trim);
    mu_run_test(test_string_toupper);
    return 0;
}

int main(void) {
    char* result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
