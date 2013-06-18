
#include "threadpool.h"

#include <unistd.h>
#include <string.h>




static char g_ip[256] = {0};
static char g_port[256];

static void parse_address_fromarg(int argc, const char* argv)
{
  char opt;

  while ((opt = getopt(argc,argv,"a:p:")) != -1)
  {
      switch(opt)
      {
          case 'a':
              strncpy(g_ip,optarg,255);
              break;
          case 'p':
              strncpy(g_port,optarg,255);
              break;
          default:
              break;
      }
  }
}


static void create_socket()
{
}


void run_listener(int argc, const char* argv)
{
    parse_address_fromarg(argc,argv);
}





