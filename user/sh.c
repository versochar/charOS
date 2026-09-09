typedef unsigned int uint32_t;
#define SYS_WRITE 1
#define SYS_READ 2
#define SYS_CLOSE 11
#define SYS_FORK 12
#define SYS_EXEC 15
#define SYS_WAITPID 19
#define SYS_PIPE 13
#define SYS_DUP2 147
#define SYS_OPEN 10
#define SYS_KILL 17
#define SYS_LSDIR 156
#define SYS_VTSWITCH 157
#define SYS_TASKLIST 158
#define SYS_DFMKDIR 150
#define SYS_DFREMOVE 151
#define SYS_DFTRUNCATE 152
#define SYS_DFSTAT 153
#define SYS_DFLIST 154
#define SYS_DFSAVE 133
#define SYS_DFLOAD 134
#define SYS_NANOSLEEP 16
#define SYS_EXIT 0
#define O_RDONLY 1
#define O_WRONLY 2
#define O_CREATE 4
#define O_TRUNC 8
#define O_APPEND 16
#define WNOHANG 1
static inline uint32_t syscall(uint32_t n, uint32_t a, uint32_t b, uint32_t c){ uint32_t r; asm volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c):"memory"); return r; }
void puts(const char*s){ uint32_t l=0; while(s[l]) l++; syscall(SYS_WRITE,1,(uint32_t)s,l); }
int strcmp(const char*a,const char*b){ while(*a&&*a==*b){a++;b++;} return *a-*b; }
static int sh_atoi(const char*s){ int v=0; int i=0; while(s[i]>='0'&&s[i]<='9'&&i<9){ v=v*10+(s[i]-'0'); i++; } return v; }
static void sh_putdec(uint32_t v){ char b[12]; int i=0; if(!v) b[i++]='0'; while(v){ b[i++]='0'+(v%10); v/=10; } while(i) syscall(SYS_WRITE,1,(uint32_t)&b[--i],1); }

