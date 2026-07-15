#ifndef WEBSOCKET_STRTOUINT_H
#define WEBSOCKET_STRTOUINT_H

#if !defined(STRING_TO_INT_CONVERSION)
#define STRING_TO_INT_CONVERSION 0
#endif

#include "dll_export.h"
#include <stdint.h>
#include <stdbool.h>

#if STRING_TO_INT_CONVERSION
// DLL_EXPORT bool strtoint16(int16_t* result, const char* str);
DLL_EXPORT bool strtouint16(uint16_t* result, const char* str);
DLL_EXPORT bool strtoint32(int32_t* result, const char* str);
DLL_EXPORT bool strtouint32(uint32_t* result, const char* str);
#endif
#endif
