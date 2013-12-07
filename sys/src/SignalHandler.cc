#include "SignalHandler.h"


const int kExceptionSignals[] =
{
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};

const int kNumHandledSignals =
    sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);

struct sigaction old_handlers[kNumHandledSignals];
bool handlers_installed = false;
