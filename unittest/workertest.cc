#include "gtest.h"


#include "message.h"

#include "worker.h"


class DummyMessage:public MessageBase
{
    public:
        DummyMessage()
        {
            m_number++;
        }

        virtual bool Run() { sleep(1);++m_counter; return true;}

        static bool AllMsgDone() { return m_counter == m_number;}


        static volatile int m_counter;
        static volatile int m_number;
};

volatile int DummyMessage::m_counter = 0;
volatile int DummyMessage::m_number  = 0;

TEST(WorkerTaskTest,WorkerTest)
{

    Worker worker;
    DummyMessage *msg;


    EXPECT_FALSE(worker.IsRunning());

    msg = new DummyMessage();
    worker.PostMessage(msg);

    msg = new DummyMessage();
    worker.PostMessage(msg);
    
    msg = new DummyMessage();
    worker.PostMessage(msg);


    EXPECT_EQ(DummyMessage::m_number, worker.GetMessageNumber());

    worker.Start();
    sleep(3);
    while(worker.GetMessageNumber() > 0) sleep(3);
   
    sleep(3);
    EXPECT_EQ(DummyMessage::m_number,DummyMessage::m_counter);
    EXPECT_TRUE(DummyMessage::AllMsgDone());


    for (int i = 0 ; i < 100; ++i)
    {
        msg = new DummyMessage();
        worker.PostMessage(msg);
    }

    worker.StopRunning();

    EXPECT_FALSE(DummyMessage::AllMsgDone());
    EXPECT_TRUE(worker.GetMessageNumber() > 0);

}
