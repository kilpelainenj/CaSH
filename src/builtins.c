
#include "builtins.h"
#include "cd.h"
#include "dirsum.h"
#include "exit.h"
#include "export.h"
#include "pwd.h"
#include "unset.h"
#include <stdlib.h>

const builtin_t builtins[] = {{"exit", do_exit}, {"export", do_export}, {"unset", do_unset},
                              {"pwd", do_pwd},   {"cd", do_cd},         {"dirsum", do_dirsum},
                              {NULL, NULL}};