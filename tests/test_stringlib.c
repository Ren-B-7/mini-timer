#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "include/minunit.h"
#include "../src/include/stringlib.h"

int tests_run = 0;

static char* test_string_duplicate(void) {
    const char* original = "hello";
    char* duplicated = s_duplicate(original);
    mu_assert("strings not equal", s_cmp(original, duplicated) == 0);
    return 0;
}

static char* all_tests(void) {
    mu_run_test(test_string_duplicate);
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
