#ifndef HA64_H
#define HA64_H
#include "arch/x86_64/longmode.h"

#define HA64_MAX_NODES 32
#define HA64_NAME_MAX  64

int ha64_init(void);
int ha64_node_add(const char *name, u32 ip);
int ha64_node_remove(const char *name);
int ha64_node_status(const char *name, int *out_up);
int ha64_failover_trigger(const char *primary, const char *secondary);
int ha64_heartbeat(const char *name);
int ha64_quorum_check(int *out_ok);
int ha64_cluster_report(char *buf, int max);
int ha64_node_promote(const char *name);
int ha64_node_demote(const char *name);
int ha64_sync_state(void);

#endif