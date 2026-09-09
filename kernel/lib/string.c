#include <string.h>

size_t strlen(const char* s) {
    size_t n=0; while(s[n]) n++; return n;
}
int strcmp(const char* a, const char* b) {
    while(*a && *a==*b){a++; b++;} return (unsigned char)*a - (unsigned char)*b;
}
int strncmp(const char* a, const char* b, size_t n) {
    for(size_t i=0;i<n;i++){ if(a[i]!=b[i]) return (unsigned char)a[i]-(unsigned char)b[i]; if(a[i]==0) return 0; } return 0;
}
char* strcpy(char* dst, const char* src) {
    char* r=dst; while((*dst++=*src++)); return r;
}
char* strncpy(char* dst, const char* src, size_t n) {
    size_t i; for(i=0;i<n && src[i];i++) dst[i]=src[i]; for(;i<n;i++) dst[i]=0; return dst;
}
char* strcat(char* dst, const char* src) {
    strcpy(dst+strlen(dst), src); return dst;
}
int memcmp(const void* a, const void* b, size_t n) {
    const uint8_t *x=a,*y=b; for(size_t i=0;i<n;i++) if(x[i]!=y[i]) return x[i]-y[i]; return 0;
}
void* memcpy(void* dst, const void* src, size_t n) {
    uint8_t *d=dst; const uint8_t *s=src; for(size_t i=0;i<n;i++) d[i]=s[i]; return dst;
}
void* memmove(void* dst, const void* src, size_t n) {
    uint8_t *d=dst; const uint8_t *s=src;
    if(d<s) for(size_t i=0;i<n;i++) d[i]=s[i];
    else for(size_t i=n;i>0;i--) d[i-1]=s[i-1];
    return dst;
}
void* memset(void* s, int c, size_t n) {
    uint8_t *p=s; for(size_t i=0;i<n;i++) p[i]=c; return s;
}
static char* strtok_save;
char* strtok(char* str, const char* delim) {
    char* start;
    if(str) strtok_save=str;
    if(!strtok_save) return 0;
    // skip delim
    while(*strtok_save) {
        int is_delim=0;
        for(const char* d=delim;*d;d++) if(*strtok_save==*d){is_delim=1; break;}
        if(!is_delim) break;
        strtok_save++;
    }
    if(!*strtok_save){strtok_save=0; return 0;}
    start=strtok_save;
    while(*strtok_save) {
        int is_delim=0;
        for(const char* d=delim;*d;d++) if(*strtok_save==*d){is_delim=1; break;}
        if(is_delim){*strtok_save++=0; break;}
        strtok_save++;
    }
    if(!*strtok_save) strtok_save=0;
    else if(!*strtok_save) strtok_save=0;
    return start;
}
int atoi(const char* s) {
    int r=0, neg=0;
    while(*s==' '||*s=='\t') s++;
    if(*s=='-'){neg=1; s++;} else if(*s=='+') s++;
    while(*s>='0'&&*s<='9'){r=r*10+(*s-'0'); s++;}
    return neg?-r:r;
}