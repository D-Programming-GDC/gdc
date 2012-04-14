// { dg-prune-output .* }

pragma(msg, __traits(compiles, mixin("(const(A))[0..0]")));
