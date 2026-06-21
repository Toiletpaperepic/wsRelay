#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../client/args.h"
#define CHECK_CUSTOM_ERROR_HANDLE 1
#include "check.h"

const char* description = "This is a description!";

int main() {
    {
        char* argv[] = {"./a.out", /*"--test",*/ "--thisisabool", "--thisisastring", "Hello, World!", "--thisisaint", "-1", "--thisisaunsignedint", "1"}; 
        int argc = sizeof(argv) / sizeof(char*);
        int return_error = 0;

        register_argument(arg0, NULL, "test", IS_BOOL, false, description);
        register_argument(arg1, &arg0, "thisisabool", IS_BOOL, false, description);
        register_argument(arg2, &arg1, "thisisastring", IS_STRING, false, description);
        register_argument(arg3, &arg2, "thisisaint", IS_INT, false, description);
        register_argument(arg4, &arg3, "thisisaunsignedint", IS_UNSIGNED_INT, false, description);

        rcheck(parse_args(argc, argv, &arg4, false) != true, "parsing command line failed.", return_error = EXIT_FAILURE; goto error;);

        check((bool)arg0.value != true, return_error = EXIT_FAILURE; goto error;);
        info("%s", (bool)arg0.value ? "True" : "False");

        check((bool)arg1.value != false, return_error = EXIT_FAILURE; goto error;);
        info("%s", (bool)arg1.value ? "True" : "False");

        check(arg2.value != NULL, return_error = EXIT_FAILURE; goto error;);
        check(strcmp("Hello, World!", (char*)arg2.value) == 0, return_error = EXIT_FAILURE; goto error;);
        info("%s", (char*)arg2.value);
        
        check(arg3.value != NULL, return_error = EXIT_FAILURE; goto error;);
        check(-1 == *(int*)arg3.value, return_error = EXIT_FAILURE; goto error;);
        info("%i", *(int*)arg3.value);

        check(arg4.value != NULL, return_error = EXIT_FAILURE; goto error;);
        check(1 == *(unsigned int*)arg4.value, return_error = EXIT_FAILURE; goto error;);
        info("%i", *(int*)arg4.value);
        
error:
        cleanup_args(&arg4);

        if (return_error != 0) 
            return return_error;
    }

    // test if leaving out required arguments will error out.
    {
        char* argv[] = {"./a.out", /*"--test", "Hello, World!"*/}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_STRING, true, description);

        rcheck(parse_args(argc, argv, &arg0, false) == true, "parsing command line failed to fail without the \"--test\" argument present.", cleanup_args(&arg0); return EXIT_FAILURE;);

        cleanup_args(&arg0);
    }

    // {
    //     char* argv[] = {"./a.out", /*"--test"*/}; 
    //     int argc = sizeof(argv) / sizeof(char*);

    //     register_argument(arg0, NULL, "test", IS_BOOL, true, description);

    //     rcheck(parse_args(argc, argv, &arg0) == true, "parsing command line failed to fail when a bool is required.");
    // }
    
    {
        char* argv[] = {"./a.out", "--test", "-1"}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_UNSIGNED_INT, true, description);

        rcheck(parse_args(argc, argv, &arg0, false) == true, "parsing command line failed to fail when using unexpected negative numbers.", cleanup_args(&arg0); return EXIT_FAILURE;);

        cleanup_args(&arg0);
    }

    {
        char* argv[] = {"./a.out", "--test", "2147483648" /* INT_MAX + 1 */}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_INT, true, description);

        rcheck(parse_args(argc, argv, &arg0, false) == true, "parsing command line failed to fail when result is higher then INT_MAX.", cleanup_args(&arg0); return EXIT_FAILURE;);

        cleanup_args(&arg0);
    }

    {
        char* argv[] = {"./a.out", "--test", "4294967296" /* INT_MAX + 1 */}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "test", IS_UNSIGNED_INT, true, description);

        rcheck(parse_args(argc, argv, &arg0, false) == true, "parsing command line failed to fail when result is higher then UINT_MAX.", cleanup_args(&arg0); return EXIT_FAILURE;);

        cleanup_args(&arg0);
    }

    {
        char* argv[] = {"./a.out", "--thisisunknown"}; 
        int argc = sizeof(argv) / sizeof(char*);

        register_argument(arg0, NULL, "thisisknown", IS_BOOL, false, description);

        rcheck(parse_args(argc, argv, &arg0, true) == false, "parsing command line failed to ignore unknown arguments.", cleanup_args(&arg0); return EXIT_FAILURE;);

        cleanup_args(&arg0);
    }

    return EXIT_SUCCESS;
}