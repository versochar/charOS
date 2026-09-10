#include "arch/x86_64/ent.h"
#include <string.h>
#include <stdio.h>

struct ent_user {
    char name[ENT64_NAME_MAX];
    int role_id;
    int used;
};

struct ent_role {
    int id;
    char name[ENT64_NAME_MAX];
    int used;
};

static struct ent_user users[ENT64_MAX_USERS];
static struct ent_role roles[ENT64_MAX_ROLES];
static int user_cnt = 0;
static int role_cnt = 0;
static int features[128];
static int initialized = 0;

int ent64_init(void) {
    memset(users, 0, sizeof(users));
    memset(roles, 0, sizeof(roles));
    memset(features, 0, sizeof(features));
    user_cnt = 0;
    role_cnt = 0;
    initialized = 1;
    return 0;
}

int ent64_user_add(const char *name, int role_id) {
    int i;
    if (!initialized) return -1;
    if (!name) return -2;
    for (i = 0; i < user_cnt; i++) if (strcmp(users[i].name, name)==0) return -3;
    if (user_cnt >= ENT64_MAX_USERS) return -4;
    strncpy(users[user_cnt].name, name, ENT64_NAME_MAX-1);
    users[user_cnt].name[ENT64_NAME_MAX-1] = '\0';
    users[user_cnt].role_id = role_id;
    users[user_cnt].used = 1;
    user_cnt++;
    return 0;
}

int ent64_user_remove(const char *name) {
    int i, j;
    if (!name) return -1;
    for (i = 0; i < user_cnt; i++) {
        if (strcmp(users[i].name, name)==0) {
            for (j = i; j < user_cnt - 1; j++) users[j] = users[j+1];
            user_cnt--;
            return 0;
        }
    }
    return -2;
}

int ent64_user_exists(const char *name) {
    int i;
    if (!name) return 0;
    for (i = 0; i < user_cnt; i++) {
        if (strcmp(users[i].name, name)==0) return 1;
    }
    return 0;
}

int ent64_role_create(int role_id, const char *role_name) {
    int i;
    if (!role_name) return -1;
    for (i = 0; i < role_cnt; i++) if (roles[i].id == role_id) return -2;
    if (role_cnt >= ENT64_MAX_ROLES) return -3;
    roles[role_cnt].id = role_id;
    strncpy(roles[role_cnt].name, role_name, ENT64_NAME_MAX-1);
    roles[role_cnt].name[ENT64_NAME_MAX-1] = '\0';
    roles[role_cnt].used = 1;
    role_cnt++;
    return 0;
}

int ent64_role_assign(int user_index, int role_id) {
    if (user_index < 0 || user_index >= user_cnt) return -1;
    users[user_index].role_id = role_id;
    return 0;
}

int ent64_policy_check(int user_index, int resource_id) {
    if (user_index < 0 || user_index >= user_cnt) return -1;
    return (users[user_index].role_id % 2 == resource_id % 2);
}

int ent64_audit_log(const char *msg) {
    (void)msg;
    return 0;
}

int ent64_license_validate(const char *key, int *out_ok) {
    if (!key || !out_ok) return -1;
    *out_ok = (strlen(key) >= 8);
    return 0;
}

int ent64_feature_enable(int feature_id) {
    if (feature_id < 0 || feature_id >= 128) return -1;
    features[feature_id] = 1;
    return 0;
}

int ent64_feature_status(int feature_id) {
    if (feature_id < 0 || feature_id >= 128) return -1;
    return features[feature_id];
}

int ent64_report_users(char *buf, int max) {
    int i;
    if (!buf || max <= 0) return -1;
    if (max < 32) return -2;
    buf[0] = '\0';
    for (i = 0; i < user_cnt; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%s:%d\n", users[i].name, users[i].role_id);
        if ((int)strlen(buf) + (int)strlen(line) >= max) break;
        strcat(buf, line);
    }
    return 0;
}