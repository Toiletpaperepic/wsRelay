#if !defined(STRING_TO_INT_CONVERSION)
#define STRING_TO_INT_CONVERSION 0
#endif

#if STRING_TO_INT_CONVERSION
// bool strtoint16(int16_t* result, const char* str);
bool strtouint16(uint16_t* result, const char* str);
bool strtoint32(int32_t* result, const char* str);
bool strtouint32(uint32_t* result, const char* str);
#endif