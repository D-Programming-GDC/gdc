// D import file generated from 'src\common\core\sync\rwmutex.d'
module core.sync.rwmutex;
public 
{
    import core.sync.exception;
}
private 
{
    import core.sync.condition;
}
private 
{
    import core.sync.mutex;
}
version (Win32)
{
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
    import core.sys.posix.pthread;
}
}
}
class ReadWriteMutex
{
    enum Policy 
{
PREFER_READERS,
PREFER_WRITERS,
}
    this(Policy policy = Policy.PREFER_WRITERS);
    Policy policy()
{
return m_policy;
}
    Reader reader()
{
return m_reader;
}
    Writer writer()
{
return m_writer;
}
    class Reader : Object.Monitor
{
    this()
{
m_proxy.link = this;
(cast(void**)this)[1] = &m_proxy;
}
    void lock();
    void unlock();
    bool tryLock();
    private 
{
    bool shouldQueueReader();
    struct MonitorProxy
{
    Object.Monitor link;
}
    MonitorProxy m_proxy;
}
}
    class Writer : Object.Monitor
{
    this()
{
m_proxy.link = this;
(cast(void**)this)[1] = &m_proxy;
}
    void lock();
    void unlock();
    bool tryLock();
    private 
{
    bool shouldQueueWriter();
    struct MonitorProxy
{
    Object.Monitor link;
}
    MonitorProxy m_proxy;
}
}
    private 
{
    Policy m_policy;
    Reader m_reader;
    Writer m_writer;
    Mutex m_commonMutex;
    Condition m_readerQueue;
    Condition m_writerQueue;
    int m_numQueuedReaders;
    int m_numActiveReaders;
    int m_numQueuedWriters;
    int m_numActiveWriters;
}
}
version (unittest)
{
    static if(!is(typeof(Thread)))
{
    private 
{
    import core.thread;
}
}
    void testRead(ReadWriteMutex.Policy policy);
    void testReadWrite(ReadWriteMutex.Policy policy);
    }
