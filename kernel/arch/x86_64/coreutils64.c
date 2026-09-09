/* 54B: coreutils — 100+ komut kaydi + calisan alt-kume. */
#include "arch/x86_64/longmode.h"

static const char *coreutils64_names[] = {
    "arch", "b2sum", "base32", "base64", "basename", "basenc", "cat",
    "chcon", "chgrp", "chmod", "chown", "chroot", "cksum", "comm", "cp",
    "csplit", "cut", "date", "dd", "df", "dir", "dircolors", "dirname",
    "du", "echo", "env", "expand", "expr", "factor", "false", "fmt",
    "fold", "groups", "head", "hostid", "id", "install", "join", "kill",
    "link", "ln", "logname", "ls", "md5sum", "mkdir", "mkfifo", "mknod",
    "mktemp", "mv", "nice", "nl", "nohup", "nproc", "numfmt", "od",
    "paste", "pathchk", "pinky", "pr", "printenv", "printf", "ptx",
    "pwd", "readlink", "realpath", "rm", "rmdir", "runcon", "seq",
    "sha1sum", "sha224sum", "sha256sum", "sha384sum", "sha512sum",
    "shred", "shuf", "sleep", "sort", "split", "stat", "stdbuf",
    "stty", "sum", "sync", "tac", "tail", "tee", "test", "timeout",
    "touch", "tr", "true", "truncate", "tsort", "tty", "uname",
    "unexpand", "uniq", "unlink", "uptime", "users", "vdir", "wc",
    "who", "whoami", "yes", 0
};

static int coreutils_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int coreutils64_exists(const char *name) {
    int i;
    if (!name) return 0;
    for (i = 0; coreutils64_names[i]; i++) {
        if (coreutils_str_eq(coreutils64_names[i], name)) return 1;
    }
    return 0;
}

int coreutils64_list(int cat, char out[][32], int max) {
    /* cat: 0=tumu, 1=dosya, 2=metin, 3=sistem. Skeleton: 0 hepsi. */
    int i, n = 0;
    if (!out || max <= 0) return -1;
    if (cat < 0 || cat > 3) return -1;
    for (i = 0; coreutils64_names[i] && n < max; i++) {
        int j;
        for (j = 0; coreutils64_names[i][j] && j < 31; j++)
            out[n][j] = coreutils64_names[i][j];
        out[n][j] = 0;
        n++;
    }
    return n;
}

int coreutils64_echo(int argc, char argv[][64], char *out, int max) {
    return shell64_builtin("echo", argc, argv, out, max);
}

int coreutils64_wc(const char *text, u64 *lines, u64 *words, u64 *bytes) {
    u64 l = 0, w = 0, b = 0;
    int inword = 0;
    if (!text) return -1;
    while (*text) {
        char c = *text++;
        b++;
        if (c == '\n') l++;
        if (c == ' ' || c == '\t' || c == '\n') {
            inword = 0;
        } else if (!inword) {
            inword = 1;
            w++;
        }
    }
    if (lines) *lines = l;
    if (words) *words = w;
    if (bytes) *bytes = b;
    return 0;
}
