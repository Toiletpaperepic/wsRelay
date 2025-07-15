#include "parse_url.h"

int websocket_connect(struct parsed_url* purl);
void make_http_header(struct parsed_url* url, char* message);