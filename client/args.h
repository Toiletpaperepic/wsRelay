enum argType {
    IS_BOOL,
    IS_UNSIGNED_INT,
    IS_INT,
    IS_STRING,
};

struct Argument {
    const char* name;
    enum argType type;
    void* value;
    struct Argument* next;
};

#define register_argument(structname, ptrnext, dname, dtype) \
    struct Argument structname ; \
    structname.name = dname; \
    structname.type = dtype; \
    structname.value = NULL; \
    structname.next = ptrnext; 

bool parse_args(int argc, char *argv[], struct Argument* registerargs);