// D import file generated from 'src\core\sync\config.d'
module core.sync.config;
version (Posix)
{
    private 
{
    import core.sys.posix.time;
}
    private 
{
    import core.sys.posix.sys.time;
}
    void mktspec(ref timespec t, long delta = 0);
    void mvtspec(ref timespec t, long delta);
}
