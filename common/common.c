#include <stdint.h>
#define CHECKED_ARITHMETIC 1
#include "common.h"

#define STR_TO_X_BUILDER(type, type_max)                                                                                                          \
    for (type i = 1; str[i] != '\0'; i++) {                                                                                                       \
        if (str[i] >= 48 && str[i] <= 57) {                                                                                                       \
            type result = 0;                                                                                                                      \
                                                                                                                                                  \
            if (CHECKED_MUL(&result, num, 10))                                                                                                    \
                return type_max;                                                                                                                  \
            if (CHECKED_ADD(&result, result, str[i] - 48))                                                                                        \
                return type_max;                                                                                                                  \
                                                                                                                                                  \
            num = result;                                                                                                                         \
        }                                                                                                                                         \
        else {                                                                                                                                    \
            return type_max;                                                                                                                      \
        }                                                                                                                                         \
    }                                                                                                                                             \

int32_t strtoint32(const char* str) {
    int32_t num = 0;

    if (str[0] == '-') {
        STR_TO_X_BUILDER(int32_t, INT32_MAX);
        num = -num;
    } else {
        STR_TO_X_BUILDER(int32_t, INT32_MAX);
    }

    return num;
}

uint32_t strtouint32(const char* str) {
    uint32_t num = 0;
    STR_TO_X_BUILDER(uint32_t, UINT32_MAX);
    return num;
}

int16_t strtoint16(const char* str) {
    int16_t num = 0;

    if (str[0] == '-') {
        STR_TO_X_BUILDER(int16_t, INT16_MAX);
        num = -num;
    } else {
        STR_TO_X_BUILDER(int16_t, INT16_MAX);
    }

    return num;
}

uint16_t strtouint16(const char* str) {
    uint16_t num = 0;
    STR_TO_X_BUILDER(uint16_t, UINT16_MAX);
    return num;
}