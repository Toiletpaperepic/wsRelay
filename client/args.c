#include "logger.h"
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define STRING_TO_INT_CONVERSION 1
#include "other.h"
#include "args.h"

void print_description(struct Argument* nextarg) {
    while (true) {
        print("    --%s - %s", nextarg->name, nextarg->description);
    
        if (nextarg->next == NULL) {
            break;
        } else {
            nextarg = nextarg->next;
        }
    }
}

void cleanup_args(struct Argument* nextarg) {
    while (true) {
        if (nextarg->value != NULL && nextarg->type != IS_BOOL && nextarg->type != IS_STRING) {
            free(nextarg->value);
        }
    
        if (nextarg->next == NULL) {
            break;
        } else {
            nextarg = nextarg->next;
        }
    }
}

bool parse_args(int argc, char *argv[], struct Argument* registerargs, bool ignore_unknowns) {
    for (int i = 1; i < argc; i++) {
        struct Argument* nextarg = registerargs;

        while (true) {
            char* dashdashname = malloc(3 + strlen(nextarg->name));
            memcpy(dashdashname, "--", sizeof("--")); // there isn't a null terminator in dashdashname so we can't use strcat() just yet.
            strcat(dashdashname, nextarg->name);

            bool condition_result = strcmp(dashdashname, argv[i]) == 0; 
            free(dashdashname);

            if (condition_result) {
                if (nextarg->type == IS_BOOL) {
                    nextarg->value = (void*)true;
                } else if (i + 1 != argc) /*if operand needed*/ {
                    switch (nextarg->type) {
                        case IS_UNSIGNED_INT:
                            nextarg->value = malloc(sizeof(unsigned int));
                            unsigned int x = 0;
                            if (strtouint32(&x, argv[i + 1])) {
                                error("strtouint32() Failed: invalid parameter.");
                                return true;
                            }
                            memcpy(nextarg->value, &x, sizeof(unsigned int));
                            i++;
                            break;
                        case IS_INT:
                            nextarg->value = malloc(sizeof(int));
                            int y = 0;
                            if (strtoint32(&y, argv[i + 1])) {
                                error("strtoint32() Failed: invalid parameter.");
                                return true;
                            }
                            memcpy(nextarg->value, &y, sizeof(int));
                            i++;
                            break;
                        case IS_STRING:
                            nextarg->value = argv[i + 1];
                            i++;
                            break;
                        default:
                            return true;
                    }
                } else {
                    error("Missing operand.\n");
                    return true;
                }

                break;
            }

            if (nextarg->next == NULL) {
                if (ignore_unknowns) {
                    break;
                } else {
                    error("Unknown command: %s", argv[i]);

                    return true;
                }
            } else {
                nextarg = nextarg->next;
            }
        }
    }

    struct Argument* nextarg = registerargs;
    while (true) {
        assert(nextarg->type == IS_BOOL ? nextarg->required != true : true); // required is NOT allowed if the type is a bool.
    
        if (nextarg->value == NULL && nextarg->required == true) {
            error("Argument --%s is required.", nextarg->name);
            return true;
        }
    
        if (nextarg->next == NULL) {
            break;
        } else {
            nextarg = nextarg->next;
        }
    }

    return false;
}