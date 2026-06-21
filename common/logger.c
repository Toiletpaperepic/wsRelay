#include "logger.h"
#if LOGGER_COMPILE_OUT != 1 
#include <stdlib.h>
#include <string.h>

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
#endif