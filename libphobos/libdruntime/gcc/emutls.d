/* GDC -- D front-end for GCC
   Copyright (C) 2013 Free Software Foundation, Inc.

   GCC is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   GCC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.
*/

// This code is based on the libgcc TLS emulation routines.


module gcc.emutls;

import core.stdc.stdlib;
import core.stdc.string;
import gcc.gthreads;
import gcc.builtins;
private alias gcc.builtins.__builtin_machine_uint word_t;
private alias gcc.builtins.__builtin_pointer_uint pointer_t;

struct emutls_object_t
{
  word_t size;
  word_t palign;
  loc_t loc;
  void* templ;

  union loc_t {
    pointer_t offset;
    void* ptr;
  }
}

struct emutls_array_t
{
  pointer_t length;
  void*** ptr;
}

private
{
  static __gshared gthread_key_t emutls_key;
  static __gshared pointer_t emutls_size;
  static __gshared gthread_mutex_t emutls_mutex;
}

extern(C):

private void
emutls_destroy(void* ptr)
{
  emutls_array_t* arr = cast(emutls_array_t*) ptr;

  for (pointer_t i = 0; i < arr.length; i++)
    {
      if (arr.ptr[i])
	free(arr.ptr[i][-1]);
    }

  free(ptr);
}

private nothrow void
emutls_init()
{
  gthread_mutex_init(&emutls_mutex);

  if (gthread_key_create(&emutls_key, &emutls_destroy) != 0)
    abort();
}

private void* 
emutls_alloc(emutls_object_t* obj)
{
  void* ret;

  if (obj.palign <= (void*).sizeof)
    {
      void* ptr = malloc(cast(pointer_t)(obj.size) + (void*).sizeof);
      assert(ptr != null);

      (cast(void**) ptr)[0] = ptr;
      ret = ptr + (void*).sizeof;
    }
  else
    {
      pointer_t alignsize = cast(pointer_t)(obj.palign - 1) + (void*).sizeof;
      void* ptr = malloc(cast(pointer_t)(obj.size) + alignsize);
      assert(ptr != null);

      ret = cast(void*)((cast(pointer_t)(ptr + alignsize)) & ~cast(pointer_t)(obj.palign - 1));
      (cast(void**) ret)[-1] = ptr;
    }

  if (obj.templ)
    memcpy(ret, obj.templ, cast(pointer_t)(obj.size));
  else
    memset(ret, 0, cast(pointer_t)(obj.size));

  return ret;
}

void* 
__emutls_get_address(emutls_object_t* obj)
{
  if (! gthread_active_p())
    {
      if (obj.loc.ptr == null)
	obj.loc.ptr = emutls_alloc(obj);

      return obj.loc.ptr;
    }

  pointer_t offset = obj.loc.offset;

  if (offset == 0)
    {
      static __gshared gthread_once_t once = GTHREAD_ONCE_INIT;
      gthread_once(&once, &emutls_init);
      gthread_mutex_lock(&emutls_mutex);
      offset = obj.loc.offset;

      if (offset == 0)
	{
	  offset = ++emutls_size;
	  obj.loc.offset = offset;
	}

      gthread_mutex_unlock(&emutls_mutex);
    }

  emutls_array_t* arr = cast(emutls_array_t*) gthread_getspecific(emutls_key);
  if (arr == null)
    {
      pointer_t size = offset + 32;
      arr = cast(emutls_array_t*) malloc(emutls_array_t.sizeof);
      assert(arr != null);

      arr.ptr = cast(void***) calloc(size + 1, (void*).sizeof);
      arr.length = size;
      gthread_setspecific(emutls_key, cast(void*) arr);
    }
  else if (offset > arr.length)
    {
      pointer_t orig_size = arr.length;
      pointer_t size = orig_size * 2;

      if (offset > size)
	size = offset + 32;

      arr.ptr = cast(void***) realloc(arr.ptr, (size + 1) * (void*).sizeof);
      assert(arr.ptr != null);

      arr.length = size;
      memset(arr.ptr + orig_size, 0, (size - orig_size) * (void*).sizeof);
      gthread_setspecific(emutls_key, cast(void*) arr);
    }

  void* ret = arr.ptr[offset - 1];
  if (ret == null)
    {
      ret = emutls_alloc(obj);
      arr.ptr[offset - 1] = cast(void**) ret;
    }

  return ret;
}

void
__emutls_register_common(emutls_object_t* obj, word_t size,
			  word_t palign, void* templ)
{
  if (obj.size < size)
    {
      obj.size = size;
      obj.templ = null;
    }

  if (obj.palign < palign)
    obj.palign = palign;

  if (templ && size == obj.size)
    obj.templ = templ;
}