/* ---- 19I: ortam değişkenleri (shell-içi tablo, exec'e miras yok) ---- */
#define SH_ENVMAX 8
static char enames[SH_ENVMAX][32];
static char evals[SH_ENVMAX][64];
static int env_n;
static const char* env_get(const char* nm) {
    for (int i = 0; i < env_n; i++) {
        const char* e = enames[i];
        int k = 0;
        while (e[k] && nm[k] && e[k] == nm[k]) k++;
        if (e[k] == 0 && nm[k] == 0) return evals[i];
    }
    return 0;
}
static int env_set(const char* nm, const char* val) {
    int nl = 0; while (nm[nl]) nl++;
    int vl = 0; while (val[vl]) vl++;
    if (!nl || nl > 31 || vl > 63) return -1;
    if (!((nm[0] >= 'A' && nm[0] <= 'Z') || (nm[0] >= 'a' && nm[0] <= 'z') || nm[0] == '_')) return -1;
    for (int i = 0; nm[i]; i++) {
        char c = nm[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return -1;
    }
    for (int i = 0; i < env_n; i++) {
        const char* e = enames[i];
        int k = 0;
        while (e[k] && nm[k] && e[k] == nm[k]) k++;
        if (e[k] == 0 && nm[k] == 0) {
            for (int j = 0; j <= vl; j++) evals[i][j] = val[j];
            return 0;
        }
    }
    if (env_n >= SH_ENVMAX) return -1;
    for (int j = 0; j <= nl; j++) enames[env_n][j] = nm[j];
    for (int j = 0; j <= vl; j++) evals[env_n][j] = val[j];
    env_n++;
    return 0;
}
/* 19I/19H: $NAME ve $? genişletme (tırnak yok; değerlerde boşluk olamaz). */
static int last_status;
static void expand_vars(const char* in, char* out, int outmax) {
    int p = 0;
    for (int i = 0; in[i] && p < outmax - 1; i++) {
        if (in[i] == '$') {
            if (in[i+1] == '?') {
                char nb[12]; int nn = 0;
                unsigned v = (unsigned)last_status;
                if (!v) nb[nn++] = '0';
                while (v && nn < 11) { nb[nn++] = '0' + (v % 10); v /= 10; }
                for (int k = nn - 1; k >= 0 && p < outmax - 1; k--) out[p++] = nb[k];
                i++;
            } else if ((in[i+1] >= 'A' && in[i+1] <= 'Z') || (in[i+1] >= 'a' && in[i+1] <= 'z') || in[i+1] == '_') {
                char nm[32]; int nl = 0;
                i++;
                while (((in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= 'a' && in[i] <= 'z') ||
                        (in[i] >= '0' && in[i] <= '9') || in[i] == '_') && nl < 31) nm[nl++] = in[i++];
                nm[nl] = 0; i--;
                const char* v = env_get(nm);
                if (v) while (*v && p < outmax - 1) out[p++] = *v++;
            } else out[p++] = in[i];
        } else out[p++] = in[i];
    }
    out[p] = 0;
}

/* ---- 19G: background iş listesi (pid başına kayıt) ---- */
#define SH_JOBMAX 8
static uint32_t job_pids[SH_JOBMAX];
static int job_n;
static int job_add(uint32_t pid) {
    if (job_n >= SH_JOBMAX) { puts("jobs full\n"); return -1; }
    job_pids[job_n++] = pid;
    return 0;
}
static int job_take(int idx) { /* listeden çıkar, pid dön */
    if (idx < 0 || idx >= job_n) return -1;
    int pid = (int)job_pids[idx];
    for (int i = idx; i + 1 < job_n; i++) job_pids[i] = job_pids[i + 1];
    job_n--;
    return pid;
}
/* %n -> job index, sade sayı -> pid. -1 hata (0 da geçersiz). */
static int job_resolve(const char* s) {
    if (!s || !s[0]) return -1;
    if (s[0] == '%') {
        int idx = sh_atoi(s + 1) - 1;
        if (idx < 0 || idx >= job_n) return -1;
        return (int)job_pids[idx];
    }
    int pid = sh_atoi(s);
    if (pid <= 0) return -1;
    return pid;
}
static int job_index_of(int pid) {
    for (int i = 0; i < job_n; i++) if ((int)job_pids[i] == pid) return i;
    return -1;
}

/* ---- 19J: geçmiş (derinlik 8) + TAB tamamlama ---- */
#define SH_HISTMAX 8
static char hist[SH_HISTMAX][128];
static int hist_n;
static int hist_cur;
static void hist_push(const char* line) {
    int i = 0;
    while (line[i]) i++;
    if (!i) return;
    if (hist_n >= SH_HISTMAX) {
        for (int h = 1; h < SH_HISTMAX; h++)
            for (int k = 0; k < 128; k++) hist[h - 1][k] = hist[h][k];
        hist_n = SH_HISTMAX - 1;
    }
    for (int k = 0; k < 128; k++) { hist[hist_n][k] = line[k]; if (!line[k]) break; }
    hist_n++;
    hist_cur = hist_n;
}
static char lsbuf[1024];
static char sh_cwd[128] = "/";
static const char* builtins[] = { "echo", "help", "exit", "jobs", "wait", "kill", "fg", "export", "sleep", "desktop", "console", "ls", "pwd", "cd", "mkdir", "rmdir", "touch", "rm", "cp", "mv", "clear", "ps", "uname", 0 };
/* 19J: öneke uyan adayları topla (builtin + LSDIR). Dönüş: aday sayısı. */
static int tab_collect(const char* word, char out[][64], int maxc) {
    int nc = 0;
    int wl = 0;
    while (word[wl]) wl++;
    for (int b = 0; builtins[b] && nc < maxc; b++) {
        int k = 0;
        while (k < wl && builtins[b][k] == word[k]) k++;
        if (k == wl) {
            int dup = 0;
            for (int j = 0; j < nc; j++) if (!strcmp(out[j], builtins[b])) dup = 1;
            if (!dup) {
                int k2 = 0;
                while (builtins[b][k2] && k2 < 63) { out[nc][k2] = builtins[b][k2]; k2++; }
                out[nc][k2] = 0; nc++;
            }
        }
    }
    int lr = syscall(SYS_LSDIR, (uint32_t)"/", (uint32_t)lsbuf, sizeof(lsbuf));
    if (lr > 0) {
        int p = 0;
        while (p < lr && nc < maxc) {
            int e = p;
            while (e < lr && lsbuf[e] != '\n') e++;
            int el = e - p;
            if (el > 0 && el < 64) {
                int k = 0;
                while (k < wl && k < el && lsbuf[p + k] == word[k]) k++;
                if (k == wl) {
                    int dup = 0;
                    for (int j = 0; j < nc && !dup; j++) {
                        int m = 0;
                        while (out[j][m] && m < el && out[j][m] == lsbuf[p + m]) m++;
                        if (out[j][m] == 0 && m == el) dup = 1;
                    }
                    if (!dup) {
                        for (int k2 = 0; k2 < el; k2++) out[nc][k2] = lsbuf[p + k2];
                        out[nc][el] = 0; nc++;
                    }
                }
            }
            p = e + 1;
        }
    }
    return nc;
}

/* 19B: satırı yerinde token'lara böl. Dönüş: argc (argv[max] sınırında keser). */
static int tokenize(char* line, char* argv[], int max) {
    int n = 0;
    int i = 0;
    while (line[i] && n < max) {
        while (line[i] == ' ') i++;
        if (!line[i]) break;
        argv[n++] = &line[i];
        while (line[i] && line[i] != ' ') i++;
        if (line[i]) { line[i] = 0; i++; }
    }
    return n;
}

/* 19B: program yolu çöz — '/' içeriyorsa aynen, yoksa /bin/ altına bak. */
static void resolve_prog(const char* cmd, char* out) {
    int has_slash = 0;
    for (int i = 0; cmd[i]; i++) if (cmd[i] == '/') has_slash = 1;
    if (has_slash) {
        int i = 0;
        while (cmd[i] && i < 127) { out[i] = cmd[i]; i++; }
        out[i] = 0;
    } else {
        out[0] = '/'; out[1] = 'b'; out[2] = 'i'; out[3] = 'n'; out[4] = '/';
        int i = 0;
        while (cmd[i] && i < 122) { out[5 + i] = cmd[i]; i++; }
        out[5 + i] = 0;
    }
}

/* 19B: argv[1..]'yi tek boşlukla birleştir (exec args dizesi). */
static void join_args(char* argv[], int argc, char* out) {
    int p = 0;
    for (int i = 1; i < argc && p < 120; i++) {
        if (i > 1 && p < 120) out[p++] = ' ';
        for (int k = 0; argv[i][k] && p < 120; k++) out[p++] = argv[i][k];
    }
    out[p] = 0;
}

/* 19C: satırı '|' segmentlerine böl (yerinde NUL'la). Dönüş: segment sayısı. */
#define SH_MAXCMDS 4
#define SH_MAXARGS 16
static int split_pipe(char* line, char* cmds[]) {
    int n = 0;
    int i = 0;
    while (line[i] && n < SH_MAXCMDS) {
        while (line[i] == ' ') i++;
        if (!line[i]) break;
        cmds[n++] = &line[i];
        while (line[i] && line[i] != '|') i++;
        if (line[i] == '|') { line[i] = 0; i++; }
    }
    return n;
}

/* 19E: segmentten </>/>> yönlendirmesini ayıkla (birer tane). Segment yerinde
 * kesilir; dosya adları kopyalanır. append=1 ise >>. */
static void parse_redir(char* seg, char* infile, char* outfile, int* append) {
    infile[0] = 0; outfile[0] = 0; *append = 0;
    for (int i = 0; seg[i]; i++) {
        if (seg[i] == '<') {
            seg[i] = 0;
            int j = i + 1;
            while (seg[j] == ' ') j++;
            int k = 0;
            while (seg[j] && seg[j] != ' ' && seg[j] != '>' && seg[j] != '<' && k < 63)
                infile[k++] = seg[j++];
            infile[k] = 0;
            break;
        }
    }
    for (int i = 0; seg[i]; i++) {
        if (seg[i] == '>') {
            int ap = 0;
            if (seg[i + 1] == '>') ap = 1;
            seg[i] = 0;
            int j = ap ? i + 2 : i + 1;
            while (seg[j] == ' ') j++;
            int k = 0;
            while (seg[j] && seg[j] != ' ' && seg[j] != '>' && seg[j] != '<' && k < 63)
                outfile[k++] = seg[j++];
            outfile[k] = 0;
            *append = ap;
            break;
        }
    }
    {
        int e = 0;
        while (seg[e]) e++;
        while (e > 0 && seg[e - 1] == ' ') { seg[e - 1] = 0; e--; }
    }
}

/* 19E: dosya yönlendirmelerini uygula (pipe dup'larından SONRA). Dönüş 0/-1. */
static int apply_redir(const char* infile, const char* outfile, int append) {
    if (infile[0]) {
        int f = (int)syscall(SYS_OPEN, (uint32_t)infile, O_RDONLY, 0);
        if (f < 0) { puts("open fail: "); puts(infile); puts("\n"); return -1; }
        syscall(SYS_DUP2, (uint32_t)f, 0, 0);
        syscall(SYS_CLOSE, (uint32_t)f, 0, 0);
    }
    if (outfile[0]) {
        int mode = O_WRONLY | O_CREATE | (append ? O_APPEND : O_TRUNC);
        int f = (int)syscall(SYS_OPEN, (uint32_t)outfile, (uint32_t)mode, 0);
        if (f < 0) { puts("open fail: "); puts(outfile); puts("\n"); return -1; }
        syscall(SYS_DUP2, (uint32_t)f, 1, 0);
        syscall(SYS_CLOSE, (uint32_t)f, 0, 0);
    }
    return 0;
}

/* ---- 19F: builtin'ler ---- */
/* Dönüş: >=0 durum (işlendi), -1 değil, -2 yalnız-shell (child exec'e düşer).
 * NOT: yönlendirme çağıranda uygulanır (tek-komutta fd yedekli, child'da
 * pipe sonrası); burası sadece işi yapar. */
static int try_builtin(char* argv[], int argc, int in_child) {
    if (argc <= 0) return 0;
    if (!strcmp(argv[0], "echo")) {
        for (int i = 1; i < argc; i++) {
            if (i > 1) puts(" ");
            puts(argv[i]);
        }
        puts("\n");
        return 0;
    }
    if (!strcmp(argv[0], "help")) {
            puts("charOS sh builtins:\n");
            puts("  echo args.. / help / exit [code] / sleep ticks\n");
            puts("  jobs / wait [%n] / kill %n|pid / fg %n\n");
            puts("  export NAME=val / export / desktop / console\n");
            puts("  ls [path] / pwd / cd path / mkdir path / rmdir path\n");
            puts("  touch path / rm path / cp src dst / mv src dst\n");
            puts("  clear / ps / uname\n");
        puts("ops: | > >> < ; && || & $VAR $? history(TAB/arrows)\n");
        return 0;
    }
    /* ---- 20A: FS komutları ---- */
    if (!strcmp(argv[0], "clear")) {
        puts("\x1B[2J\x1B[H");
        return 0;
    }
    if (!strcmp(argv[0], "pwd")) {
        puts(sh_cwd);
        puts("\n");
        return 0;
    }
    if (!strcmp(argv[0], "cd")) {
        if (argc < 2) { puts("usage: cd path\n"); return 1; }
        int L = 0;
        while (argv[1][L] && L < 127) L++;
        if (L == 0) return 1;
        /* varlık + dizin kontrolü: is_dir üst bit (0x80000000), uint32_t olarak */
        uint32_t cdst = syscall(SYS_DFSTAT, (uint32_t)argv[1], 0, 0);
        if (cdst == (uint32_t)-1 || !(cdst & 0x80000000u)) {
            puts("cd: not a dir: "); puts(argv[1]); puts("\n"); return 1;
        }
        for (int i = 0; i <= L; i++) sh_cwd[i] = argv[1][i];
        return 0;
    }
    if (!strcmp(argv[0], "ls")) {
        static char lbuf[1024];
        const char* p = (argc > 1) ? argv[1] : "/";
        int r = (int)syscall(SYS_DFLIST, (uint32_t)p, (uint32_t)lbuf, sizeof(lbuf));
        if (r > 0) {
            puts(lbuf);
        } else {
            r = (int)syscall(SYS_LSDIR, (uint32_t)p, (uint32_t)lbuf, sizeof(lbuf));
            if (r > 0) {
                /* SYS_LSDIR "ad\n"/"ad/\n" formatını olduğu gibi bas */
                int i = 0;
                while (i < r) { char ch = lbuf[i]; syscall(SYS_WRITE, 1, (uint32_t)&ch, 1); i++; }
            } else {
                puts("ls: empty\n");
            }
        }
        return 0;
    }
    if (!strcmp(argv[0], "mkdir")) {
        if (argc < 2) { puts("usage: mkdir path\n"); return 1; }
        if ((int)syscall(SYS_DFMKDIR, (uint32_t)argv[1], 0, 0) != 0)
            { puts("mkdir fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "rmdir")) {
        if (argc < 2) { puts("usage: rmdir path\n"); return 1; }
        if ((int)syscall(SYS_DFREMOVE, (uint32_t)argv[1], 0, 0) != 0)
            { puts("rmdir fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "touch")) {
        if (argc < 2) { puts("usage: touch path\n"); return 1; }
        /* boş dosya yaz (yoksa oluşturur) */
        if ((int)syscall(SYS_DFSAVE, (uint32_t)argv[1], 0, 0) != 0)
            { puts("touch fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "rm")) {
        if (argc < 2) { puts("usage: rm path\n"); return 1; }
        if ((int)syscall(SYS_DFREMOVE, (uint32_t)argv[1], 0, 0) != 0)
            { puts("rm fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "cp")) {
        if (argc < 3) { puts("usage: cp src dst\n"); return 1; }
        static char dbuf[4096];
        /* diskfs okuma (e) -> yazma */
        int n = (int)syscall(SYS_DFLOAD, (uint32_t)argv[1], (uint32_t)dbuf, sizeof(dbuf));
        if (n < 0) { puts("cp: read fail\n"); return 1; }
        if ((int)syscall(SYS_DFSAVE, (uint32_t)argv[2], (uint32_t)dbuf, (uint32_t)n) != 0)
            { puts("cp: write fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "mv")) {
        if (argc < 3) { puts("usage: mv src dst\n"); return 1; }
        static char dbuf[4096];
        int n = (int)syscall(SYS_DFLOAD, (uint32_t)argv[1], (uint32_t)dbuf, sizeof(dbuf));
        if (n < 0) { puts("mv: read fail\n"); return 1; }
        if ((int)syscall(SYS_DFSAVE, (uint32_t)argv[2], (uint32_t)dbuf, (uint32_t)n) != 0)
            { puts("mv: write fail\n"); return 1; }
        if ((int)syscall(SYS_DFREMOVE, (uint32_t)argv[1], 0, 0) != 0)
            { puts("mv: rm src fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "ps")) {
        static char pbuf[2048];
        int n = (int)syscall(SYS_TASKLIST, (uint32_t)pbuf, sizeof(pbuf), 0);
        if (n > 0) {
            int i = 0;
            while (i < n) { char ch = pbuf[i]; syscall(SYS_WRITE, 1, (uint32_t)&ch, 1); i++; }
        } else { puts("ps: empty\n"); }
        return 0;
    }
    if (!strcmp(argv[0], "uname")) {
        puts("charOS 0.19 kernel on i386\n");
        return 0;
    }
    if (!strcmp(argv[0], "exit")) {
        int code = (argc > 1) ? sh_atoi(argv[1]) : 0;
        syscall(SYS_EXIT, (uint32_t)code, 0, 0);
        for (;;) { }
    }
    if (in_child) {
        /* durum değiştirenler yalnız-shell'dir; child exec'e düşer */
        if (!strcmp(argv[0], "jobs") || !strcmp(argv[0], "wait") ||
            !strcmp(argv[0], "kill") || !strcmp(argv[0], "fg") ||
            !strcmp(argv[0], "export"))
            return -2;
        return -1;
    }
    if (!strcmp(argv[0], "jobs")) {
        if (job_n == 0) puts("no jobs\n");
        for (int i = 0; i < job_n; ) {
            int st = 0;
            int r = (int)syscall(SYS_WAITPID, job_pids[i], (uint32_t)&st, WNOHANG);
            if (r > 0) {
                puts("["); sh_putdec((uint32_t)(i + 1)); puts("] ");
                sh_putdec(job_pids[i]); puts(" done\n");
                job_take(i);
            } else if (r == 0) {
                puts("["); sh_putdec((uint32_t)(i + 1)); puts("] ");
                sh_putdec(job_pids[i]); puts(" running\n");
                i++;
            } else job_take(i); /* artık child değil: sessizce düşür */
        }
        return 0;
    }
    if (!strcmp(argv[0], "wait")) {
        if (argc < 2) {
            while (job_n > 0) {
                int st = 0;
                syscall(SYS_WAITPID, job_pids[0], (uint32_t)&st, 0);
                job_take(0);
            }
            return 0;
        }
        int pid = job_resolve(argv[1]);
        if (pid < 0) { puts("wait: no such job\n"); return 1; }
        int st = 0;
        int r = (int)syscall(SYS_WAITPID, (uint32_t)pid, (uint32_t)&st, 0);
        int idx = job_index_of(pid);
        if (idx >= 0) job_take(idx);
        return (r > 0) ? 0 : 1;
    }
    if (!strcmp(argv[0], "kill")) {
        if (argc < 2) { puts("usage: kill %n|pid\n"); return 1; }
        int pid = job_resolve(argv[1]);
        if (pid < 0) { puts("kill: no such job\n"); return 1; }
        if (syscall(SYS_KILL, (uint32_t)pid, 1, 0) != 0) { puts("kill: failed\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "fg")) {
        if (argc < 2) { puts("usage: fg %n\n"); return 1; }
        int pid = job_resolve(argv[1]);
        if (pid < 0) { puts("fg: no such job\n"); return 1; }
        int st = 0;
        int r = (int)syscall(SYS_WAITPID, (uint32_t)pid, (uint32_t)&st, 0);
        int idx = job_index_of(pid);
        if (idx >= 0) job_take(idx);
        return (r > 0) ? 0 : 1;
    }
    if (!strcmp(argv[0], "export")) {
        if (argc < 2) {
            for (int i = 0; i < env_n; i++) {
                puts(enames[i]); puts("="); puts(evals[i]); puts("\n");
            }
            return 0;
        }
        int e = 0;
        while (argv[1][e] && argv[1][e] != '=') e++;
        if (!argv[1][e]) { puts("usage: export NAME=val\n"); return 1; }
        argv[1][e] = 0;
        if (env_set(argv[1], argv[1] + e + 1) != 0) { puts("export: failed\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "sleep")) {
        /* 19G: kill testinde hedefe zaman kazandırır (tick) */
        if (argc < 2) { puts("usage: sleep ticks\n"); return 1; }
        syscall(SYS_NANOSLEEP, (uint32_t)sh_atoi(argv[1]), 0, 0);
        return 0;
    }
    if (!strcmp(argv[0], "desktop")) {
        /* 19K: GUI masaüstünü aç (VT1) */
        if (syscall(SYS_VTSWITCH, 1, 0, 0) != 0) { puts("desktop: vt fail\n"); return 1; }
        return 0;
    }
    if (!strcmp(argv[0], "console")) {
        /* 19K: metin konsoluna dön (VT2) */
        if (syscall(SYS_VTSWITCH, 2, 0, 0) != 0) { puts("console: vt fail\n"); return 1; }
        return 0;
    }
    return -1;
}

/* 19C/19D/19E: genel pipeline çalıştırıcı (tek komut dahil). Dönüş: durum. */
static int run_pipeline(char* cmds[], int ncmds, int bg) {
    char xseg[128];
    char* argvs[SH_MAXCMDS][SH_MAXARGS];
    int argcs[SH_MAXCMDS];
    char progs[SH_MAXCMDS][128];
    char args[SH_MAXCMDS][128];
    char infiles[SH_MAXCMDS][64];
    char outfiles[SH_MAXCMDS][64];
    int appends[SH_MAXCMDS];
    for (int i = 0; i < ncmds; i++) {
        expand_vars(cmds[i], xseg, sizeof(xseg));
        int k = 0;
        while (xseg[k] && k < 127) { cmds[i][k] = xseg[k]; k++; }
        cmds[i][k] = 0;
        parse_redir(cmds[i], infiles[i], outfiles[i], &appends[i]);
        argcs[i] = tokenize(cmds[i], argvs[i], SH_MAXARGS);
        if (argcs[i] == 0) return 0; /* boş segment: hiçbir şey çalıştırma */
        if (ncmds == 1) {
            /* 19F: tek komut builtin'leri fd yedeğiyle in-shell çalışır */
            syscall(SYS_DUP2, 0, 14, 0);
            syscall(SYS_DUP2, 1, 15, 0);
            int rr = apply_redir(infiles[i], outfiles[i], appends[i]);
            int b = -1;
            if (rr == 0) b = try_builtin(argvs[i], argcs[i], 0);
            else b = 1;
            syscall(SYS_DUP2, 14, 0, 0);
            syscall(SYS_DUP2, 15, 1, 0);
            syscall(SYS_CLOSE, 14, 0, 0);
            syscall(SYS_CLOSE, 15, 0, 0);
            if (b != -1 && b != -2) {
                last_status = b;
                if (bg) puts(" & bg\n");
                return b;
            }
        }
        resolve_prog(argvs[i][0], progs[i]);
        join_args(argvs[i], argcs[i], args[i]);
    }
    int prev = -1;
    uint32_t pids[SH_MAXCMDS];
    int started = 0;
    int lastst = 0;
    for (int i = 0; i < ncmds; i++) {
        int fds[2] = { -1, -1 };
        int has_next = (i + 1 < ncmds);
        if (has_next) {
            if (syscall(SYS_PIPE, (uint32_t)fds, 0, 0) != 0) {
                if (prev >= 0) syscall(SYS_CLOSE, (uint32_t)prev, 0, 0);
                break;
            }
        }
        uint32_t c = syscall(SYS_FORK, 0, 0, 0);
        if ((int)c < 0) {
            if (prev >= 0) { syscall(SYS_CLOSE, (uint32_t)prev, 0, 0); prev = -1; }
            if (has_next) { syscall(SYS_CLOSE, (uint32_t)fds[0], 0, 0); syscall(SYS_CLOSE, (uint32_t)fds[1], 0, 0); }
            break;
        }
        if (c == 0) {
            if (prev >= 0) {
                syscall(SYS_DUP2, (uint32_t)prev, 0, 0);
                syscall(SYS_CLOSE, (uint32_t)prev, 0, 0);
            }
            if (has_next) {
                syscall(SYS_CLOSE, (uint32_t)fds[0], 0, 0);
                syscall(SYS_DUP2, (uint32_t)fds[1], 1, 0);
                syscall(SYS_CLOSE, (uint32_t)fds[1], 0, 0);
            }
            if (apply_redir(infiles[i], outfiles[i], appends[i]) != 0)
                syscall(SYS_EXIT, 1, 0, 0);
            int b = try_builtin(argvs[i], argcs[i], 1);
            if (b < 0) {
                if (b == -1) {
                    syscall(SYS_EXEC, (uint32_t)progs[i], (uint32_t)args[i], 0);
                    puts("exec fail: "); puts(progs[i]); puts("\n");
                }
                syscall(SYS_EXIT, 1, 0, 0);
            }
            syscall(SYS_EXIT, (uint32_t)b, 0, 0);
        } else {
            pids[i] = c;
            started = i + 1;
            if (bg) job_add(c);
            if (prev >= 0) { syscall(SYS_CLOSE, (uint32_t)prev, 0, 0); prev = -1; }
            if (has_next) {
                syscall(SYS_CLOSE, (uint32_t)fds[1], 0, 0);
                prev = fds[0];
            }
        }
    }
    if (prev >= 0) syscall(SYS_CLOSE, (uint32_t)prev, 0, 0);
    if (!bg) {
        for (int i = 0; i < started; i++) {
            int st = 0;
            syscall(SYS_WAITPID, pids[i], (uint32_t)&st, 0);
            lastst = st;
        }
    } else puts(" & bg\n");
    last_status = bg ? 0 : lastst;
    return last_status;
}

/* 19F: ';' ile sıralama. 19H: '&&' / '||' zinciri (soldan, kısa devre). */
static int run_chain(char* seg, int bg) {
    /* && ve ||'ya böl (operatörleri sakla) */
    char* parts[8];
    int ops[8]; /* 0=son, 1=&&, 2=|| */
    int np = 0;
    int i = 0;
    while (seg[i] && np < 8) {
        while (seg[i] == ' ') i++;
        if (!seg[i]) break;
        parts[np] = &seg[i];
        while (seg[i] && !(seg[i] == '&' && seg[i+1] == '&') &&
               !(seg[i] == '|' && seg[i+1] == '|')) i++;
        if (!seg[i]) ops[np++] = 0;
        else if (seg[i] == '&') { seg[i] = 0; seg[i+1] = 0; ops[np++] = 1; i += 2; }
        else { seg[i] = 0; seg[i+1] = 0; ops[np++] = 2; i += 2; }
    }
    int st = 0;
    for (int p = 0; p < np; p++) {
        if (p > 0) {
            if (ops[p-1] == 1 && st != 0) continue; /* &&: başarısızsa atla */
            if (ops[p-1] == 2 && st == 0) continue; /* ||: başarılıysa atla */
        }
        char* cmds[SH_MAXCMDS];
        int ncmds = split_pipe(parts[p], cmds);
        if (ncmds == 0) { st = 0; continue; }
        st = run_pipeline(cmds, ncmds, bg);
    }
    return st;
}

/* 19J: satır içi düzenleme — geçmiş (ESC[A/B) + TAB tamamlama */
static void hist_show(int n) {
    for (int i = 0; i < n; i++) puts("\b \b");
}
static int tab_complete(char* line, int* np) {
    int n = *np;
    int t = n;
    while (t > 0 && line[t-1] != ' ') t--;
    char word[64];
    int wl = 0;
    while (t + wl < n && wl < 63) { word[wl] = line[t + wl]; wl++; }
    word[wl] = 0;
    if (!wl) return 0;
    /* yol ise basename kısmını tamamla */
    char* base = word;
    int bl = 0;
    while (word[bl]) { if (word[bl] == '/') base = word + bl + 1; bl++; }
    int pl = 0;
    while (base[pl]) pl++;
    char cands[8][64];
    int nc = tab_collect(base, cands, 8);
    if (nc == 0) return 0;
    if (nc == 1) {
        int cl = 0;
        while (cands[0][cl]) cl++;
        for (int k = pl; k < cl && n < 127; k++) {
            line[n++] = cands[0][k];
            syscall(SYS_WRITE, 1, (uint32_t)&cands[0][k], 1);
        }
        if (n < 127) { line[n++] = ' '; puts(" "); }
        line[n] = 0;
        *np = n;
        return 1;
    }
    puts("\n");
    for (int i = 0; i < nc; i++) { puts(cands[i]); puts("  "); }
    puts("\nsh> ");
    puts(line);
    return 0;
}

void shell(void){
    puts("charOS sh\n");
    char line[128];
    while(1){
        puts("sh> ");
        int n=0;
        int esc = 0; /* 0 yok, 1 ESC, 2 ESC[ */
        while(n<127){
            char c = 0; int r=syscall(SYS_READ,0,(uint32_t)&c,1);
            if(r<=0) break;
            if (esc == 1) {
                if (c == '[') { esc = 2; continue; }
                esc = 0; /* yalnız ESC: yoksay, karakteri işleme */
                continue;
            }
            if (esc == 2) {
                esc = 0;
                if (c == 'A' || c == 'B') { /* 19J: geçmiş */
                    if (c == 'A' && hist_cur > 0) hist_cur--;
                    if (c == 'B' && hist_cur < hist_n) hist_cur++;
                    const char* h = (hist_cur < hist_n) ? hist[hist_cur] : "";
                    hist_show(n);
                    n = 0;
                    while (h[n] && n < 127) { line[n] = h[n]; n++; }
                    line[n] = 0;
                    puts(line);
                }
                continue;
            }
            if (c == 0x1B) { esc = 1; continue; }
            if (c == '\t') { tab_complete(line, &n); continue; } /* 19J */
            if(c=='\n'||c=='\r'){ line[n]=0; syscall(SYS_WRITE,1,(uint32_t)&c,1); break; }
            if(c=='\b'){ if(n>0) n--; puts("\b \b"); }
            else { line[n++]=c; syscall(SYS_WRITE,1,(uint32_t)&c,1); }
        }
        line[n]=0;
        esc = 0;
        if(n==0) continue;
        hist_push(line); /* 19J */
        int bg=0; if(n>0&&line[n-1]=='&'){ bg=1; line[n-1]=0; n--; }
        /* 19F: ';' ile sırala */
        {
            char* segs[8];
            int ns = 0;
            int i = 0;
            while (line[i] && ns < 8) {
                while (line[i] == ' ') i++;
                if (!line[i]) break;
                segs[ns++] = &line[i];
                while (line[i] && line[i] != ';') i++;
                if (line[i] == ';') { line[i] = 0; i++; }
            }
            for (int s = 0; s < ns; s++) run_chain(segs[s], bg);
        }
    }
}
__attribute__((section(".entry"))) void _start(void){ shell(); }
