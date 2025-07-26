#include <string.h>
#include "cpp-base64/base64.h"

// const char* base64_decode(const char* s);

extern "C" {
    void base64_encode_c(char* dist, const char* src) {
        strcpy(dist, base64_encode(std::string(src)).c_str());
    }
}