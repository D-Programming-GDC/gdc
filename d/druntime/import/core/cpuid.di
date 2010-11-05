// D import file generated from 'src\core\cpuid.d'
module core.cpuid;
public 
{
    struct CacheInfo
{
    uint size;
    ubyte associativity;
    uint lineSize;
}
    public 
{
    char[] vendor()
{
return vendorID;
}
    char[] processor()
{
return processorName;
}
    __gshared 
{
    CacheInfo[5] datacache;
}
    bool x87onChip()
{
return (features & FPU_BIT) != 0;
}
    bool mmx()
{
return (features & MMX_BIT) != 0;
}
    bool sse()
{
return (features & SSE_BIT) != 0;
}
    bool sse2()
{
return (features & SSE2_BIT) != 0;
}
    bool sse3()
{
return (miscfeatures & SSE3_BIT) != 0;
}
    bool ssse3()
{
return (miscfeatures & SSSE3_BIT) != 0;
}
    bool sse41()
{
return (miscfeatures & SSE41_BIT) != 0;
}
    bool sse42()
{
return (miscfeatures & SSE42_BIT) != 0;
}
    bool sse4a()
{
return (amdmiscfeatures & SSE4A_BIT) != 0;
}
    bool sse5()
{
return (amdmiscfeatures & SSE5_BIT) != 0;
}
    bool amd3dnow()
{
return (amdfeatures & AMD_3DNOW_BIT) != 0;
}
    bool amd3dnowExt()
{
return (amdfeatures & AMD_3DNOW_EXT_BIT) != 0;
}
    bool amdMmx()
{
return (amdfeatures & AMD_MMX_BIT) != 0;
}
    bool hasFxsr()
{
return (features & FXSR_BIT) != 0;
}
    bool hasCmov()
{
return (features & CMOV_BIT) != 0;
}
    bool hasRdtsc()
{
return (features & TIMESTAMP_BIT) != 0;
}
    bool hasCmpxchg8b()
{
return (features & CMPXCHG8B_BIT) != 0;
}
    bool hasCmpxchg16b()
{
return (miscfeatures & CMPXCHG16B_BIT) != 0;
}
    bool hasSysEnterSysExit();
    bool has3dnowPrefetch()
{
return (amdmiscfeatures & AMD_3DNOW_PREFETCH_BIT) != 0;
}
    bool hasLahfSahf()
{
return (amdmiscfeatures & LAHFSAHF_BIT) != 0;
}
    bool hasPopcnt()
{
return (miscfeatures & POPCNT_BIT) != 0;
}
    bool hasLzcnt()
{
return (amdmiscfeatures & LZCNT_BIT) != 0;
}
    bool isX86_64()
{
return (amdfeatures & AMD64_BIT) != 0;
}
    bool isItanium()
{
return (features & IA64_BIT) != 0;
}
    bool hyperThreading()
{
return maxThreads > maxCores;
}
    uint threadsPerCPU()
{
return maxThreads;
}
    uint coresPerCPU()
{
return maxCores;
}
    bool preferAthlon()
{
return probablyAMD && family >= 6;
}
    bool preferPentium4()
{
return probablyIntel && family == 15;
}
    bool preferPentium1()
{
return family < 6 || family == 6 && model < 15 && !probablyIntel;
}
    __gshared 
{
    public 
{
    uint stepping;
    uint model;
    uint family;
    uint numCacheLevels = 1;
    private 
{
    bool probablyIntel;
    bool probablyAMD;
    char[12] vendorID;
    char[] processorName;
    char[48] processorNameBuffer;
    uint features = 0;
    uint miscfeatures = 0;
    uint amdfeatures = 0;
    uint amdmiscfeatures = 0;
    uint maxCores = 1;
    uint maxThreads = 1;
    bool hyperThreadingBit()
{
return (features & HTT_BIT) != 0;
}
    enum : uint
{
FPU_BIT = 1,
TIMESTAMP_BIT = 1 << 4,
MDSR_BIT = 1 << 5,
CMPXCHG8B_BIT = 1 << 8,
SYSENTERSYSEXIT_BIT = 1 << 11,
CMOV_BIT = 1 << 15,
MMX_BIT = 1 << 23,
FXSR_BIT = 1 << 24,
SSE_BIT = 1 << 25,
SSE2_BIT = 1 << 26,
HTT_BIT = 1 << 28,
IA64_BIT = 1 << 30,
}
    enum : uint
{
SSE3_BIT = 1,
PCLMULQDQ_BIT = 1 << 1,
MWAIT_BIT = 1 << 3,
SSSE3_BIT = 1 << 9,
FMA_BIT = 1 << 12,
CMPXCHG16B_BIT = 1 << 13,
SSE41_BIT = 1 << 19,
SSE42_BIT = 1 << 20,
POPCNT_BIT = 1 << 23,
AES_BIT = 1 << 25,
OSXSAVE_BIT = 1 << 27,
AVX_BIT = 1 << 28,
}
    enum : uint
{
AMD_MMX_BIT = 1 << 22,
FFXSR_BIT = 1 << 25,
PAGE1GB_BIT = 1 << 26,
RDTSCP_BIT = 1 << 27,
AMD64_BIT = 1 << 29,
AMD_3DNOW_EXT_BIT = 1 << 30,
AMD_3DNOW_BIT = 1 << 31,
}
    enum : uint
{
LAHFSAHF_BIT = 1,
LZCNT_BIT = 1 << 5,
SSE4A_BIT = 1 << 6,
AMD_3DNOW_PREFETCH_BIT = 1 << 8,
SSE5_BIT = 1 << 11,
}
    version (GNU)
{
}
else
{
    version (D_InlineAsm_X86)
{
    version = Really_D_InlineAsm_X86;
}
}
    version (Really_D_InlineAsm_X86)
{
    __gshared 
{
    uint max_cpuid;
    uint max_extended_cpuid;
}
    void getcacheinfoCPUID2();
    void getcacheinfoCPUID4();
    void getAMDcacheinfo();
    void getCpuInfo0B();
    void cpuidX86();
    bool hasCPUID();
}
else
{
    bool hasCPUID()
{
return false;
}
    void cpuidX86()
{
datacache[0].size = 8;
datacache[0].associativity = 2;
datacache[0].lineSize = 32;
}
}
    void cpuidPPC()
{
enum : int
{
PPC601,
PPC603,
PPC603E,
PPC604,
PPC604E,
PPC620,
PPCG3,
PPCG4,
PPCG5,
}
;
int cputype = PPC603;
uint[] sizes = [4,8,16,16,32,32,32,32,64];
ubyte[] ways = [8,2,4,4,4,8,8,8,8];
uint[] L2size = [0,0,0,0,0,0,0,256,512];
uint[] L3size = [0,0,0,0,0,0,0,2048,0];
datacache[0].size = sizes[cputype];
datacache[0].associativity = ways[cputype];
datacache[0].lineSize = cputype == PPCG5 ? 128 : cputype == PPC620 || cputype == PPCG3 ? 64 : 32;
datacache[1].size = L2size[cputype];
datacache[2].size = L3size[cputype];
datacache[1].lineSize = datacache[0].lineSize;
datacache[2].lineSize = datacache[0].lineSize;
}
    void cpuidSparc()
{
}
    static this();
}
}
}
}
}
