// D import file generated from 'src\common\core\sync\barrier.d'
module core.sync.barrier;
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
    import core.stdc.errno;
}
    private 
{
    import core.sys.posix.pthread;
}
}
}
class Barrier
{
    this(uint limit)
in
{
assert(limit > 0);
}
body
{
m_lock = new Mutex;
m_cond = new Condition(m_lock);
m_group = 0;
m_limit = limit;
m_count = limit;
}
    void wait();
    private 
{
    Mutex m_lock;
    Condition m_cond;
    uint m_group;
    uint m_limit;
    uint m_count;
}
}
version (unittest)
{
    private 
{
    import core.thread;
}
    }
