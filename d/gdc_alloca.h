/* This should be autoconf'd, but I want to avoid
   patching the configure script. */
#include <stdlib.h>
#ifndef alloca
# if _WIN32
#  include <malloc.h>
# elif __sun__
#  include <alloca.h>
# elif SKYOS
#  define alloca __builtin_alloca
# elif defined(__APPLE__) && (GCC_VER <= 33) || defined(__OpenBSD__)
#  include <stdlib.h>
# else
/* guess... */
#  include <alloca.h>
# endif
#endif
