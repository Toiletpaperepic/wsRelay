enum protocol_type {
    wss,
    ws,
    unknown,
};

struct parsed_url {
    enum protocol_type protocol;
    const char* address;
    int port;
    const char* path;
};


struct parsed_url parse_url(const char* url);