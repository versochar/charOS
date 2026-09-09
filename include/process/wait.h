#ifndef CHAROS_PROCESS_WAIT_H
#define CHAROS_PROCESS_WAIT_H

#include <stdint.h>
#include <process/task.h>

int waitpid(int pid, int* status, int options);
int wait_task(int pid, int* status);
int wait_task_wnohang(int pid, int* status); /* 19G: WNOHANG yoklaması */
void wait_wakeup_parent(struct task* child);

#endif
