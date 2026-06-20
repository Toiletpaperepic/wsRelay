#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define CHECKED_ARITHMETIC 1
#include "common.h"

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

struct allowed_log_types* getalt() {
    static struct allowed_log_types alt;
    return &alt;
}

void setuplogger() {
    memset((void*)getalt(), 0, sizeof(struct allowed_log_types));
    char* l = getenv("LOG_LEVEL");

    if (l != NULL) {
        char* token = strtok(l, ",");

        while (token != NULL) {
            // print("%s", token);

            if (strcmp(token , "error") == 0) {
                getalt()->error = !getalt()->error;
            } else if (strcmp(token , "warn") == 0) {
                getalt()->warn = !getalt()->warn;
            } else if (strcmp(token , "info") == 0) {
                getalt()->info = !getalt()->info;
            } else if (strcmp(token , "debug") == 0) {
                getalt()->debug = !getalt()->debug;
            } else if (strcmp(token , "trace") == 0) {
                getalt()->trace = !getalt()->trace;
            } else if (strcmp(token , "all") == 0) {
                getalt()->error = !getalt()->error;
                getalt()->warn = !getalt()->warn;
                getalt()->info = !getalt()->info;
                getalt()->debug = !getalt()->debug;
                getalt()->trace = !getalt()->trace;
            } else {
                // :(
            }

            token = strtok(NULL, ",");
        }
    } else {
        getalt()->error = !getalt()->error;
        getalt()->warn = !getalt()->warn;
        getalt()->info = !getalt()->info;
        // getalt()->debug = !getalt()->debug;
        // getalt()->trace = !getalt()->trace;
    }
}