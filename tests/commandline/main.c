#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../client/args.h"
#include "check.h"

int main() {
    {
        char* argv[] = {"./a.out", /*"--test",*/ "--thisisabool", "--thisisastring", "Hello, World!", "--thisisaint", "-1", "--thisisaunsignedint", "1"}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_BOOL, false);
        register_argument(arg1, &arg0, "thisisabool", IS_BOOL, false);
        register_argument(arg2, &arg1, "thisisastring", IS_STRING, false);
        register_argument(arg3, &arg2, "thisisaint", IS_INT, false);
        register_argument(arg4, &arg3, "thisisaunsignedint", IS_UNSIGNED_INT, false);

        rcheck(parse_args(argc, argv, &arg4) != true, "parsing command line failed.");

        check((bool)arg0.value != true);
        printf("%s\n", (bool)arg0.value ? "True" : "False");

        check((bool)arg1.value != false);
        printf("%s\n", (bool)arg1.value ? "True" : "False");

        check(arg2.value != NULL);
        check(strcmp("Hello, World!", (char*)arg2.value) == 0);
        printf("%s\n", (char*)arg2.value);
        
        check(arg3.value != NULL);
        check(-1 == *(int*)arg3.value);
        printf("%i\n", *(int*)arg3.value);

        check(arg4.value != NULL);
        check(1 == *(unsigned int*)arg4.value);
        printf("%i\n", *(int*)arg4.value);

        struct Argument* nextarg = &arg4;
        while (true) {
            if (nextarg->value != NULL && nextarg->type != IS_BOOL) {
                printf("freed %s\n", nextarg->name);
                free(nextarg->value);
            } else {
                printf("not freed %s\n", nextarg->name);
            }
        
            if (nextarg->next == NULL) {
                break;
            } else {
                nextarg = nextarg->next;
            }
        }
    }

    // test if leaving out required arguments will error out.
    {
        char* argv[] = {"./a.out", /*"--test", "Hello, World!"*/}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_STRING, true);

        rcheck(parse_args(argc, argv, &arg0) == true, "parsing command line failed to failed without the \"--test\" argument present.");
        free(arg0.value);
    }

    // {
    //     char* argv[] = {"./a.out", /*"--test", "Hello, World!"*/}; 
    //     int argc = sizeof(argv) / sizeof(char*);

    //     register_argument(arg0, NULL, "test", IS_BOOL, true);

    //     rcheck(parse_args(argc, argv, &arg0) == true, "parsing command line failed to failed without the \"--test\" argument present.");
    // }
    
    return EXIT_SUCCESS;
}