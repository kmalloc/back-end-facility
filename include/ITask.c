#include "ITask.h"

void ITask::SetLoop(bool loop) 
{
    m_loop = loop;
}

void ITask::StopLoop() 
{ 
    m_loop = false; 
}

