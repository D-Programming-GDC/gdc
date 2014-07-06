/**
 * D header file for Android
 *
 * $(LINK2 https://android.googlesource.com/platform/bionic.git/+/master/libc/kernel/uapi/linux/elf.h, bionic linux/elf.h)
 */
module core.sys.android.elf;

version (linux):
extern (C):
pure:
nothrow:

import core.stdc.stdint;

alias uint32_t Elf32_Addr;
alias uint64_t Elf64_Addr;

alias uint16_t Elf32_Half;
alias uint16_t Elf64_Half;

alias uint32_t Elf32_Off;
alias uint64_t Elf64_Off;

alias uint32_t Elf32_Word;
alias int32_t  Elf32_Sword;
alias uint32_t Elf64_Word;
alias int32_t  Elf64_Sword;

alias uint64_t Elf64_Xword;
alias int64_t  Elf64_Sxword;

enum EI_NIDENT = 16;

struct Elf32_Ehdr
{
    char e_ident[EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
}

struct Elf64_Ehdr
{
    char e_ident[EI_NIDENT];
    Elf64_Half    e_type;
    Elf64_Half    e_machine;
    Elf64_Word    e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf64_Word    e_flags;
    Elf64_Half    e_ehsize;
    Elf64_Half    e_phentsize;
    Elf64_Half    e_phnum;
    Elf64_Half    e_shentsize;
    Elf64_Half    e_shnum;
    Elf64_Half    e_shstrndx;
}

enum EI_MAG0 =         0;
enum ELFMAG0 =         0x7f;

enum EI_MAG1 =         1;
enum ELFMAG1 =         'E';

enum EI_MAG2 =         2;
enum ELFMAG2 =         'L';

enum EI_MAG3 =         3;
enum ELFMAG3 =         'F';
enum ELFMAG =          "\177ELF";
enum SELFMAG =         4;

enum EI_CLASS =        4;
enum ELFCLASSNONE =    0;
enum ELFCLASS32 =      1;
enum ELFCLASS64 =      2;
enum ELFCLASSNUM =     3;

enum EI_DATA =         5;
enum ELFDATANONE =     0;
enum ELFDATA2LSB =     1;
enum ELFDATA2MSB =     2;

enum EI_VERSION =      6;

enum EI_OSABI =        7;
enum ELFOSABI_NONE =   0;
enum ELFOSABI_LINUX =  3;
enum ELFOSABI =        ELFOSABI_NONE;

enum EI_PAD =          8;

enum ET_NONE =         0;
enum ET_REL =          1;
enum ET_EXEC =         2;
enum ET_DYN =          3;
enum ET_CORE =         4;
enum ET_LOPROC =       0xff00;
enum ET_HIPROC =       0xffff;

enum EM_NONE =          0;
enum EM_M32 =           1;
enum EM_SPARC =         2;
enum EM_386 =           3;
enum EM_68K =           4;
enum EM_88K =           5;
enum EM_486 =           6;
enum EM_860 =           7;
enum EM_MIPS =          8;
enum EM_MIPS_RS3_LE =  10;
enum EM_MIPS_RS4_LE =  10;

enum EM_PARISC =       15;
enum EM_SPARC32PLUS =  18;
enum EM_PPC =          20;
enum EM_PPC64 =        21;
enum EM_SPU =          23;

enum EM_SH =           42;
enum EM_SPARCV9 =      43;
enum EM_IA_64 =        50;
enum EM_X86_64 =       62;
enum EM_S390 =         22;

enum EM_V850 =         87;
enum EM_M32R =         88;
enum EM_MN10300 =      89;
enum EM_BLACKFIN =     106;
enum EM_TI_C6000 =     140;
enum EM_FRV =          0x5441;
enum EM_AVR32 =        0x18ad;
enum EM_ALPHA =        0x9026;
enum EM_CYGNUS_V850 =  0x9080;
enum EM_CYGNUS_M32R    0x9041;
enum EM_S390_OLD =     0xa390;

enum EM_CYGNUS_MN10300 = 0xbeef;

enum EV_NONE =         0;
enum EV_CURRENT =      1;
enum EV_NUM =          2;

struct Elf32_Shdr
{
  Elf32_Word    sh_name;
  Elf32_Word    sh_type;
  Elf32_Word    sh_flags;
  Elf32_Addr    sh_addr;
  Elf32_Off     sh_offset;
  Elf32_Word    sh_size;
  Elf32_Word    sh_link;
  Elf32_Word    sh_info;
  Elf32_Word    sh_addralign;
  Elf32_Word    sh_entsize;
}

struct Elf64_Shdr
{
  Elf64_Word    sh_name;
  Elf64_Word    sh_type;
  Elf64_Xword   sh_flags;
  Elf64_Addr    sh_addr;
  Elf64_Off     sh_offset;
  Elf64_Xword   sh_size;
  Elf64_Word    sh_link;
  Elf64_Word    sh_info;
  Elf64_Xword   sh_addralign;
  Elf64_Xword   sh_entsize;
}

enum SHN_UNDEF =       0;
enum SHN_LORESERVE =   0xff00;
enum SHN_LOPROC =      0xff00;
enum SHN_HIPROC =      0xff1f;
enum SHN_ABS =         0xfff1;
enum SHN_COMMON =      0xfff2;
enum SHN_HIRESERVE =   0xffff;

enum SHT_NULL =          0;
enum SHT_PROGBITS =      1;
enum SHT_SYMTAB =        2;
enum SHT_STRTAB =        3;
enum SHT_RELA =          4;
enum SHT_HASH =          5;
enum SHT_DYNAMIC =       6;
enum SHT_NOTE =          7;
enum SHT_NOBITS =        8;
enum SHT_REL =           9;
enum SHT_SHLIB =         10;
enum SHT_DYNSYM =        11;
enum SHT_NUM =           12;
enum SHT_LOPROC =        0x70000000;
enum SHT_HIPROC =        0x7fffffff;
enum SHT_LOUSER =        0x80000000;
enum SHT_HIUSER =        0x8fffffff;

enum SHF_WRITE =            (1 << 0);
enum SHF_ALLOC =            (1 << 1);
enum SHF_EXECINSTR =        (1 << 2);
enum SHF_MASKPROC =         0xf0000000;

struct Elf32_Sym
{
  Elf32_Word    st_name;
  Elf32_Addr    st_value;
  Elf32_Word    st_size;
  ubyte st_info;
  ubyte st_other;
  Elf32_Section st_shndx;
}

struct Elf64_Sym
{
  Elf64_Word    st_name;
  ubyte st_info;
  ubyte st_other;
  Elf64_Section st_shndx;
  Elf64_Addr    st_value;
  Elf64_Xword   st_size;
}

extern (D)
{
    auto ELF32_ST_BIND(T)(T val) { return cast(ubyte)val >> 4; }
    auto ELF32_ST_TYPE(T)(T val) { return cast(uint)val & 0xf; }
    alias ELF32_ST_BIND ELF64_ST_BIND;
    alias ELF32_ST_TYPE ELF64_ST_TYPE;
}

enum STB_LOCAL =       0;
enum STB_GLOBAL =      1;
enum STB_WEAK =        2;

enum STT_NOTYPE =      0;
enum STT_OBJECT =      1;
enum STT_FUNC =        2;
enum STT_SECTION =     3;
enum STT_FILE =        4;
enum STT_COMMON =      5;
enum STT_TLS =         6;

struct Elf32_Rel
{
  Elf32_Addr    r_offset;
  Elf32_Word    r_info;
}

struct Elf64_Rel
{
  Elf64_Addr    r_offset;
  Elf64_Xword   r_info;
}

struct Elf32_Rela
{
  Elf32_Addr    r_offset;
  Elf32_Word    r_info;
  Elf32_Sword   r_addend;
}

struct Elf64_Rela
{
  Elf64_Addr    r_offset;
  Elf64_Xword   r_info;
  Elf64_Sxword  r_addend;
}

extern (D)
{
    auto ELF32_R_SYM(V)(V val) { return val >> 8; }
    auto ELF32_R_TYPE(V)(V val) { return val & 0xff; }

    auto ELF64_R_SYM(I)(I i) { return i >> 32; }
    auto ELF64_R_TYPE(I)(I i) { return i & 0xffffffff; }
}

struct Elf32_Phdr
{
  Elf32_Word    p_type;
  Elf32_Off     p_offset;
  Elf32_Addr    p_vaddr;
  Elf32_Addr    p_paddr;
  Elf32_Word    p_filesz;
  Elf32_Word    p_memsz;
  Elf32_Word    p_flags;
  Elf32_Word    p_align;
}

struct Elf64_Phdr
{
  Elf64_Word    p_type;
  Elf64_Word    p_flags;
  Elf64_Off     p_offset;
  Elf64_Addr    p_vaddr;
  Elf64_Addr    p_paddr;
  Elf64_Xword   p_filesz;
  Elf64_Xword   p_memsz;
  Elf64_Xword   p_align;
}

enum PN_XNUM =         0xffff;

enum PT_NULL =         0;
enum PT_LOAD =         1;
enum PT_DYNAMIC =      2;
enum PT_INTERP =       3;
enum PT_NOTE =         4;
enum PT_SHLIB =        5;
enum PT_PHDR =         6;
enum PT_TLS =          7;
enum PT_LOOS =         0x60000000;
enum PT_HIOS =         0x6fffffff;
enum PT_LOPROC =       0x70000000;
enum PT_HIPROC =       0x7fffffff;
enum PT_GNU_EH_FRAME = 0x6474e550;
enum PT_GNU_STACK =    PT_LOOS + 0x474e551;

enum PF_X =            (1 << 0);
enum PF_W =            (1 << 1);
enum PF_R =            (1 << 2);

enum NT_PRSTATUS =     1;
enum NT_PRFPREG =      2;
enum NT_PRPSINFO =     3;
enum NT_TASKSTRUCT =   4;
enum NT_AUXV =         6;
enum NT_SIGINFO =      0x53494749;
enum NT_FILE =         0x46494c45;
enum NT_PRXFPREG =     0x46e62b7f;
enum NT_PPC_VMX =      0x100;
enum NT_PPC_SPE =      0x101;
enum NT_PPC_VSX =      0x102;
enum NT_386_TLS =      0x200;
enum NT_386_IOPERM =   0x201;
enum NT_X86_XSTATE =   0x202;
enum NT_S390_HIGH_GPRS =       0x300;
enum NT_S390_TIMER =   0x301;
enum NT_S390_TODCMP =  0x302;
enum NT_S390_TODPREG = 0x303;
enum NT_S390_CTRS =    0x304;
enum NT_S390_PREFIX =  0x305;
enum NT_S390_LAST_BREAK =      0x306;
enum NT_S390_SYSTEM_CALL =     0x307;
enum NT_S390_TDB =     0x308;
enum NT_ARM_VFP =      0x400;
enum NT_ARM_TLS =      0x401;
enum NT_ARM_HW_BREAK = 0x402;
enum NT_ARM_HW_WATCH = 0x403;
enum NT_METAG_CBUF =   0x500
enum NT_METAG_RPIPE =  0x501
enum NT_METAG_TLS =    0x502

struct Elf32_Dyn
{
  Elf32_Sword   d_tag;
  union _d_un
  {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
  } _d_un d_un;
}

struct Elf64_Dyn
{
  Elf64_Sxword  d_tag;
  union _d_un
  {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
  } _d_un d_un;
}

enum DT_NULL =         0;
enum DT_NEEDED =       1;
enum DT_PLTRELSZ =     2;
enum DT_PLTGOT =       3;
enum DT_HASH =         4;
enum DT_STRTAB =       5;
enum DT_SYMTAB =       6;
enum DT_RELA =         7;
enum DT_RELASZ =       8;
enum DT_RELAENT =      9;
enum DT_STRSZ =        10;
enum DT_SYMENT =       11;
enum DT_INIT =         12;
enum DT_FINI =         13;
enum DT_SONAME =       14;
enum DT_RPATH =        15;
enum DT_SYMBOLIC =     16;
enum DT_REL =          17;
enum DT_RELSZ =        18;
enum DT_RELENT =       19;
enum DT_PLTREL =       20;
enum DT_DEBUG =        21;
enum DT_TEXTREL =      22;
enum DT_JMPREL =       23;
enum DT_ENCODING =     32;
enum DT_LOOS =         0x6000000d;
enum DT_HIOS =         0x6ffff000;
enum DT_VALRNGLO =     0x6ffffd00;
enum DT_VALRNGHI =     0x6ffffdff;
enum DT_ADDRRNGLO =    0x6ffffe00;
enum DT_ADDRRNGHI =    0x6ffffeff;
enum DT_VERSYM =       0x6ffffff0;
enum DT_RELACOUNT =    0x6ffffff9;
enum DT_RELCOUNT =     0x6ffffffa;
enum DT_FLAGS_1 =      0x6ffffffb;
enum DT_VERDEF =       0x6ffffffc;
enum DT_VERDEFNUM =    0x6ffffffd;
enum DT_VERNEED =      0x6ffffffe;
enum DT_VERNEEDNUM =   0x6fffffff;
enum DT_LOPROC =       0x70000000;
enum DT_HIPROC =       0x7fffffff;

struct Elf32_Nhdr
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
}

struct Elf64_Nhdr
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
}
