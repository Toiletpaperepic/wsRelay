#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "args.h"

int32_t strtoint(const char* str) {
    int32_t num = 0;

    if (str[0] == '-'){
        for (int i = 1; str[i] != '\0'; i++) {
            if (str[i] >= 48 && str[i] <= 57) {
                num = num * 10 + (str[i] - 48);
            }
            else {
                return INT32_MAX;
            }
        }
        num = -num;
    } else {
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] >= 48 && str[i] <= 57) {
                num = num * 10 + (str[i] - 48);
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
            num = num * 10 + (str[i] - 48);
        }
        else {
            return UINT32_MAX;
        }
    }

    return num;
}

void help() {
    // todo: ...
}

bool parse_args(int argc, char *argv[], struct Argument* registerargs) {
    for (int i = 1; i < argc; i++) {
        struct Argument* nextarg = registerargs;

        while (true) {
            char dashdashname[3 + strlen(nextarg->name)] = {};
            memcpy(dashdashname, "--", sizeof("--"));
            strcat(dashdashname, nextarg->name);

            printf("%s == %s\n", dashdashname, argv[i]);
            if (strcmp(dashdashname, argv[i]) == 0) {
                if (nextarg->type == IS_BOOL) {
                    nextarg->value = (void*)true;
                } else if (i + 1 != argc) /*if operand needed*/ {
                    switch (nextarg->type) {
                        case IS_UNSIGNED_INT:
                            nextarg->value = malloc(sizeof(unsigned int));
                            unsigned int x = strtouint(argv[i + 1]);
                            if (x == UINT32_MAX) {
                                fprintf(stderr, "strtouint() Failed: invalid parameter.\n");
                                return true;
                            }
                            memcpy(nextarg->value, &x, sizeof(unsigned int));
                            i++;
                            break;
                        case IS_INT:
                            nextarg->value = malloc(sizeof(int));
                            int y = strtoint(argv[i + 1]);
                            if (y == INT32_MAX) {
                                fprintf(stderr, "strtoint() Failed: invalid parameter.\n");
                                return true;
                            }
                            memcpy(nextarg->value, &y, sizeof(int));
                            i++;
                            break;
                        case IS_STRING:
                            nextarg->value = strdup(argv[i + 1]);
                            i++;
                            break;
                        default:
                            return true;
                    }
                } else {
                    printf("Missing operand.\n");
                    help();
                    return true;
                }

                break;
            }
            
            if (nextarg->next == NULL) {
                printf("Unknown command: %s\n", argv[i]);

                return true;
            } else {
                nextarg = nextarg->next;
            }
        }
    }

    struct Argument* nextarg = registerargs;
    while (true) {
        assert(nextarg->type == IS_BOOL ? nextarg->required != true : true); // required is NOT allowed if the type is a bool.
    
        if (nextarg->value == NULL && nextarg->required == true) {
            printf("Argument --%s is required.\n", nextarg->name);
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