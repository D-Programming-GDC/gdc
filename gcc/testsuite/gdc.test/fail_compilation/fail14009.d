/*
TEST_OUTPUT:
---
fail_compilation/fail14009.d(12): Error: expression expected not `:`
---
*/

void main()
{
  version(GNU)
  {
    asm {
      "" : : "r" 1 ? 2 : 3;       // accepted
      "" : : "r" 1 ? 2 : : 3;     // rejected
    }
  }
  else
  {
    asm {
      mov EAX, FS: 1 ? 2 : 3;     // accepted
      mov EAX, FS: 1 ? 2 : : 3;   // rejected
    }
  }
}
