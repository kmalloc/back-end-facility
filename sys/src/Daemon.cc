#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>


void daemonize()
{
    int i,fd0,fd1,fd2;
    pid_t pid,sid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0);
    if (getrlimit(RLIMIT_NOFILE,&rl) < 0)
        exit(0);

    if ((pid = fork()) < 0)
        exit(0);
    else if (pid != 0)
        exit(0);
   
    sid = setsid();
    if (sid < 0)
        exit(0);

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP,&sa,NULL) < 0 )
        exit(0);

    if ((pid = fork()) < 0)
        exit(0);
    else if (pid != 0)
        exit(0);

    if (chdir("/") < 0)
        exit(0);

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    //confirm we have closed stdin,stdout,stderr by above operation.
    //if that is the case new allocated fd number should count from 0.
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    if(fd0 != 0 || fd1 != 1 || fd2 != 2)
        exit(0);

}

/*
int main()
{
  daemonize();
  
  while(1)
      sleep(250);
  
  return 0;
}
*/

