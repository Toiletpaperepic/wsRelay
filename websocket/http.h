#include "dll_export.h"

DLL_EXPORT void make_user_agent(char** destinationstring);
const char* make_http_header(struct parsed_url purl);