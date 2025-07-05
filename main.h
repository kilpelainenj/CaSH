

typedef int (*cmdfunc)(int, char**);

typedef struct {
    const char *name;
    cmdfunc     fn;
} builtin_t;
