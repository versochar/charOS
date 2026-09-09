#ifndef CHAROS_PROCESS_SIGNAL_H
#define CHAROS_PROCESS_SIGNAL_H

#include <stdint.h>
#include <core/isr.h>
struct task;

/* 12G: Real signals */
#define SIGTERM 1
#define SIGUSR1 10
#define SIGCHLD 17
#define SIG_MAX 32

typedef void (*sighandler_t)(int);

void signal_init(void);
void signal_send(int pid, int sig);
int signal_check(void);
int sys_signal_handler(int sig, uint32_t handler);
int sys_kill_handler(int pid, int sig);
int sys_sigreturn_handler(void);
void signal_deliver(struct task* t, struct registers* regs);
int signal_has_pending(struct task* t);

#endif
