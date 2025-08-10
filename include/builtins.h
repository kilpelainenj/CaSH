#ifndef BUILTINS_H
#define BUILTINS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    int (*fn)(int argc, char **argv);
} builtin_t;

extern const builtin_t builtins[];  // defined in builtins.c

#ifdef __cplusplus
}
#endif
#endif