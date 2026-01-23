enum argType {
    IS_BOOL,
    IS_UNSIGNED_INT,
    IS_INT,
    IS_STRING,
};

struct Argument {
    const char* name;
    // required is NOT allowed if the type is a bool.
    bool required; 
    enum argType type;
    void* value;
    struct Argument* next;
};

#define register_argument(structname, ptrnext, dname, dtype, drequired) \
    struct Argument structname ;                                        \
    structname.name = dname;                                            \
    structname.required = drequired;                                    \
    structname.type = dtype;                                            \
    structname.value = NULL;                                            \
    structname.next = ptrnext;                                          \

void cleanup_args(struct Argument* nextarg);
bool parse_args(int argc, char *argv[], struct Argument* registerargs);