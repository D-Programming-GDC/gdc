// D import file generated from 'core\vararg.d'
module core.vararg;
alias void* va_list;
template va_start(T)
{
void va_start(out va_list ap, ref T parmn)
{
ap = cast(va_list)(cast(void*)&parmn + (T.sizeof + (int).sizeof - 1 & ~((int).sizeof - 1)));
}
}
template va_arg(T)
{
T va_arg(ref va_list ap)
{
T arg = *cast(T*)ap;
ap = cast(va_list)(cast(void*)ap + (T.sizeof + (int).sizeof - 1 & ~((int).sizeof - 1)));
return arg;
}
}
void va_end(va_list ap)
{
}
void va_copy(out va_list dst, va_list src)
{
dst = src;
}
