#include <stdbool.h>
#include <stdint.h>
#define STRING_TO_INT_CONVERSION 1
#include "strtouint.h"
#define CHECKED_ARITHMETIC 1
#include "other.h"

#define STR_TO_X_BUILDER(type, is_negative)                                                                                                       \
    for (type i = is_negative ? 1 : 0; str[i] != '\0'; i++) {                                                                                     \
        if (str[i] >= 48 && str[i] <= 57) {                                                                                                       \
            if (CHECKED_MUL(result, *result, 10))                                                                                                 \
                return FAILURE;                                                                                                                   \
            if (CHECKED_ADD(result, *result, str[i] - 48))                                                                                        \
                return FAILURE;                                                                                                                   \
        }                                                                                                                                         \
        else {                                                                                                                                    \
            return FAILURE;                                                                                                                       \
        }                                                                                                                                         \
    }                                                                                                                                             \

bool strtoint32(int32_t* result, const char* str) {
    if (str[0] == '-') {
        STR_TO_X_BUILDER(int32_t, true);
        *result = -*result;
    } else {
        STR_TO_X_BUILDER(int32_t, false);
    }
    return SUCCESS;
}

bool strtouint32(uint32_t* result, const char* str) {
    STR_TO_X_BUILDER(uint32_t, false);
    return SUCCESS;
}

// isn't used yet
// bool strtoint16(int16_t* result, const char* str) {
//     if (str[0] == '-') {
//         STR_TO_X_BUILDER(int16_t, true);
//         *result = -*result;
//     } else {
//         STR_TO_X_BUILDER(int16_t, false);
//     }
//     return SUCCESS;
// }

bool strtouint16(uint16_t* result, const char* str) {
    STR_TO_X_BUILDER(uint16_t, false);
    return SUCCESS;
}