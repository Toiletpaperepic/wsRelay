#include <stdint.h>
#define CHECKED_ARITHMETIC 1
#include "common.h"

int32_t strtoint(const char* str) {
    int32_t num = 0;

    if (str[0] == '-'){
        for (int i = 1; str[i] != '\0'; i++) {
            if (str[i] >= 48 && str[i] <= 57) {
                int32_t result = 0;
                
                if (CHECKED_MUL(&result, num, 10))
                    return INT32_MAX;
                if (CHECKED_ADD(&result, result, str[i] - 48))
                    return INT32_MAX;

                num = result;
            }
            else {
                return INT32_MAX;
            }
        }
        num = -num;
    } else {
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] >= 48 && str[i] <= 57) {
                int32_t result = 0;
                
                if (CHECKED_MUL(&result, num, 10))
                    return INT32_MAX;
                if (CHECKED_ADD(&result, result, str[i] - 48))
                    return INT32_MAX;

                num = result;
            }
            else {
                return INT32_MAX;
            }
        }
    }

    return num;
}

uint32_t strtouint(const char* str) {
    uint32_t num = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 48 && str[i] <= 57) {
            uint32_t result = 0;

            if (CHECKED_MUL(&result, num, 10))
                return UINT32_MAX;
            if (CHECKED_ADD(&result, result, str[i] - 48))
                return UINT32_MAX;

            num = result;
        }
        else {
            return UINT32_MAX;
        }
    }

    return num;
}