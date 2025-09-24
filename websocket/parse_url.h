enum protocol_type {
    unknown,
    ws,
    wss,
};

struct parsed_url {
    enum protocol_type protocol;
    const char* address;
    unsigned int port;
    const char* path;
};

void free_purl(struct parsed_url purl);
struct parsed_url parse_url(const char* url);