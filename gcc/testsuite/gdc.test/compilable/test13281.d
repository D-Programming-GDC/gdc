// REQUIRED_ARGS: -o-
// PERMUTE_ARGS:
/*
TEST_OUTPUT:
---
123
123u
123L
123LU
123.4
123.4F
123.4L
123.4i
123.4Fi
123.4Li
(123.4+5.6i)
(123.4F+5.6Fi)
(123.4L+5.6Li)
---
*/
pragma(msg, 123);
pragma(msg, 123u);
pragma(msg, 123L);
pragma(msg, 123uL);
pragma(msg, 123.4);
pragma(msg, 123.4f);
pragma(msg, 123.4L);
pragma(msg, 123.4i);
pragma(msg, 123.4fi);
pragma(msg, 123.4Li);
pragma(msg, 123.4 +5.6i);
pragma(msg, 123.4f+5.6fi);
pragma(msg, 123.4L+5.6Li);

static assert((123  ).stringof == "123");
static assert((123u ).stringof == "123u");
static assert((123L ).stringof == "123L");
static assert((123uL).stringof == "123LU");
static assert((123.4  ).stringof == "1.234e+2");
static assert((123.4f ).stringof == "1.234e+2F");
static assert((123.4L ).stringof == "1.234e+2L");
static assert((123.4i ).stringof == "1.234e+2i");
static assert((123.4fi).stringof == "1.234e+2Fi");
static assert((123.4Li).stringof == "1.234e+2Li");
static assert((123.4 +5.6i ).stringof == "1.234e+2 + 5.6e+0i");
static assert((123.4f+5.6fi).stringof == "1.234e+2F + 5.6e+0Fi");
static assert((123.4L+5.6Li).stringof == "1.234e+2L + 5.6e+0Li");
