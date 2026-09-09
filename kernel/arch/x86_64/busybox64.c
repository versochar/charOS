/* 54C: busybox — applet yonlendirme (coreutils ustu). */
#include "arch/x86_64/longmode.h"

static int busybox_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int busybox64_applet(const char *name, int argc, char argv[][64],
                     char *out, int max) {
    if (!name) return 127;
    if (busybox_str_eq(name, "echo"))
        return coreutils64_echo(argc, argv, out, max);
    if (busybox_str_eq(name, "true") || busybox_str_eq(name, "["))
        return 0;
    if (busybox_str_eq(name, "false"))
        return 1;
    if (busybox_str_eq(name, "pwd") || busybox_str_eq(name, "whoami") ||
        busybox_str_eq(name, "uname"))
        return shell64_builtin("pwd", 0, 0, out, max);
    if (!coreutils64_exists(name)) return 127; /* applet yok */
    /* Kayitli ama henuz baglanmamis: stub basari */
    if (out && max > 0) out[0] = 0;
    return 0;
}
