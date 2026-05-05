#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>
#include <string.h>

#define mu_assert(message, test) do { if (!(test)) { \
    printf("%s failed: %s\n", __func__, message); \
    return (char*)message; \
} } while (0)

#define mu_run_test(test) do { char *message = test(); tests_run++; \
    if (message) return message; } while (0)

extern int tests_run;

#endif
