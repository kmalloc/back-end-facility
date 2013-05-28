#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

typedef void (WorkProc*)(void*);

void register_worker_proc(workproc* proc);
void set_worker_number(int num);

#endif

