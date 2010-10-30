// D import file generated from 'src\common\core\sync\condition.d'
module core.sync.condition;
public 
{
    import core.sync.exception;
}
public 
{
    import core.sync.mutex;
}
version (Win32)
{
    private 
{
    import core.sync.semaphore;
}
    private 
{
    import core.sys.windows.windows;
}
}
else
{
    version (Posix)
{
    private 
{
    import core.sync.config;
}
    private 
{
    import core.stdc.errno;
}
    private 
{
    import core.sys.posix.pthread;
}
    private 
{
    import core.sys.posix.time;
}
}
}
class Condition
{
    this(Mutex m);
    ~this();
    void wait();
    bool wait(long period);
    void notify();
    void notifyAll();
    private 
{
    version (Win32)
{
    bool timedWait(DWORD timeout);
    void notify(bool all);
    HANDLE m_blockLock;
    HANDLE m_blockQueue;
    Mutex m_assocMutex;
    CRITICAL_SECTION m_unblockLock;
    int m_numWaitersGone = 0;
    int m_numWaitersBlocked = 0;
    int m_numWaitersToUnblock = 0;
}
else
{
    version (Posix)
{
    pthread_cond_t m_hndl;
    pthread_mutex_t* m_mutexAddr;
}
}
}
}
version (unittest)
{
    private 
{
    import core.thread;
}
    private 
{
    import core.sync.mutex;
}
    private 
{
    import core.sync.semaphore;
}
    void testNotify();
    void testNotifyAll();
    void testWaitTimeout();
    }
