/*
TEST_OUTPUT:
---
fail_compilation/diag6717.d(12): Error: end of instruction expected, not `h`
---
*/

void main()
{
    version(GNU)
    {
        asm { ""h; }
    }
    else
    {
        asm
        {
            mov AX, 12h ;
        }
    }
}
