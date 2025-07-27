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


struct parsed_url parse_url(const char* url);