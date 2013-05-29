#ifndef _I_TASK_H_
#define _I_TASK_H_

class ITask
{
    public:

      ITask():m_loop(false){}
      virtual ~ITask(){}

      virtual bool Run()=0;
      
      void SetLoop(bool loop = true); 
      void StopLoop();
      bool Loop() const { return m_loop; }
      
    private:
      
      bool m_loop;
};

#endif

