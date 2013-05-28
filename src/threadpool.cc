#include <unistd.h>
#include <pthread.h>


static void (* g_worker_proc)(void*parg);
static int g_worker_num = 4;


void register_worker_proc(workproc*proc)
{
    g_worker_proc = proc;
}

void set_worker_number(int num)
{
    g_worker_proc = num;
}

static void worker(void*parg)
{
    if (g_worker_proc)
        *g_worker_proc(parg);
}


void start_threadpool()
{

}



