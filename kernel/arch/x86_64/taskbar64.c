/* 56H: gorev cubugu — pencere dugmeleri + aciliyet + saat. */
#include "arch/x86_64/longmode.h"

#define TASKBAR64_MAX 32

struct taskbar64_task {
    int used;
    int win;
    char title[64];
    int minimized;
    int urgent;
    int pinned;
    int pos;
};

static struct taskbar64_task taskbar64_tab[TASKBAR64_MAX];

static void taskbar_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int taskbar64_add(int win, const char *title) {
    int i;
    if (win < 0) return -1;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            if (title) taskbar_str_copy(taskbar64_tab[i].title, title, 64);
            return 0;
        }
    }
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (!taskbar64_tab[i].used) {
            int p = 0, k;
            for (k = 0; k < TASKBAR64_MAX; k++) {
                if (taskbar64_tab[k].used) {
                    if (taskbar64_tab[k].pos >= p) p = taskbar64_tab[k].pos + 1;
                }
            }
            taskbar64_tab[i].used = 1;
            taskbar64_tab[i].win = win;
            taskbar_str_copy(taskbar64_tab[i].title, title ? title : "", 64);
            taskbar64_tab[i].minimized = 0;
            taskbar64_tab[i].urgent = 0;
            taskbar64_tab[i].pinned = 0;
            taskbar64_tab[i].pos = p;
            return 0;
        }
    }
    return -2;
}

int taskbar64_remove(int win) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            taskbar64_tab[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int taskbar64_urgent(int win, int on) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            taskbar64_tab[i].urgent = on ? 1 : 0;
            return 0;
        }
    }
    return -1;
}

/* Tik: simge-durumu degistir; donus 1=odaklanmali, 0=kucultuldu. */
int taskbar64_click(int win) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            taskbar64_tab[i].urgent = 0;
            taskbar64_tab[i].minimized = !taskbar64_tab[i].minimized;
            return taskbar64_tab[i].minimized ? 0 : 1;
        }
    }
    return -1;
}

int taskbar64_clock(int hh, int mm, char *out, int max) {
    int pos = 0;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return -1;
    if (!out || max < 6) return -1;
    out[pos++] = (char)('0' + (hh / 10));
    out[pos++] = (char)('0' + (hh % 10));
    out[pos++] = ':';
    out[pos++] = (char)('0' + (mm / 10));
    out[pos++] = (char)('0' + (mm % 10));
    out[pos] = 0;
    return 0;
}

int taskbar64_list(int *wins, int max) {
    int i, n = 0, idx[TASKBAR64_MAX];
    int cnt = 0;
    if (!wins || max <= 0) return -1;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used) {
            idx[cnt++] = i;
        }
    }
    /* simple bubble sort by pos */
    for (i = 0; i < cnt; i++) {
        for (int j = i + 1; j < cnt; j++) {
            if (taskbar64_tab[idx[j]].pos < taskbar64_tab[idx[i]].pos) {
                int t = idx[i];
                idx[i] = idx[j];
                idx[j] = t;
            }
        }
    }
    for (i = 0; i < cnt && n < max; i++) {
        wins[n++] = taskbar64_tab[idx[i]].win;
    }
    return n;
}

int taskbar64_title(int win, char *out, int max) {
    int i;
    if (!out || max <= 0) return -1;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            taskbar_str_copy(out, taskbar64_tab[i].title, max);
            return 0;
        }
    }
    return -1;
}

int taskbar64_minimized(int win) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            return taskbar64_tab[i].minimized;
        }
    }
    return -1;
}

int taskbar64_move(int win, int pos) {
    int i, j, cnt = 0;
    int idx[TASKBAR64_MAX];
    if (pos < 0) return -1;
    /* collect used entries sorted by current pos */
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used) {
            idx[cnt++] = i;
        }
    }
    /* sort idx by pos */
    for (i = 0; i < cnt; i++) {
        for (j = i + 1; j < cnt; j++) {
            if (taskbar64_tab[idx[j]].pos < taskbar64_tab[idx[i]].pos) {
                int t = idx[i];
                idx[i] = idx[j];
                idx[j] = t;
            }
        }
    }
    /* find win */
    int found = -1;
    for (i = 0; i < cnt; i++) {
        if (taskbar64_tab[idx[i]].win == win) {
            found = i;
            break;
        }
    }
    if (found < 0) return -1;
    int win_idx = idx[found];
    /* remove from order */
    for (i = found; i < cnt - 1; i++) idx[i] = idx[i + 1];
    cnt--;
    if (pos > cnt) pos = cnt;
    /* insert */
    for (i = cnt; i > pos; i--) idx[i] = idx[i - 1];
    idx[pos] = win_idx;
    cnt++;
    /* reassign positions */
    for (i = 0; i < cnt; i++) {
        taskbar64_tab[idx[i]].pos = i;
    }
    return 0;
}

int taskbar64_pin(int win, int on) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            taskbar64_tab[i].pinned = on ? 1 : 0;
            return 0;
        }
    }
    return -1;
}

int taskbar64_pinned(int win) {
    int i;
    for (i = 0; i < TASKBAR64_MAX; i++) {
        if (taskbar64_tab[i].used && taskbar64_tab[i].win == win) {
            return taskbar64_tab[i].pinned;
        }
    }
    return -1;
}

int taskbar64_clock12(int hh, int mm, char *out, int max) {
    int pos = 0, h12;
    const char *suff;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return -1;
    if (!out || max < 9) return -1;
    h12 = hh % 12;
    if (h12 == 0) h12 = 12;
    suff = (hh >= 12) ? "PM" : "AM";
    out[pos++] = (char)('0' + (h12 / 10));
    out[pos++] = (char)('0' + (h12 % 10));
    out[pos++] = ':';
    out[pos++] = (char)('0' + (mm / 10));
    out[pos++] = (char)('0' + (mm % 10));
    out[pos++] = ' ';
    out[pos++] = suff[0];
    out[pos++] = suff[1];
    out[pos] = 0;
    return 0;
}
