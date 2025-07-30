#include <string.h>
#include "cpp-base64/base64.h"

// const char* base64_decode(const char* s);

extern "C" {
    const char* base64_encode_c(const char* src) {
        return strdup(base64_encode(std::string(src)).c_str());
    }
}