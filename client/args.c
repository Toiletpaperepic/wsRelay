#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "args.h"

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
                            unsigned int x = strtoul(argv[i + 1], NULL, 0);
                            memcpy(nextarg->value, &x, sizeof(unsigned int));
                            i++;
                            break;
                        case IS_INT:
                            nextarg->value = malloc(sizeof(int));
                            int y = strtol(argv[i + 1], NULL, 0);
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

    return false;
}