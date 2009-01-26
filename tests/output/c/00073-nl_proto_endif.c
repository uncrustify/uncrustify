
#include <stdio.h>

static void status_cb(status_t *status);
static int add_conn(const char *path);

#ifdef USE_FOO_CMD
static void foo_cmd(void *user, const info_t *info);
#endif

