/**
 * This module is intended to provide some basic support for lock-free
 * concurrent programming via the GCC builtins if the platform supports it.
 *
 * Copyright: (C) 2010 Iain Buclaw
 * License:   LGPLv3
 * Authors:   Iain Buclaw
 */

module gcc.atomics;

import gcc.builtins;

/**
 * Private helper function for generating similar sync functions
 */
private template __sync_op_and(string op1, string op2)
{
    const __sync_op_and = `
T __sync_` ~ op1 ~ `_and_` ~ op2 ~ `(T)(const ref shared T ptr, T value)
{
    static if (T.sizeof == byte.sizeof)
        return __sync_` ~ op1 ~ `_and_`~ op2 ~`_1(cast(void*) ptr, value);
    else static if (T.sizeof == short.sizeof)
        return __sync_` ~ op1 ~ `_and_`~ op2 ~`_2(cast(void*) ptr, value);
    else static if (T.sizeof == int.sizeof)
        return __sync_` ~ op1 ~ `_and_`~ op2 ~`_4(cast(void*) ptr, value);
    else static if (T.sizeof == long.sizeof)
        return __sync_` ~ op1 ~ `_and_`~ op2 ~`_8(cast(void*) ptr, value);
    else
        static assert(0, "Invalid template type specified.");
}`
    ;
}


/**
 * These builtins perform the operation suggested by the name, and returns
 * the value that had previously been in memory.
 * That is,
 *  { tmp = *ptr; *ptr op= value; return tmp; }
 *  { tmp = *ptr; *ptr = ~(tmp & value); return tmp; }   // nand
 */
mixin(__sync_op_and!("fetch", "add"));
mixin(__sync_op_and!("fetch", "sub"));
mixin(__sync_op_and!("fetch", "or"));
mixin(__sync_op_and!("fetch", "and"));
mixin(__sync_op_and!("fetch", "xor"));
mixin(__sync_op_and!("fetch", "nand"));


/**
 * These builtins perform the operation suggested by the name, and return
 * the new value.
 * That is,
 *  { *ptr op= value; return *ptr; }
 *  { *ptr = ~(*ptr & value); return *ptr; }   // nand
 */
mixin(__sync_op_and!("add", "fetch"));  
mixin(__sync_op_and!("sub", "fetch"));  
mixin(__sync_op_and!("or", "fetch"));  
mixin(__sync_op_and!("and", "fetch"));  
mixin(__sync_op_and!("xor", "fetch"));  
mixin(__sync_op_and!("nand", "fetch"));  


/**
 * These builtins perform an atomic compare and swap.  That is, if the
 * current value of *ptr is oldval, then write newval into *ptr.
 *
 * The "bool" version returns true if the comparison is successful and
 * newval was written.  The "val" version returns the contents of *ptr
 * before the operation.
 */
bool __sync_bool_compare_and_swap(T)(shared(T)* ptr, const T oldval, const T newval)
{
    static if (is(T == class))
    {
        version (D_LP64)
            return __sync_bool_compare_and_swap_8(cast(ulong*) ptr, cast(ulong)(cast(void*) oldval), cast(ulong)(cast(void*) newval));
        else
            return __sync_bool_compare_and_swap_4(cast(uint*) ptr, cast(uint)(cast(void*) oldval), cast(uint)(cast(void*) newval));
    }
    else static if (T.sizeof == byte.sizeof)
        return __sync_bool_compare_and_swap_1(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == short.sizeof)
        return __sync_bool_compare_and_swap_2(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == int.sizeof)
        return __sync_bool_compare_and_swap_4(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == long.sizeof)
        return __sync_bool_compare_and_swap_8(cast(void*) ptr, oldval, newval);
    else
         static assert(0, "Invalid template type specified.");
}

T __sync_val_compare_and_swap(T)(shared(T)* ptr, const T oldval, const T newval)
{
    static if (is(T == class))
    {
        version (D_LP64)
            return cast(T)cast(void*)__sync_val_compare_and_swap_8(cast(ulong*) ptr, cast(ulong)(cast(void*) oldval), cast(ulong)(cast(void*) newval));
        else
            return cast(T)cast(void*)__sync_val_compare_and_swap_4(cast(uint*) ptr, cast(uint)(cast(void*) oldval), cast(uint)(cast(void*) newval));
    }
    static if (T.sizeof == byte.sizeof)
        return __sync_val_compare_and_swap_1(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == short.sizeof)
        return __sync_val_compare_and_swap_2(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == int.sizeof)
        return __sync_val_compare_and_swap_4(cast(void*) ptr, oldval, newval);
    else static if (T.sizeof == long.sizeof)
        return __sync_val_compare_and_swap_8(cast(void*) ptr, oldval, newval);
    else
         static assert(0, "Invalid template type specified.");
}


/**
 * This builtin, as described by Intel, is not a traditional test-and-set
 * operation, but rather an atomic exchange operation.
 * It writes value into *ptr, and returns the previous contents of *ptr.
 *
 * Many targets have only minimal support for such locks, and do not
 * support a full exchange operation.  In this case, a target may support
 * reduced functionality here by which the only valid value to store is
 * the immediate constant 1.  The exact value actually stored in *ptr is
 * implementation defined.
 *
 * This builtin is not a full barrier, but rather an acquire barrier.
 * This means that references after the builtin cannot move to
 * (or be speculated to) before the builtin, but previous memory stores
 * may not be globally visible yet, and previous memory loads may not yet
 * be satisfied.
 */
T __sync_lock_test_and_set(T)(shared(T)* ptr, const T value)
{
    static if (T.sizeof == byte.sizeof)
        return __sync_lock_test_and_set_1(cast(void*) ptr, value);
    else static if (T.sizeof == short.sizeof)
        return __sync_lock_test_and_set_2(cast(void*) ptr, value);
    else static if (T.sizeof == int.sizeof)
        return __sync_lock_test_and_set_4(cast(void*) ptr, value);
    else static if (T.sizeof == long.sizeof)
        return __sync_lock_test_and_set_8(cast(void*) ptr, value);
    else
         static assert(0, "Invalid template type specified.");
}


/**
 * This builtin releases the lock acquired by __sync_lock_test_and_set.
 * Normally this means writing the constant 0 to *ptr.
 *
 * This builtin is not a full barrier, but rather a release barrier.
 * This means that all previous memory stores are globally visible, and
 * all previous memory loads have been satisfied, but following memory
 * reads are not prevented from being speculated to before the barrier.
 */
void __sync_lock_release(T)(shared(T)* ptr)
{
    static if (T.sizeof == byte.sizeof)
        return __sync_lock_release_1(cast(void*) ptr);
    else static if (T.sizeof == short.sizeof)
        return __sync_lock_release_2(cast(void*) ptr);
    else static if (T.sizeof == int.sizeof)
        return __sync_lock_release_4(cast(void*) ptr);
    else static if (T.sizeof == long.sizeof)
        return __sync_lock_release_8(cast(void*) ptr);
    else
         static assert(0, "Invalid template type specified.");
}


