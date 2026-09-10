#include "arch/x86_64/ha.h"
#include <string.h>
#include <stdio.h>

struct ha_node {
    char name[HA64_NAME_MAX];
    u32 ip;
    int up;
    int primary;
};

static struct ha_node nodes[HA64_MAX_NODES];
static int node_cnt = 0;
static int initialized = 0;

int ha64_init(void) {
    memset(nodes, 0, sizeof(nodes));
    node_cnt = 0;
    initialized = 1;
    return 0;
}

int ha64_node_add(const char *name, u32 ip) {
    int i;
    if (!initialized) return -1;
    if (!name) return -2;
    for (i = 0; i < node_cnt; i++) if (strcmp(nodes[i].name, name)==0) return -3;
    if (node_cnt >= HA64_MAX_NODES) return -4;
    strncpy(nodes[node_cnt].name, name, HA64_NAME_MAX-1);
    nodes[node_cnt].name[HA64_NAME_MAX-1] = '\0';
    nodes[node_cnt].ip = ip;
    nodes[node_cnt].up = 1;
    nodes[node_cnt].primary = 0;
    node_cnt++;
    return 0;
}

int ha64_node_remove(const char *name) {
    int i, j;
    if (!name) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, name)==0) {
            for (j = i; j < node_cnt - 1; j++) nodes[j] = nodes[j+1];
            node_cnt--;
            return 0;
        }
    }
    return -2;
}

int ha64_node_status(const char *name, int *out_up) {
    int i;
    if (!name || !out_up) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, name)==0) {
            *out_up = nodes[i].up;
            return 0;
        }
    }
    return -2;
}

int ha64_heartbeat(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, name)==0) {
            nodes[i].up = 1;
            return 0;
        }
    }
    return -2;
}

int ha64_failover_trigger(const char *primary, const char *secondary) {
    int i, p=-1, s=-1;
    if (!primary || !secondary) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, primary)==0) p = i;
        if (strcmp(nodes[i].name, secondary)==0) s = i;
    }
    if (p < 0 || s < 0) return -2;
    if (!nodes[p].up) {
        nodes[p].primary = 0;
        nodes[s].primary = 1;
        return 0;
    }
    return -3;
}

int ha64_quorum_check(int *out_ok) {
    int i, up = 0;
    if (!out_ok) return -1;
    for (i = 0; i < node_cnt; i++) if (nodes[i].up) up++;
    *out_ok = up >= (node_cnt / 2 + 1);
    return 0;
}

int ha64_cluster_report(char *buf, int max) {
    int i;
    if (!buf || max <= 0) return -1;
    if (max < 32) return -2;
    buf[0] = '\0';
    for (i = 0; i < node_cnt; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%s:%s:%s\n", nodes[i].name, nodes[i].up?"up":"down", nodes[i].primary?"primary":"");
        if ((int)strlen(buf) + (int)strlen(line) >= max) break;
        strcat(buf, line);
    }
    return 0;
}

int ha64_node_promote(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, name)==0) {
            nodes[i].primary = 1;
            return 0;
        }
    }
    return -2;
}

int ha64_node_demote(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < node_cnt; i++) {
        if (strcmp(nodes[i].name, name)==0) {
            nodes[i].primary = 0;
            return 0;
        }
    }
    return -2;
}

int ha64_sync_state(void) {
    return 0;
}