#ifndef LIB_UTIL_PASS_FAIL_H__
#define LIB_UTIL_PASS_FAIL_H__

#include <stdio.h>

#define PF_TEST(condition, message) do {if (condition) {                \
          fprintf(stderr, "PASS %s %d: %s\n", __FILE__, __LINE__, message); \
        } else { \
          fprintf(stderr, "FAIL %s %d: %s\n", __FILE__, __LINE__, message); \
    } } while(false)


#endif //  LIB_UTIL_PASS_FAIL_H__
