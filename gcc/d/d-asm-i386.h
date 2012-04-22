// d-asm-i386.h -- D frontend for GCC.
// Originally contributed by David Friedman
// Maintained by Iain Buclaw

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

enum Reg
{
  Reg_Invalid = -1,
  Reg_EAX = 0,
  Reg_EBX,
  Reg_ECX,
  Reg_EDX,
  Reg_ESI,
  Reg_EDI,
  Reg_EBP,
  Reg_ESP,
  Reg_ST,
  Reg_ST1, Reg_ST2, Reg_ST3, Reg_ST4, Reg_ST5, Reg_ST6, Reg_ST7,
  Reg_MM0, Reg_MM1, Reg_MM2, Reg_MM3, Reg_MM4, Reg_MM5, Reg_MM6, Reg_MM7,
  Reg_XMM0, Reg_XMM1, Reg_XMM2, Reg_XMM3, Reg_XMM4, Reg_XMM5, Reg_XMM6, Reg_XMM7,
  
  Reg_RAX, Reg_RBX, Reg_RCX, Reg_RDX, Reg_RSI, Reg_RDI, Reg_RBP, Reg_RSP,
  Reg_R8, Reg_R9, Reg_R10, Reg_R11, Reg_R12, Reg_R13, Reg_R14, Reg_R15,
  Reg_R8B, Reg_R9B, Reg_R10B, Reg_R11B, Reg_R12B, Reg_R13B, Reg_R14B, Reg_R15B,
  Reg_R8W, Reg_R9W, Reg_R10W, Reg_R11W, Reg_R12W, Reg_R13W, Reg_R14W, Reg_R15W,
  Reg_R8D, Reg_R9D, Reg_R10D, Reg_R11D, Reg_R12D, Reg_R13D, Reg_R14D, Reg_R15D,
  Reg_XMM8, Reg_XMM9, Reg_XMM10, Reg_XMM11, Reg_XMM12, Reg_XMM13, Reg_XMM14, Reg_XMM15,
  Reg_RIP,
  Reg_SIL, Reg_DIL, Reg_BPL, Reg_SPL,
  
  Reg_EFLAGS,
  Reg_CS,
  Reg_DS,
  Reg_SS,
  Reg_ES,
  Reg_FS,
  Reg_GS,
  Reg_AX, Reg_BX, Reg_CX, Reg_DX, Reg_SI, Reg_DI, Reg_BP, Reg_SP,
  Reg_AL, Reg_AH, Reg_BL, Reg_BH, Reg_CL, Reg_CH, Reg_DL, Reg_DH,
  Reg_CR0, Reg_CR2, Reg_CR3, Reg_CR4,
  Reg_DR0, Reg_DR1, Reg_DR2, Reg_DR3, Reg_DR6, Reg_DR7,
  Reg_TR3, Reg_TR4, Reg_TR5, Reg_TR6, Reg_TR7,
  
  Reg_MAX
};

static const int N_Regs = Reg_MAX;

struct RegInfo
{
  const char * name;
  tree gccName; // GAS will take upper case, but GCC won't (needed for the clobber list)
  Identifier * ident;
  char size;
  char baseReg; // %% todo: Reg, Reg_XX
};

static RegInfo regInfo[N_Regs] =
{
    { "EAX", NULL_TREE, NULL, 4,  Reg_EAX },
    { "EBX", NULL_TREE, NULL, 4,  Reg_EBX },
    { "ECX", NULL_TREE, NULL, 4,  Reg_ECX },
    { "EDX", NULL_TREE, NULL, 4,  Reg_EDX },
    { "ESI", NULL_TREE, NULL, 4,  Reg_ESI },
    { "EDI", NULL_TREE, NULL, 4,  Reg_EDI },
    { "EBP", NULL_TREE, NULL, 4,  Reg_EBP },
    { "ESP", NULL_TREE, NULL, 4,  Reg_ESP },
    { "ST", NULL_TREE, NULL,   10, Reg_ST },
    { "ST(1)", NULL_TREE, NULL,10, Reg_ST1 },
    { "ST(2)", NULL_TREE, NULL,10, Reg_ST2 },
    { "ST(3)", NULL_TREE, NULL,10, Reg_ST3 },
    { "ST(4)", NULL_TREE, NULL,10, Reg_ST4 },
    { "ST(5)", NULL_TREE, NULL,10, Reg_ST5 },
    { "ST(6)", NULL_TREE, NULL,10, Reg_ST6 },
    { "ST(7)", NULL_TREE, NULL,10, Reg_ST7 },
    { "MM0", NULL_TREE, NULL, 8, Reg_MM0 },
    { "MM1", NULL_TREE, NULL, 8, Reg_MM1 },
    { "MM2", NULL_TREE, NULL, 8, Reg_MM2 },
    { "MM3", NULL_TREE, NULL, 8, Reg_MM3 },
    { "MM4", NULL_TREE, NULL, 8, Reg_MM4 },
    { "MM5", NULL_TREE, NULL, 8, Reg_MM5 },
    { "MM6", NULL_TREE, NULL, 8, Reg_MM6 },
    { "MM7", NULL_TREE, NULL, 8, Reg_MM7 },
    { "XMM0", NULL_TREE, NULL, 16, Reg_XMM0 },
    { "XMM1", NULL_TREE, NULL, 16, Reg_XMM1 },
    { "XMM2", NULL_TREE, NULL, 16, Reg_XMM2 },
    { "XMM3", NULL_TREE, NULL, 16, Reg_XMM3 },
    { "XMM4", NULL_TREE, NULL, 16, Reg_XMM4 },
    { "XMM5", NULL_TREE, NULL, 16, Reg_XMM5 },
    { "XMM6", NULL_TREE, NULL, 16, Reg_XMM6 },
    { "XMM7", NULL_TREE, NULL, 16, Reg_XMM7 },
    { "RAX", NULL_TREE, NULL, 8,  Reg_RAX },
    { "RBX", NULL_TREE, NULL, 8,  Reg_RBX },
    { "RCX", NULL_TREE, NULL, 8,  Reg_RCX },
    { "RDX", NULL_TREE, NULL, 8,  Reg_RDX },
    { "RSI", NULL_TREE, NULL, 8,  Reg_RSI },
    { "RDI", NULL_TREE, NULL, 8,  Reg_RDI },
    { "RBP", NULL_TREE, NULL, 8,  Reg_RBP },
    { "RSP", NULL_TREE, NULL, 8,  Reg_RSP },
    { "R8",  NULL_TREE, NULL, 8,  Reg_R8 },
    { "R9",  NULL_TREE, NULL, 8,  Reg_R9 },
    { "R10", NULL_TREE, NULL, 8,  Reg_R10 },
    { "R11", NULL_TREE, NULL, 8,  Reg_R11 },
    { "R12", NULL_TREE, NULL, 8,  Reg_R12 },
    { "R13", NULL_TREE, NULL, 8,  Reg_R13 },
    { "R14", NULL_TREE, NULL, 8,  Reg_R14 },
    { "R15", NULL_TREE, NULL, 8,  Reg_R15 },
    { "R8B", NULL_TREE, NULL, 1,  Reg_R8 },
    { "R9B", NULL_TREE, NULL, 1,  Reg_R9 },
    { "R10B", NULL_TREE, NULL, 1,  Reg_R10 },
    { "R11B", NULL_TREE, NULL, 1,  Reg_R11 },
    { "R12B", NULL_TREE, NULL, 1,  Reg_R12 },
    { "R13B", NULL_TREE, NULL, 1,  Reg_R13 },
    { "R14B", NULL_TREE, NULL, 1,  Reg_R14 },
    { "R15B", NULL_TREE, NULL, 1,  Reg_R15 },
    { "R8W",  NULL_TREE, NULL, 2,  Reg_R8 },
    { "R9W",  NULL_TREE, NULL, 2,  Reg_R9 },
    { "R10W", NULL_TREE, NULL, 2,  Reg_R10 },
    { "R11W", NULL_TREE, NULL, 2,  Reg_R11 },
    { "R12W", NULL_TREE, NULL, 2,  Reg_R12 },
    { "R13W", NULL_TREE, NULL, 2,  Reg_R13 },
    { "R14W", NULL_TREE, NULL, 2,  Reg_R14 },
    { "R15W", NULL_TREE, NULL, 2,  Reg_R15 },
    { "R8D",  NULL_TREE, NULL, 4,  Reg_R8 },
    { "R9D",  NULL_TREE, NULL, 4,  Reg_R9 },
    { "R10D", NULL_TREE, NULL, 4,  Reg_R10 },
    { "R11D", NULL_TREE, NULL, 4,  Reg_R11 },
    { "R12D", NULL_TREE, NULL, 4,  Reg_R12 },
    { "R13D", NULL_TREE, NULL, 4,  Reg_R13 },
    { "R14D", NULL_TREE, NULL, 4,  Reg_R14 },
    { "R15D", NULL_TREE, NULL, 4,  Reg_R15 },
    { "XMM8",  NULL_TREE, NULL, 16, Reg_XMM8 },
    { "XMM9",  NULL_TREE, NULL, 16, Reg_XMM9 },
    { "XMM10", NULL_TREE, NULL, 16, Reg_XMM10 },
    { "XMM11", NULL_TREE, NULL, 16, Reg_XMM11 },
    { "XMM12", NULL_TREE, NULL, 16, Reg_XMM12 },
    { "XMM13", NULL_TREE, NULL, 16, Reg_XMM13 },
    { "XMM14", NULL_TREE, NULL, 16, Reg_XMM14 },
    { "XMM15", NULL_TREE, NULL, 16, Reg_XMM15 },
    { "RIP", NULL_TREE, NULL, 8,  Reg_RIP },
    { "SIL", NULL_TREE, NULL, 1,  Reg_SIL },
    { "DIL", NULL_TREE, NULL, 1,  Reg_DIL },
    { "BPL", NULL_TREE, NULL, 1,  Reg_BPL },
    { "SPL", NULL_TREE, NULL, 1,  Reg_SPL },
    { "FLAGS", NULL_TREE, NULL, 0, Reg_EFLAGS }, // the gcc name is "flags"; not used in assembler input
    { "CS",  NULL_TREE, NULL, 2, -1 },
    { "DS",  NULL_TREE, NULL, 2, -1 },
    { "SS",  NULL_TREE, NULL, 2, -1 },
    { "ES",  NULL_TREE, NULL, 2, -1 },
    { "FS",  NULL_TREE, NULL, 2, -1 },
    { "GS",  NULL_TREE, NULL, 2, -1 },
    { "AX",  NULL_TREE, NULL, 2,  Reg_EAX },
    { "BX",  NULL_TREE, NULL, 2,  Reg_EBX },
    { "CX",  NULL_TREE, NULL, 2,  Reg_ECX },
    { "DX",  NULL_TREE, NULL, 2,  Reg_EDX },
    { "SI",  NULL_TREE, NULL, 2,  Reg_ESI },
    { "DI",  NULL_TREE, NULL, 2,  Reg_EDI },
    { "BP",  NULL_TREE, NULL, 2,  Reg_EBP },
    { "SP",  NULL_TREE, NULL, 2,  Reg_ESP },
    { "AL",  NULL_TREE, NULL, 1,  Reg_EAX },
    { "AH",  NULL_TREE, NULL, 1,  Reg_EAX },
    { "BL",  NULL_TREE, NULL, 1,  Reg_EBX },
    { "BH",  NULL_TREE, NULL, 1,  Reg_EBX },
    { "CL",  NULL_TREE, NULL, 1,  Reg_ECX },
    { "CH",  NULL_TREE, NULL, 1,  Reg_ECX },
    { "DL",  NULL_TREE, NULL, 1,  Reg_EDX },
    { "DH",  NULL_TREE, NULL, 1,  Reg_EDX },
    { "CR0", NULL_TREE, NULL, 0, -1 },
    { "CR2", NULL_TREE, NULL, 0, -1 },
    { "CR3", NULL_TREE, NULL, 0, -1 },
    { "CR4", NULL_TREE, NULL, 0, -1 },
    { "DR0", NULL_TREE, NULL, 0, -1 },
    { "DR1", NULL_TREE, NULL, 0, -1 },
    { "DR2", NULL_TREE, NULL, 0, -1 },
    { "DR3", NULL_TREE, NULL, 0, -1 },
    { "DR6", NULL_TREE, NULL, 0, -1 },
    { "DR7", NULL_TREE, NULL, 0, -1 },
    { "TR3", NULL_TREE, NULL, 0, -1 },
    { "TR4", NULL_TREE, NULL, 0, -1 },
    { "TR5", NULL_TREE, NULL, 0, -1 },
    { "TR6", NULL_TREE, NULL, 0, -1 },
    { "TR7", NULL_TREE, NULL, 0, -1 }
};

enum TypeNeeded
{
  No_Type_Needed,
  Int_Types,
  Word_Types, // same as Int_Types, but byte is not allowed
  FP_Types,
  FPInt_Types,
  Byte_NoType, // byte only, but no type suffix
};

enum OpLink
{
  No_Link,
  Out_Mnemonic,
  Next_Form
};

enum ImplicitClober
{
  Clb_SizeAX   = 0x01,
  Clb_SizeDXAX = 0x02,
  Clb_EAX      = 0x03,
  Clb_DXAX_Mask = 0x03,
  
  Clb_Flags    = 0x04,
  Clb_DI       = 0x08,
  Clb_SI       = 0x10,
  Clb_CX       = 0x20,
  Clb_ST       = 0x40,
  Clb_SP       = 0x80 // Doesn't actually let GCC know the frame pointer is modified
};

// "^ +/..\([A-Za-z_0-9]+\).*" -> "    \1,"
enum AsmOp
{
  Op_Invalid,
  Op_Adjust,
  Op_Dst,
  Op_Upd,
  Op_DstW,
  Op_DstF,
  Op_UpdF,
  Op_DstQ,
  Op_DstSrc,
  Op_DstSrcF,
  Op_UpdSrcF,
  Op_DstSrcFW,
  Op_UpdSrcFW,
  Op_DstSrcSSE,
  Op_DstSrcMMX,
  Op_DstSrcImmS,
  Op_DstSrcImmM,
  Op_DstSrcXmmS,
  Op_UpdSrcShft,
  Op_DstSrcNT,
  Op_UpdSrcNT,
  Op_DstMemNT,
  Op_DstRMBNT,
  Op_DstRMWNT,
  Op_UpdUpd,
  Op_UpdUpdF,
  Op_Src,
  Op_SrcRMWNT,
  Op_SrcW,
  Op_SrcImm,
  Op_Src_DXAXF,
  Op_SrcMemNT,
  Op_SrcMemNTF,
  Op_SrcSrc,
  Op_SrcSrcF,
  Op_SrcSrcFW,
  Op_SrcSrcSSEF,
  Op_SrcSrcMMX,
  Op_Shift,
  Op_Branch,
  Op_CBranch,
  Op_0,
  Op_0_AX,
  Op_0_DXAX,
  Op_Loop,
  Op_Flags,
  Op_F0_ST,
  Op_F0_P,
  Op_Fs_P,
  Op_Fis,
  Op_Fis_ST,
  Op_Fis_P,
  Op_Fid,
  Op_Fid_P,
  Op_FidR_P,
  Op_Ffd,
  Op_FfdR,
  Op_Ffd_P,
  Op_FfdR_P,
  Op_FfdRR_P,
  Op_Fd_P,
  Op_FdST,
  Op_FMath,
  Op_FMath0,
  Op_FMath2,
  Op_FdSTiSTi,
  Op_FdST0ST1,
  Op_FPMath,
  Op_FCmp,
  Op_FCmp1,
  Op_FCmpP,
  Op_FCmpP1,
  Op_FCmpFlg0,
  Op_FCmpFlg1,
  Op_FCmpFlg,
  Op_FCmpFlgP0,
  Op_FCmpFlgP1,
  Op_FCmpFlgP,
  Op_fld,
  Op_fldR,
  Op_fxch,
  Op_fxch1,
  Op_fxch0,
  Op_SizedStack,
  Op_bound,
  Op_bswap,
  Op_cmps,
  Op_cmpsd,
  Op_cmpsX,
  Op_cmpxchg,
  Op_cmpxchg8b,
  Op_cpuid,
  Op_enter,
  Op_fdisi,
  Op_feni,
  Op_fsetpm,
  Op_fXstsw,
  Op_imul,
  Op_imul2,
  Op_imul1,
  Op_in,
  Op_ins,
  Op_insX,
  Op_iret,
  Op_iretd,
  Op_iretq,
  Op_lods,
  Op_lodsX,
  Op_movs,
  Op_movsd,
  Op_movsX,
  Op_movsx,
  Op_movzx,
  Op_mul,
  Op_out,
  Op_outs,
  Op_outsX,
  Op_push,
  Op_pushq,
  Op_ret,
  Op_retf,
  Op_scas,
  Op_scasX,
  Op_stos,
  Op_stosX,
  Op_xlat,
  N_AsmOpInfo,
  Op_Align,
  Op_Even,
  Op_Naked,
  Op_db,
  Op_ds,
  Op_di,
  Op_dl,
  Op_df,
  Op_dd,
  Op_de
};

enum OprVals
{
  Opr_None      = 0x0,
  OprC_MRI      = 0x1,
  OprC_MR       = 0x2,
  OprC_Mem      = 0x4,
  OprC_Reg      = 0x8,
  OprC_Imm      = 0x10,
  OprC_SSE      = 0x20,
  OprC_SSE_Mem  = 0x40,
  OprC_R32      = 0x80,
  OprC_RWord    = 0x100,
  OprC_RFP      = 0x200,
  OprC_AbsRel   = 0x400,
  OprC_Relative = 0x800,
  OprC_Port     = 0x1000, // DX or imm
  OprC_AX       = 0x2000, // AL,AX,EAX,RAX
  OprC_DX       = 0x4000, // only DX
  OprC_MMX      = 0x8000,
  OprC_MMX_Mem  = 0x10000,
  OprC_Shift    = 0x20000, // imm or CL
  
  Opr_ClassMask = 0x40000,
  
  Opr_Dest      = 0x80000,
  Opr_Update    = 0x100000,
  
  Opr_NoType    = 0x80000000,
};


typedef unsigned Opr;

struct AsmOpInfo
{
  Opr operands[3];
  unsigned char
    needsType : 3,
      	      implicitClobbers : 8,
      	      linkType : 2;
  unsigned link;
  
  unsigned nOperands() const {
      if (!operands[0])
	return 0;
      else if (!operands[1])
	return 1;
      else if (!operands[2])
	return 2;
      else
	return 3;
  }
};

enum Alternate_Mnemonics
{
  Mn_fdisi,
  Mn_feni,
  Mn_fsetpm,
  Mn_iretw,
  Mn_iret,
  Mn_iretq,
  Mn_lret,
  N_AltMn
};

static const char * alternateMnemonics[N_AltMn] =
{
  ".byte 0xdb, 0xe1",
  ".byte 0xdb, 0xe0",
  ".byte 0xdb, 0xe4",
  "iretw",
  "iret",
  "iretq",
  "lret",
};

#define mri  OprC_MRI
#define mr   OprC_MR
#define mem  OprC_Mem
// for now mfp=mem
#define mfp  OprC_Mem
#define reg  OprC_Reg
#define imm  OprC_Imm
#define sse  OprC_SSE
#define ssem OprC_SSE_Mem
#define mmx  OprC_MMX
#define mmxm OprC_MMX_Mem
#define r32  OprC_R32
#define rw   OprC_RWord
#define rfp  OprC_RFP
#define port OprC_Port
#define ax   OprC_AX
#define dx   OprC_DX
#define shft OprC_Shift
#define D    Opr_Dest
#define U    Opr_Update
#define N    Opr_NoType
//#define L    Opr_Label

// D=dest, N=notype
static const AsmOpInfo asmOpInfo[N_AsmOpInfo] =
{
  /* Op_Invalid   */  {},
  /* Op_Adjust    */  { 0,0,0,             0, Clb_EAX /*just AX*/ },
  /* Op_Dst       */  { D|mr,  0,    0,    1  },
  /* Op_Upd       */  { U|mr,  0,    0,    1  },
  /* Op_DstW      */  { D|mr,  0,    0,    Word_Types  },
  /* Op_DstF      */  { D|mr,  0,    0,    1, Clb_Flags },
  /* Op_UpdF      */  { U|mr,  0,    0,    1, Clb_Flags },
  /* Op_DstQ      */  { D|mr,  0,    0,    0  },
  /* Op_DstSrc    */  { D|mr,  mri,  0,/**/1  },
  /* Op_DstSrcF   */  { D|mr,  mri,  0,/**/1, Clb_Flags },
  /* Op_UpdSrcF   */  { U|mr,  mri,  0,/**/1, Clb_Flags },
  /* Op_DstSrcFW  */  { D|mr,  mri,  0,/**/Word_Types, Clb_Flags },
  /* Op_UpdSrcFW  */  { U|mr,  mri,  0,/**/Word_Types, Clb_Flags },
  /* Op_DstSrcSSE */  { U|sse, ssem, 0     },  // some may not be update %%
  /* Op_DstSrcMMX */  { U|mmx, mmxm, 0     },  // some may not be update %%
  /* Op_DstSrcImmS*/  { U|sse, ssem, N|imm  }, // some may not be update %%
  /* Op_DstSrcImmM*/  { U|mmx, mmxm, N|imm  }, // some may not be update %%
  /* Op_DstSrcXmmS*/  { U|sse, ssem, N|mmx  }, // some may not be update %%
  /* Op_UpdSrcShft*/  { U|mr,  reg,  N|shft, 1, Clb_Flags }, // 16/32 only
  /* Op_DstSrcNT  */  { D|mr,  mr,   0,    0 }, // used for movd .. operands can be rm32,sse,mmx
  /* Op_UpdSrcNT  */  { U|mr,  mr,   0,    0 }, // used for movd .. operands can be rm32,sse,mmx
  /* Op_DstMemNT  */  { D|mem, 0,    0     },
  /* Op_DstRMBNT  */  { D|mr,  0,    0,    Byte_NoType },
  /* Op_DstRMWNT  */  { D|mr,  0,    0     },
  /* Op_UpdUpd    */  { U|mr,U|mr,   0,/**/1  },
  /* Op_UpdUpdF   */  { U|mr,U|mr,   0,/**/1, Clb_Flags },
  /* Op_Src       */  {   mri, 0,    0,    1  },
  /* Op_SrcRMWNT  */  {   mr,  0,    0,    0  },
  /* Op_SrcW      */  {   mri, 0,    0,    Word_Types  },
  /* Op_SrcImm    */  {   imm },
  /* Op_Src_DXAXF */  {   mr,  0,    0,    1, Clb_SizeDXAX|Clb_Flags },
  /* Op_SrcMemNT  */  {   mem, 0,    0     },
  /* Op_SrcMemNTF */  {   mem, 0,    0,    0, Clb_Flags },
  /* Op_SrcSrc    */  {   mr,  mri,  0,    1  },
  /* Op_SrcSrcF   */  {   mr,  mri,  0,    1, Clb_Flags },
  /* Op_SrcSrcFW  */  {   mr,  mri,  0,    Word_Types, Clb_Flags },
  /* Op_SrcSrcSSEF*/  {   sse, ssem, 0,    0, Clb_Flags },
  /* Op_SrcSrcMMX */  {   mmx, mmx,  0, },
  /* Op_Shift     */  { D|mr,N|shft, 0,/**/1, Clb_Flags },
  /* Op_Branch    */  {   mri },
  /* Op_CBranch   */  {   imm },
  /* Op_0         */  {   0,0,0 },
  /* Op_0_AX      */  {   0,0,0,           0, Clb_SizeAX },
  /* Op_0_DXAX    */  {   0,0,0,           0, Clb_SizeDXAX }, // but for cwd/cdq -- how do know the size..
  /* Op_Loop      */  {   imm, 0,    0,    0, Clb_CX },
  /* Op_Flags     */  {   0,0,0,           0, Clb_Flags },
  /* Op_F0_ST     */  {   0,0,0,           0, Clb_ST },
  /* Op_F0_P      */  {   0,0,0,           0, Clb_ST }, // push, pops, etc. not sure how to inform gcc..
  /* Op_Fs_P      */  {   mem, 0,    0,    0, Clb_ST }, // "
  /* Op_Fis       */  {   mem, 0,    0,    FPInt_Types }, // only 16bit and 32bit, DMD defaults to 16bit
  /* Op_Fis_ST    */  {   mem, 0,    0,    FPInt_Types, Clb_ST }, // "
  /* Op_Fis_P     */  {   mem, 0,    0,    FPInt_Types, Clb_ST }, // push and pop, fild so also 64 bit
  /* Op_Fid       */  { D|mem, 0,    0,    FPInt_Types }, // only 16bit and 32bit, DMD defaults to 16bit
  /* Op_Fid_P     */  { D|mem, 0,    0,    FPInt_Types, Clb_ST, Next_Form, Op_FidR_P }, // push and pop, fild so also 64 bit
  /* Op_FidR_P    */  { D|mem,rfp,   0,    FPInt_Types, Clb_ST }, // push and pop, fild so also 64 bit
  /* Op_Ffd       */  { D|mfp, 0,    0,    FP_Types, 0, Next_Form, Op_FfdR }, // only 16bit and 32bit, DMD defaults to 16bit, reg form doesn't need type
  /* Op_FfdR      */  { D|rfp, 0,    0  },
  /* Op_Ffd_P     */  { D|mfp, 0,    0,    FP_Types, Clb_ST, Next_Form, Op_FfdR_P, }, // pop, fld so also 80 bit, "
  /* Op_FfdR_P    */  { D|rfp, 0,    0,    0,        Clb_ST, Next_Form, Op_FfdRR_P },
  /* Op_FfdRR_P   */  { D|rfp, rfp,  0,    0,        Clb_ST },
  /* Op_Fd_P      */  { D|mem, 0,    0,    0,        Clb_ST }, // "
  /* Op_FdST      */  { D|rfp, 0,    0  },
  /* Op_FMath     */  {   mfp, 0,    0,    FP_Types, Clb_ST, Next_Form, Op_FMath0  }, // and only single or double prec
  /* Op_FMath0    */  { 0,     0,    0,    0,        Clb_ST, Next_Form, Op_FMath2  },
  /* Op_FMath2    */  { D|rfp, rfp,  0,    0,        Clb_ST, Next_Form, Op_FdST0ST1 },
  /* Op_FdSTiSTi  */  { D|rfp, rfp,  0  },
  /* Op_FdST0ST1  */  { 0,     0,    0  },
  /* Op_FPMath    */  { D|rfp, rfp,  0,    0,        Clb_ST, Next_Form, Op_F0_P }, // pops
  /* Op_FCmp      */  {   mfp, 0,    0,    FP_Types, 0,      Next_Form, Op_FCmp1 }, // DMD defaults to float ptr
  /* Op_FCmp1     */  {   rfp, 0,    0,    0,        0,      Next_Form, Op_0 },
  /* Op_FCmpP     */  {   mfp, 0,    0,    FP_Types, 0,      Next_Form, Op_FCmpP1 }, // pops
  /* Op_FCmpP1    */  {   rfp, 0,    0,    0,        0,      Next_Form, Op_F0_P }, // pops
  /* Op_FCmpFlg0  */  {   0,   0,    0,    0,        Clb_Flags },
  /* Op_FCmpFlg1  */  {   rfp, 0,    0,    0,        Clb_Flags, Next_Form, Op_FCmpFlg0 },
  /* Op_FCmpFlg   */  {   rfp, rfp,  0,    0,        Clb_Flags, Next_Form, Op_FCmpFlg1 },
  /* Op_FCmpFlgP0 */  {   0,   0,    0,    0,        Clb_Flags }, // pops
  /* Op_FCmpFlgP1 */  {   rfp, 0,    0,    0,        Clb_Flags, Next_Form, Op_FCmpFlgP0 }, // pops
  /* Op_FCmpFlgP  */  {   rfp, rfp,  0,    0,        Clb_Flags, Next_Form, Op_FCmpFlgP1 }, // pops
  /* Op_fld       */  {   mfp, 0,    0,    FP_Types, Clb_ST, Next_Form, Op_fldR },
  /* Op_fldR      */  {   rfp, 0,    0,    0,        Clb_ST },
  /* Op_fxch      */  { D|rfp,D|rfp, 0,    0,        Clb_ST, Next_Form, Op_fxch1 }, // not in intel manual?, but DMD allows it (gas won't), second arg must be ST
  /* Op_fxch1     */  { D|rfp, 0,    0,    0,        Clb_ST, Next_Form, Op_fxch0 },
  /* Op_fxch0     */  {   0,   0,    0,    0,        Clb_ST }, // Also clobbers ST(1)
  /* Op_SizedStack*/  {   0,   0,    0,    0,        Clb_SP }, // type suffix special case
  /* Op_bound     */  {   mr,  mri,  0,    Word_Types  }, // operands *not* reversed for gas
  /* Op_bswap     */  { D|r32      },
  /* Op_cmps      */  {   mem, mem, 0,     1, Clb_DI|Clb_SI|Clb_Flags },
  /* Op_cmpsd     */  {   0,   0,   0,     0, Clb_DI|Clb_SI|Clb_Flags, Next_Form, Op_DstSrcImmS },
  /* Op_cmpsX     */  {   0,   0,   0,     0, Clb_DI|Clb_SI|Clb_Flags },
  /* Op_cmpxchg   */  { D|mr,  reg, 0,     1, Clb_SizeAX|Clb_Flags },
  /* Op_cmpxchg8b */  { D|mem/*64*/,0,0,   0, Clb_SizeDXAX/*32*/|Clb_Flags },
  /* Op_cpuid     */  {   0,0,0 },    // Clobbers eax, ebx, ecx, and edx. Handled specially below.
  /* Op_enter     */  {   imm, imm }, // operands *not* reversed for gas, %% inform gcc of EBP clobber?,
  /* Op_fdisi     */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_fdisi },
  /* Op_feni      */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_feni },
  /* Op_fsetpm    */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_fsetpm },
  /* Op_fXstsw    */  { D|mr,  0,   0,     }, // ax is the only allowed register
  /* Op_imul      */  { D|reg, mr,  imm,   1, Clb_Flags, Next_Form, Op_imul2 }, // 16/32 only
  /* Op_imul2     */  { D|reg, mri, 0,     1, Clb_Flags, Next_Form, Op_imul1 }, // 16/32 only
  /* Op_imul1     */  {   mr,  0,   0,     1, Clb_Flags|Clb_SizeDXAX },
  /* Op_in        */  { D|ax,N|port,0,     1  },
  /* Op_ins       */  {   mem,N|dx, 0,     1, Clb_DI }, // can't override ES segment for this one
  /* Op_insX      */  {   0,   0,   0,     0, Clb_DI }, // output segment overrides %% needs work
  /* Op_iret      */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_iretw },
  /* Op_iretd     */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_iret },
  /* Op_iretq     */  {   0,0,0,           0, 0, Out_Mnemonic, Mn_iretq },
  /* Op_lods      */  {   mem, 0,   0,     1, Clb_SI },
  /* Op_lodsX     */  {   0,   0,   0,     0, Clb_SI },
  /* Op_movs      */  {   mem, mem, 0,     1, Clb_DI|Clb_SI }, // only src/DS can be overridden
  /* Op_movsd     */  {   0,   0,   0,     0, Clb_DI|Clb_SI, Next_Form, Op_DstSrcSSE }, // %% gas doesn't accept movsd .. has to movsl
  /* Op_movsX     */  {   0,   0,   0,     0, Clb_DI|Clb_SI },
  /* Op_movsx     */  { D|reg, mr,  0,     1 }, // type suffix is special case
  /* Op_movzx     */  { D|reg, mr,  0,     1 }, // type suffix is special case
  /* Op_mul       */  { U|ax,  mr,  0,     1, Clb_SizeDXAX|Clb_Flags, Next_Form, Op_Src_DXAXF },
  /* Op_out       */  { N|port,ax,  0,     1  },
  /* Op_outs      */  { N|dx,  mem, 0,     1, Clb_SI },
  /* Op_outsX     */  {   0,   0,   0,     0, Clb_SI },
  /* Op_push      */  {   mri, 0,    0,    Word_Types, Clb_SP }, // would be Op_SrcW, but DMD defaults to 32-bit for immediate form
  /* Op_pushq     */  {   mri, 0,    0,    0, Clb_SP },
  /* Op_ret       */  {   imm, 0,   0,     0, 0, Next_Form, Op_0  },
  /* Op_retf      */  {   0,   0,   0,     0, 0, Out_Mnemonic, Mn_lret  },
  /* Op_scas      */  {   mem, 0,   0,     1, Clb_DI|Clb_Flags },
  /* Op_scasX     */  {   0,   0,   0,     0, Clb_DI|Clb_Flags },
  /* Op_stos      */  {   mem, 0,   0,     1, Clb_DI },
  /* Op_stosX     */  {   0,   0,   0,     0, Clb_DI },
  /* Op_xlat      */  {   mem, 0,   0,     0, Clb_SizeAX, Next_Form, Op_0_AX }
  
  /// * Op_arpl      */  { D|mr,  reg }, // 16 only -> DstSrc
  /// * Op_bsX       */  {   rw,  mrw,  0,    1, Clb_Flags },//->srcsrcf
  /// * Op_bt        */  {   mrw, riw,  0,    1, Clb_Flags },//->srcsrcf
  /// * Op_btX       */  { D|mrw, riw,  0,    1, Clb_Flags },//->dstsrcf .. immediate does not contribute to size
  /// * Op_cmovCC    */  { D|rw,  mrw,  0,    1 } // ->dstsrc
};

#undef mri
#undef mr
#undef mem
#undef mfp
#undef reg
#undef imm
#undef sse
#undef ssem
#undef mmx
#undef mmxm
#undef r32
#undef rw
#undef rfp
#undef port
#undef ax
#undef dx
#undef shft
#undef D
#undef U
#undef N
//#undef L

struct AsmOpEnt
{
  const char * inMnemonic;
  AsmOp asmOp;
};

/* Some opcodes which have data size restrictions, but we don't check
   
   cmov, l<segreg> ?, lea, lsl, shld
   
todo: push <immediate> is always the 32-bit form, even tho push <mem> is 16-bit
*/

static const AsmOpEnt opData[] =
{
    { "__emit", Op_db },  // %% Not sure...
    { "_emit",  Op_db },
    { "aaa",    Op_Adjust },
    { "aad",    Op_Adjust },
    { "aam",    Op_Adjust },
    { "aas",    Op_Adjust },
    { "adc",    Op_UpdSrcF },
    { "add",    Op_UpdSrcF },
    { "addpd",  Op_DstSrcSSE },
    { "addps",  Op_DstSrcSSE },
    { "addsd",  Op_DstSrcSSE },
    { "addss",  Op_DstSrcSSE },
    { "addsubpd", Op_DstSrcSSE },
    { "addsubps", Op_DstSrcSSE },
    { "align",  Op_Align },
    { "and",    Op_UpdSrcF },
    { "andnpd", Op_DstSrcSSE },
    { "andnps", Op_DstSrcSSE },
    { "andpd",  Op_DstSrcSSE },
    { "andps",  Op_DstSrcSSE },
    { "arpl",   Op_UpdSrcNT },
    { "blendpd", Op_DstSrcImmS },
    { "blendps", Op_DstSrcImmS },
    { "blendvpd", Op_DstSrcXmmS },
    { "blendvps", Op_DstSrcXmmS },
    { "bound",  Op_bound },
    { "bsf",    Op_SrcSrcFW },
    { "bsr",    Op_SrcSrcFW },
    { "bswap",  Op_bswap },
    { "bt",     Op_SrcSrcFW },
    { "btc",    Op_UpdSrcFW },
    { "btr",    Op_UpdSrcFW },
    { "bts",    Op_UpdSrcFW },
    { "call",   Op_Branch },
    { "cbw",    Op_0_AX },
    { "cdq",    Op_0_DXAX },
    { "clc",    Op_Flags },
    { "cld",    Op_Flags },
    { "clflush",Op_SrcMemNT },
    { "cli",    Op_Flags },
    { "clts",   Op_0 },
    { "cmc",    Op_Flags },
    { "cmova",  Op_DstSrc },
    { "cmovae", Op_DstSrc },
    { "cmovb",  Op_DstSrc },
    { "cmovbe", Op_DstSrc },
    { "cmovc",  Op_DstSrc },
    { "cmove",  Op_DstSrc },
    { "cmovg",  Op_DstSrc },
    { "cmovge", Op_DstSrc },
    { "cmovl",  Op_DstSrc },
    { "cmovle", Op_DstSrc },
    { "cmovna", Op_DstSrc },
    { "cmovnae",Op_DstSrc },
    { "cmovnb", Op_DstSrc },
    { "cmovnbe",Op_DstSrc },
    { "cmovnc", Op_DstSrc },
    { "cmovne", Op_DstSrc },
    { "cmovng", Op_DstSrc },
    { "cmovnge",Op_DstSrc },
    { "cmovnl", Op_DstSrc },
    { "cmovnle",Op_DstSrc },
    { "cmovno", Op_DstSrc },
    { "cmovnp", Op_DstSrc },
    { "cmovns", Op_DstSrc },
    { "cmovnz", Op_DstSrc },
    { "cmovo",  Op_DstSrc },
    { "cmovp",  Op_DstSrc },
    { "cmovpe", Op_DstSrc },
    { "cmovpo", Op_DstSrc },
    { "cmovs",  Op_DstSrc },
    { "cmovz",  Op_DstSrc },
    { "cmp",    Op_SrcSrcF },
    { "cmppd",  Op_DstSrcImmS },
    { "cmpps",  Op_DstSrcImmS },
    { "cmps",   Op_cmps  },
    { "cmpsb",  Op_cmpsX },
    { "cmpsd",  Op_cmpsd }, // string cmp, and SSE cmp
    { "cmpss",  Op_DstSrcImmS },
    { "cmpsw",  Op_cmpsX },
    { "cmpxchg",   Op_cmpxchg },
    { "cmpxchg8b", Op_cmpxchg8b }, // formerly cmpxch8b
    { "comisd", Op_SrcSrcSSEF },
    { "comiss", Op_SrcSrcSSEF },
    { "cpuid",  Op_cpuid },
    { "crc32",  Op_DstSrc },
    { "cvtdq2pd", Op_DstSrcSSE },
    { "cvtdq2ps", Op_DstSrcSSE },
    { "cvtpd2dq", Op_DstSrcSSE },
    { "cvtpd2pi", Op_DstSrcSSE },
    { "cvtpd2ps", Op_DstSrcSSE },
    { "cvtpi2pd", Op_DstSrcSSE },
    { "cvtpi2ps", Op_DstSrcSSE },
    { "cvtps2dq", Op_DstSrcSSE },
    { "cvtps2pd", Op_DstSrcSSE },
    { "cvtps2pi", Op_DstSrcSSE },
    { "cvtsd2si", Op_DstSrcSSE },
    { "cvtsd2ss", Op_DstSrcSSE },
    { "cvtsi2sd", Op_DstSrcSSE },
    { "cvtsi2ss", Op_DstSrcSSE },
    { "cvtss2sd", Op_DstSrcSSE },
    { "cvtss2si", Op_DstSrcSSE },
    { "cvttpd2dq", Op_DstSrcSSE },
    { "cvttpd2pi", Op_DstSrcSSE },
    { "cvttps2dq", Op_DstSrcSSE },
    { "cvttps2pi", Op_DstSrcSSE },
    { "cvttsd2si", Op_DstSrcSSE },
    { "cvttss2si", Op_DstSrcSSE },
    { "cwd",  Op_0_DXAX },
    { "cwde", Op_0_AX },
    //{ "da", Op_ },// dunno what this is -- takes labels?
    { "daa",   Op_Adjust },
    { "das",   Op_Adjust },
    { "db",    Op_db },
    { "dd",    Op_dd },
    { "de",    Op_de },
    { "dec",   Op_UpdF },
    { "df",    Op_df },
    { "di",    Op_di },
    { "div",   Op_Src_DXAXF },
    { "divpd", Op_DstSrcSSE },
    { "divps", Op_DstSrcSSE },
    { "divsd", Op_DstSrcSSE },
    { "divss", Op_DstSrcSSE },
    { "dl",    Op_dl },
    { "dppd",  Op_DstSrcImmS },
    { "dpps",  Op_DstSrcImmS },
    { "dq",    Op_dl },
    { "ds",    Op_ds },
    { "dt",    Op_de },
    { "dw",    Op_ds },
    { "emms",  Op_0 }, // clobber all mmx/fp?
    { "enter", Op_enter },
    { "extractps", Op_DstSrcImmS },
    { "even",  Op_Even },
    { "f2xm1",  Op_F0_ST }, // %% most of these are update...
    { "fabs",   Op_F0_ST },
    { "fadd",   Op_FMath },
    { "faddp",  Op_FPMath },
    { "fbld",   Op_Fs_P },
    { "fbstp",  Op_Fd_P },
    { "fchs",   Op_F0_ST },
    { "fclex",  Op_0 },
    { "fcmovb",   Op_FdSTiSTi }, // but only ST(0) can be the destination -- should be FdST0STi
    { "fcmovbe",  Op_FdSTiSTi },
    { "fcmove",   Op_FdSTiSTi },
    { "fcmovnb",  Op_FdSTiSTi },
    { "fcmovnbe", Op_FdSTiSTi },
    { "fcmovne",  Op_FdSTiSTi },
    { "fcmovnu",  Op_FdSTiSTi },
    { "fcmovu",   Op_FdSTiSTi },
    { "fcom",   Op_FCmp },
    { "fcomi",  Op_FCmpFlg  },
    { "fcomip", Op_FCmpFlgP },
    { "fcomp",  Op_FCmpP },
    { "fcompp", Op_F0_P },   // pops twice
    { "fcos",   Op_F0_ST },
    { "fdecstp",Op_F0_P },   // changes stack
    { "fdisi",  Op_fdisi },
    { "fdiv",   Op_FMath },
    { "fdivp",  Op_FPMath },
    { "fdivr",  Op_FMath },
    { "fdivrp", Op_FPMath },
    { "feni",   Op_feni },
    { "ffree",  Op_FdST },
    { "fiadd",  Op_Fis_ST },
    { "ficom",  Op_Fis   },
    { "ficomp", Op_Fis_P },
    { "fidiv",  Op_Fis_ST },
    { "fidivr", Op_Fis_ST },
    { "fild",   Op_Fis_P },
    { "fimul",  Op_Fis_ST },
    { "fincstp",Op_F0_P },
    { "finit",  Op_F0_P },
    { "fist",   Op_Fid }, // only 16,32bit
    { "fistp",  Op_Fid_P },
    { "fisttp", Op_Fid_P },
    { "fisub",  Op_Fis_ST },
    { "fisubr", Op_Fis_ST },
    { "fld",    Op_fld },
    { "fld1",   Op_F0_P },
    { "fldcw",  Op_SrcMemNT },
    { "fldenv", Op_SrcMemNT },
    { "fldl2e", Op_F0_P },
    { "fldl2t", Op_F0_P },
    { "fldlg2", Op_F0_P },
    { "fldln2", Op_F0_P },
    { "fldpi",  Op_F0_P },
    { "fldz",   Op_F0_P },
    { "fmul",   Op_FMath },
    { "fmulp",  Op_FPMath },
    { "fnclex", Op_0 },
    { "fndisi", Op_fdisi }, // ??
    { "fneni",  Op_feni }, // ??
    { "fninit", Op_0 },
    { "fnop",   Op_0 },
    { "fnsave", Op_DstMemNT },
    { "fnstcw", Op_DstMemNT },
    { "fnstenv",Op_DstMemNT },
    { "fnstsw", Op_fXstsw },
    { "fpatan", Op_F0_P }, // pop and modify new ST
    { "fprem",  Op_F0_ST },
    { "fprem1", Op_F0_ST },
    { "fptan",  Op_F0_P }, // modify ST and push 1.0
    { "frndint",Op_F0_ST },
    { "frstor", Op_SrcMemNT }, // but clobbers everything
    { "fsave",  Op_DstMemNT },
    { "fscale", Op_F0_ST },
    { "fsetpm", Op_fsetpm },
    { "fsin",   Op_F0_ST },
    { "fsincos",Op_F0_P },
    { "fsqrt",  Op_F0_ST },
    { "fst",    Op_Ffd },
    { "fstcw",  Op_DstMemNT },
    { "fstenv", Op_DstMemNT },
    { "fstp",   Op_Ffd_P },
    { "fstsw",  Op_fXstsw },
    { "fsub",   Op_FMath },
    { "fsubp",  Op_FPMath },
    { "fsubr",  Op_FMath },
    { "fsubrp", Op_FPMath },
    { "ftst",   Op_0 },
    { "fucom",  Op_FCmp },
    { "fucomi", Op_FCmpFlg },
    { "fucomip",Op_FCmpFlgP },
    { "fucomp", Op_FCmpP },
    { "fucompp",Op_F0_P }, // pops twice
    { "fwait",  Op_0 },
    { "fxam",   Op_0 },
    { "fxch",   Op_fxch },
    { "fxrstor",Op_SrcMemNT },  // clobbers FP,MMX,SSE
    { "fxsave", Op_DstMemNT },
    { "fxtract",Op_F0_P }, // pushes
    { "fyl2x",  Op_F0_P }, // pops
    { "fyl2xp1",Op_F0_P }, // pops
    { "haddpd", Op_DstSrcSSE },
    { "haddps", Op_DstSrcSSE },
    { "hlt",  Op_0 },
    { "hsubpd", Op_DstSrcSSE },
    { "hsubps", Op_DstSrcSSE },
    { "idiv", Op_Src_DXAXF },
    { "imul", Op_imul },
    { "in",   Op_in },
    { "inc",  Op_UpdF },
    { "ins",  Op_ins },
    { "insb", Op_insX },
    { "insd", Op_insX },
    { "insertps", Op_DstSrcImmS },
    { "insw", Op_insX },
    { "int",  Op_SrcImm },
    { "into", Op_0 },
    { "invd", Op_0 },
    { "invlpg", Op_SrcMemNT },
    { "iret",  Op_iret },
    { "iretd", Op_iretd },
    { "ja",    Op_CBranch },
    { "jae",   Op_CBranch },
    { "jb",    Op_CBranch },
    { "jbe",   Op_CBranch },
    { "jc",    Op_CBranch },
    { "jcxz",  Op_CBranch },
    { "je",    Op_CBranch },
    { "jecxz", Op_CBranch },
    { "jg",    Op_CBranch },
    { "jge",   Op_CBranch },
    { "jl",    Op_CBranch },
    { "jle",   Op_CBranch },
    { "jmp",   Op_Branch },
    { "jna",   Op_CBranch },
    { "jnae",  Op_CBranch },
    { "jnb",   Op_CBranch },
    { "jnbe",  Op_CBranch },
    { "jnc",   Op_CBranch },
    { "jne",   Op_CBranch },
    { "jng",   Op_CBranch },
    { "jnge",  Op_CBranch },
    { "jnl",   Op_CBranch },
    { "jnle",  Op_CBranch },
    { "jno",   Op_CBranch },
    { "jnp",   Op_CBranch },
    { "jns",   Op_CBranch },
    { "jnz",   Op_CBranch },
    { "jo",    Op_CBranch },
    { "jp",    Op_CBranch },
    { "jpe",   Op_CBranch },
    { "jpo",   Op_CBranch },
    { "js",    Op_CBranch },
    { "jz",    Op_CBranch },
    { "lahf",  Op_0_AX },
    { "lar",   Op_DstSrcFW }, // reg dest only
    { "lddqu", Op_DstSrcSSE },
    { "ldmxcsr", Op_SrcMemNT },
    { "lds",   Op_DstSrc },  // reg dest only
    { "lea",   Op_DstSrc },  // "
    { "leave", Op_0 },       // EBP,ESP clobbers
    { "les",   Op_DstSrc },
    { "lfence",Op_0 },
    { "lfs",   Op_DstSrc },
    { "lgdt",  Op_SrcMemNT },
    { "lgs",   Op_DstSrc },
    { "lidt",  Op_SrcMemNT },
    { "lldt",  Op_SrcRMWNT },
    { "lmsw",  Op_SrcRMWNT },
    { "lock",  Op_0 },
    { "lods",  Op_lods },
    { "lodsb", Op_lodsX },
    { "lodsd", Op_lodsX },
    { "lodsw", Op_lodsX },
    { "loop",  Op_Loop },
    { "loope", Op_Loop },
    { "loopne",Op_Loop },
    { "loopnz",Op_Loop },
    { "loopz", Op_Loop },
    { "lsl",   Op_DstSrcFW }, // reg dest only
    { "lss",   Op_DstSrc },
    { "ltr",   Op_DstMemNT },
    { "maskmovdqu", Op_SrcSrcMMX }, // writes to [edi]
    { "maskmovq",   Op_SrcSrcMMX },
    { "maxpd", Op_DstSrcSSE },
    { "maxps", Op_DstSrcSSE },
    { "maxsd", Op_DstSrcSSE },
    { "maxss", Op_DstSrcSSE },
    { "mfence",Op_0},
    { "minpd", Op_DstSrcSSE },
    { "minps", Op_DstSrcSSE },
    { "minsd", Op_DstSrcSSE },
    { "minss", Op_DstSrcSSE },
    { "monitor", Op_0 },
    { "mov",   Op_DstSrc },
    { "movapd",  Op_DstSrcSSE },
    { "movaps",  Op_DstSrcSSE },
    { "movd",    Op_DstSrcNT  }, // also mmx and sse
    { "movddup", Op_DstSrcSSE },
    { "movdq2q", Op_DstSrcNT }, // mmx/sse
    { "movdqa",  Op_DstSrcSSE },
    { "movdqu",  Op_DstSrcSSE },
    { "movhlps", Op_DstSrcSSE },
    { "movhpd",  Op_DstSrcSSE },
    { "movhps",  Op_DstSrcSSE },
    { "movlhps", Op_DstSrcSSE },
    { "movlpd",  Op_DstSrcSSE },
    { "movlps",  Op_DstSrcSSE },
    { "movmskpd",Op_DstSrcSSE },
    { "movmskps",Op_DstSrcSSE },
    { "movntdq", Op_DstSrcNT  }, // limited to sse, but mem dest
    { "movntdqa", Op_DstSrcNT }, // limited to sse, but mem dest
    { "movnti",  Op_DstSrcNT  }, // limited to gpr, but mem dest
    { "movntpd", Op_DstSrcNT  }, // limited to sse, but mem dest
    { "movntps", Op_DstSrcNT  }, // limited to sse, but mem dest
    { "movntq",  Op_DstSrcNT  }, // limited to mmx, but mem dest
    { "movq",    Op_DstSrcNT  }, // limited to sse and mmx
    { "movq2dq", Op_DstSrcNT  }, // limited to sse <- mmx regs
    { "movs",  Op_movs },
    { "movsb", Op_movsX },
    { "movsd", Op_movsd },
    { "movshdup", Op_DstSrcSSE },
    { "movsldup", Op_DstSrcSSE },
    { "movss", Op_DstSrcSSE },
    { "movsw", Op_movsX },
    { "movsx", Op_movsx }, // word-only, reg dest
    { "movupd",Op_DstSrcSSE },
    { "movups",Op_DstSrcSSE },
    { "movzx", Op_movzx },
    { "mpsadbw", Op_DstSrcImmS },
    { "mul",   Op_mul },
    { "mulpd", Op_DstSrcSSE },
    { "mulps", Op_DstSrcSSE },
    { "mulsd", Op_DstSrcSSE },
    { "mulss", Op_DstSrcSSE },
    { "mwait", Op_0 },
    { "naked", Op_Naked },
    { "neg",   Op_UpdF },
    { "nop",   Op_0 },
    { "not",   Op_Upd },
    { "or",    Op_UpdSrcF },
    { "orpd",  Op_DstSrcSSE },
    { "orps",  Op_DstSrcSSE },
    { "out",   Op_out },
    { "outs",  Op_outs },
    { "outsb", Op_outsX },
    { "outsd", Op_outsX },
    { "outsw", Op_outsX },
    { "pabsb",  Op_DstSrcSSE },
    { "pabsd",  Op_DstSrcSSE },
    { "pabsw",  Op_DstSrcSSE },
    { "packssdw", Op_DstSrcMMX }, // %% also SSE
    { "packusdw", Op_DstSrcMMX },
    { "packsswb", Op_DstSrcMMX },
    { "packuswb", Op_DstSrcMMX },
    { "paddb",    Op_DstSrcMMX },
    { "paddd",    Op_DstSrcMMX },
    { "paddq",    Op_DstSrcMMX },
    { "paddsb",   Op_DstSrcMMX },
    { "paddsw",   Op_DstSrcMMX },
    { "paddusb",  Op_DstSrcMMX },
    { "paddusw",  Op_DstSrcMMX },
    { "paddw",    Op_DstSrcMMX },
    { "palignr",  Op_DstSrcSSE },
    { "pand",     Op_DstSrcMMX },
    { "pandn",    Op_DstSrcMMX },
    { "pavgb",    Op_DstSrcMMX },
    { "pavgusb",  Op_DstSrcMMX }, // AMD 3dNow!
    { "pavgw",    Op_DstSrcMMX },
    { "pblendvb", Op_DstSrcXmmS },
    { "pblendw",  Op_DstSrcImmS },
    { "pcmpeqb",  Op_DstSrcMMX },
    { "pcmpeqd",  Op_DstSrcMMX },
    { "pcmpeqq",  Op_DstSrcMMX },
    { "pcmpeqw",  Op_DstSrcMMX },
    { "pcmpestri", Op_DstSrcImmS },
    { "pcmpestrm", Op_DstSrcImmS },
    { "pcmpgtb",  Op_DstSrcMMX },
    { "pcmpgtd",  Op_DstSrcMMX },
    { "pcmpgtq",  Op_DstSrcMMX },
    { "pcmpgtw",  Op_DstSrcMMX },
    { "pcmpistri", Op_DstSrcImmS },
    { "pcmpistrm", Op_DstSrcImmS },
    { "pextrd",   Op_DstSrcImmM }, // gpr32 dest
    { "pextrq",   Op_DstSrcImmM }, // gpr64 dest
    { "pextrw",   Op_DstSrcImmM }, // gpr32 dest
    { "pf2id",    Op_DstSrcMMX }, // %% AMD 3dNow! opcodes
    { "pf2iw",    Op_DstSrcMMX },
    { "pfacc",    Op_DstSrcMMX },
    { "pfadd",    Op_DstSrcMMX },
    { "pfcmpeq",  Op_DstSrcMMX },
    { "pfcmpge",  Op_DstSrcMMX },
    { "pfcmpgt",  Op_DstSrcMMX },
    { "pfmax",    Op_DstSrcMMX },
    { "pfmin",    Op_DstSrcMMX },
    { "pfmul",    Op_DstSrcMMX },
    { "pfnacc",   Op_DstSrcMMX }, // 3dNow values are returned in MM0 register,
    { "pfpnacc",  Op_DstSrcMMX }, // so should be correct to use Op_DstSrcMMX.
    { "pfrcp",    Op_DstSrcMMX },
    { "pfrcpit1", Op_DstSrcMMX },
    { "pfrcpit2", Op_DstSrcMMX },
    { "pfrsqit1", Op_DstSrcMMX },
    { "pfrsqrt",  Op_DstSrcMMX },
    { "pfsub",    Op_DstSrcMMX },
    { "pfsubr",   Op_DstSrcMMX },
    { "phaddd",   Op_DstSrcSSE },
    { "phaddsw",  Op_DstSrcSSE },
    { "phaddw",   Op_DstSrcSSE },
    { "phminposuw", Op_DstSrcSSE },
    { "phsubd",  Op_DstSrcSSE },
    { "phsubsw", Op_DstSrcSSE },
    { "phsubw",  Op_DstSrcSSE },
    { "pi2fd",    Op_DstSrcMMX },
    { "pi2fw",    Op_DstSrcMMX }, // %%
    { "pinsrb",   Op_DstSrcImmM }, // gpr32, sse too
    { "pinsrd",   Op_DstSrcImmM }, // gpr32, sse too
    { "pinsrq",   Op_DstSrcImmM }, // gpr64, sse too
    { "pinsrw",   Op_DstSrcImmM }, // gpr32(16), mem16 src, sse too
    { "pmaddubsw",Op_DstSrcSSE },
    { "pmaddwd",  Op_DstSrcMMX },
    { "pmaxsb",   Op_DstSrcMMX },
    { "pmaxsd",   Op_DstSrcMMX },
    { "pmaxsw",   Op_DstSrcMMX },
    { "pmaxub",   Op_DstSrcMMX },
    { "pmaxud",   Op_DstSrcMMX },
    { "pmaxuw",   Op_DstSrcMMX },
    { "pminsb",   Op_DstSrcMMX },
    { "pminsd",   Op_DstSrcMMX },
    { "pminsw",   Op_DstSrcMMX },
    { "pminub",   Op_DstSrcMMX },
    { "pminud",   Op_DstSrcMMX },
    { "pminuw",   Op_DstSrcMMX },
    { "pmovmskb", Op_DstSrcMMX },
    { "pmovsxbd", Op_DstSrcMMX },
    { "pmovsxbq", Op_DstSrcMMX },
    { "pmovsxbw", Op_DstSrcMMX },
    { "pmovsxdq", Op_DstSrcMMX },
    { "pmovsxwd", Op_DstSrcMMX },
    { "pmovsxwq", Op_DstSrcMMX },
    { "pmovzxbd", Op_DstSrcMMX },
    { "pmovzxbq", Op_DstSrcMMX },
    { "pmovzxdq", Op_DstSrcMMX },
    { "pmovzxbw", Op_DstSrcMMX },
    { "pmovzxwd", Op_DstSrcMMX },
    { "pmovsxwq", Op_DstSrcMMX },
    { "pmuldq",   Op_DstSrcMMX }, // also sse
    { "pmulhrsw", Op_DstSrcMMX },
    { "pmulhrw",  Op_DstSrcMMX }, // AMD 3dNow!
    { "pmulhuw",  Op_DstSrcMMX },
    { "pmulhw",   Op_DstSrcMMX },
    { "pmulld",   Op_DstSrcMMX }, // also sse
    { "pmullw",   Op_DstSrcMMX },
    { "pmuludq",  Op_DstSrcMMX }, // also sse
    { "pop",      Op_DstW },
    { "popa",     Op_SizedStack },  // For intel this is always 16-bit
    { "popad",    Op_SizedStack },  // GAS doesn't accept 'popad' -- these clobber everything, but supposedly it would be used to preserve clobbered regs
    { "popcnt",   Op_DstSrc },
    { "popf",     Op_SizedStack },  // rewrite the insn with a special case
    { "popfd",    Op_SizedStack },
    { "por",      Op_DstSrcMMX },
    { "prefetchnta", Op_SrcMemNT },
    { "prefetcht0",  Op_SrcMemNT },
    { "prefetcht1",  Op_SrcMemNT },
    { "prefetcht2",  Op_SrcMemNT },
    { "psadbw",   Op_DstSrcMMX },
    { "pshufb",   Op_DstSrcImmM },
    { "pshufd",   Op_DstSrcImmM },
    { "pshufhw",  Op_DstSrcImmM },
    { "pshuflw",  Op_DstSrcImmM },
    { "pshufw",   Op_DstSrcImmM },
    { "psignb",   Op_DstSrcSSE },
    { "psignd",   Op_DstSrcSSE },
    { "psignw",   Op_DstSrcSSE },
    { "pslld",    Op_DstSrcMMX }, // immediate operands...
    { "pslldq",   Op_DstSrcMMX },
    { "psllq",    Op_DstSrcMMX },
    { "psllw",    Op_DstSrcMMX },
    { "psrad",    Op_DstSrcMMX },
    { "psraw",    Op_DstSrcMMX },
    { "psrld",    Op_DstSrcMMX },
    { "psrldq",   Op_DstSrcMMX },
    { "psrlq",    Op_DstSrcMMX },
    { "psrlw",    Op_DstSrcMMX },
    { "psubb",    Op_DstSrcMMX },
    { "psubd",    Op_DstSrcMMX },
    { "psubq",    Op_DstSrcMMX },
    { "psubsb",   Op_DstSrcMMX },
    { "psubsw",   Op_DstSrcMMX },
    { "psubusb",  Op_DstSrcMMX },
    { "psubusw",  Op_DstSrcMMX },
    { "psubw",    Op_DstSrcMMX },
    { "pswapd",   Op_DstSrcMMX }, // AMD 3dNow!
    { "ptest",    Op_DstSrcSSE },
    { "punpckhbw", Op_DstSrcMMX },
    { "punpckhdq", Op_DstSrcMMX },
    { "punpckhqdq",Op_DstSrcMMX },
    { "punpckhwd", Op_DstSrcMMX },
    { "punpcklbw", Op_DstSrcMMX },
    { "punpckldq", Op_DstSrcMMX },
    { "punpcklqdq",Op_DstSrcMMX },
    { "punpcklwd", Op_DstSrcMMX },
    { "push",   Op_push },
    { "pusha",  Op_SizedStack },
    { "pushad", Op_SizedStack },
    { "pushf",  Op_SizedStack },
    { "pushfd", Op_SizedStack },
    { "pxor",   Op_DstSrcMMX },
    { "rcl",    Op_Shift }, // limited src operands -- change to shift
    { "rcpps",  Op_DstSrcSSE },
    { "rcpss",  Op_DstSrcSSE },
    { "rcr",    Op_Shift },
    { "rdmsr",  Op_0_DXAX },
    { "rdpmc",  Op_0_DXAX },
    { "rdtsc",  Op_0_DXAX },
    { "rep",    Op_0 },
    { "repe",   Op_0 },
    { "repne",  Op_0 },
    { "repnz",  Op_0 },
    { "repz",   Op_0 },
    { "ret",    Op_ret },
    { "retf",   Op_retf },
    { "rol",    Op_Shift },
    { "ror",    Op_Shift },
    { "roundpd", Op_DstSrcImmS },
    { "roundps", Op_DstSrcImmS },
    { "roundsd", Op_DstSrcImmS },
    { "roundss", Op_DstSrcImmS },
    { "rsm",     Op_0 },
    { "rsqrtps", Op_DstSrcSSE },
    { "rsqrtss", Op_DstSrcSSE },
    { "sahf",   Op_Flags },
    { "sal",    Op_Shift },
    { "sar",    Op_Shift },
    { "sbb",    Op_UpdSrcF },
    { "scas",   Op_scas },
    { "scasb",  Op_scasX },
    { "scasd",  Op_scasX },
    { "scasw",  Op_scasX },
    { "seta",   Op_DstRMBNT }, // also gpr8
    { "setae",  Op_DstRMBNT },
    { "setb",   Op_DstRMBNT },
    { "setbe",  Op_DstRMBNT },
    { "setc",   Op_DstRMBNT },
    { "sete",   Op_DstRMBNT },
    { "setg",   Op_DstRMBNT },
    { "setge",  Op_DstRMBNT },
    { "setl",   Op_DstRMBNT },
    { "setle",  Op_DstRMBNT },
    { "setna",  Op_DstRMBNT },
    { "setnae", Op_DstRMBNT },
    { "setnb",  Op_DstRMBNT },
    { "setnbe", Op_DstRMBNT },
    { "setnc",  Op_DstRMBNT },
    { "setne",  Op_DstRMBNT },
    { "setng",  Op_DstRMBNT },
    { "setnge", Op_DstRMBNT },
    { "setnl",  Op_DstRMBNT },
    { "setnle", Op_DstRMBNT },
    { "setno",  Op_DstRMBNT },
    { "setnp",  Op_DstRMBNT },
    { "setns",  Op_DstRMBNT },
    { "setnz",  Op_DstRMBNT },
    { "seto",   Op_DstRMBNT },
    { "setp",   Op_DstRMBNT },
    { "setpe",  Op_DstRMBNT },
    { "setpo",  Op_DstRMBNT },
    { "sets",   Op_DstRMBNT },
    { "setz",   Op_DstRMBNT },
    { "sfence", Op_0 },
    { "sgdt",   Op_DstMemNT },
    { "shl",    Op_Shift },
    { "shld",   Op_UpdSrcShft },
    { "shr",    Op_Shift },
    { "shrd",   Op_UpdSrcShft },
    { "shufpd", Op_DstSrcImmS },
    { "shufps", Op_DstSrcImmS },
    { "sidt",   Op_DstMemNT },
    { "sldt",   Op_DstRMWNT },
    { "smsw",   Op_DstRMWNT },
    { "sqrtpd", Op_DstSrcSSE },
    { "sqrtps", Op_DstSrcSSE },
    { "sqrtsd", Op_DstSrcSSE },
    { "sqrtss", Op_DstSrcSSE },
    { "stc",    Op_Flags },
    { "std",    Op_Flags },
    { "sti",    Op_Flags },
    { "stmxcsr",Op_DstMemNT },
    { "stos",   Op_stos },
    { "stosb",  Op_stosX },
    { "stosd",  Op_stosX },
    { "stosw",  Op_stosX },
    { "str",    Op_DstMemNT }, // also r16
    { "sub",    Op_UpdSrcF },
    { "subpd",  Op_DstSrcSSE },
    { "subps",  Op_DstSrcSSE },
    { "subsd",  Op_DstSrcSSE },
    { "subss",  Op_DstSrcSSE },
    { "sysenter",Op_0 },
    { "sysexit", Op_0 },
    { "test",    Op_SrcSrcF },
    { "ucomisd", Op_SrcSrcSSEF },
    { "ucomiss", Op_SrcSrcSSEF },
    { "ud2",     Op_0 },
    { "unpckhpd", Op_DstSrcSSE },
    { "unpckhps", Op_DstSrcSSE },
    { "unpcklpd", Op_DstSrcSSE },
    { "unpcklps", Op_DstSrcSSE },
    { "verr",   Op_SrcMemNTF },
    { "verw",   Op_SrcMemNTF },
    { "wait",   Op_0 },
    { "wbinvd", Op_0 },
    { "wrmsr",  Op_0 },
    { "xadd",   Op_UpdUpdF },
    { "xchg",   Op_UpdUpd },
    { "xlat",   Op_xlat },
    { "xlatb",  Op_0_AX },
    { "xor",    Op_DstSrcF },
    { "xorpd",  Op_DstSrcSSE },
    { "xorps",  Op_DstSrcSSE },
};

static const AsmOpEnt opData64[] =
{
    { "aaa",    Op_Invalid },
    { "aad",    Op_Invalid },
    { "aam",    Op_Invalid },
    { "aas",    Op_Invalid },
    { "addq",   Op_DstSrcSSE },
    { "align",  Op_Invalid },
    { "arpl",   Op_Invalid },
    { "bound",  Op_Invalid },
    { "callf",  Op_Branch },
    { "cdqe",   Op_0_DXAX },
    { "cqo",    Op_0_DXAX },
    { "cmpq",   Op_DstSrcNT },
    { "cmpsq",  Op_cmpsX },
    { "cmpxchg16b", Op_cmpxchg8b }, // m128 operand
    { "cwde",   Op_0_DXAX },
    { "daa",    Op_Invalid },
    { "das",    Op_Invalid },
    { "into",   Op_Invalid },
    { "iretq",  Op_iretq },
    { "lds",    Op_Invalid },
    { "les",    Op_Invalid },
    { "jmpe",   Op_Branch },
    { "jmpf",   Op_Branch },
    { "jrcxz",  Op_CBranch },
    { "leaq",   Op_DstSrcSSE },  // "
    { "lodsq",  Op_lodsX },
    { "movb",   Op_DstSrcNT  },
    { "movl",   Op_DstSrc },
    { "movsq",  Op_movsd },
    { "movsxd", Op_movsx },
    { "movzbl", Op_DstSrcNT  },
    { "pabsq",  Op_DstSrcSSE },
    { "pause",   Op_DstSrcMMX },
    { "pop",    Op_DstQ },
    { "popa",   Op_Invalid },
    { "popad",  Op_Invalid },
    { "popfd",  Op_Invalid },
    { "popfq",  Op_SizedStack },
    { "popq",   Op_DstQ },
    { "push",   Op_pushq },
    { "pusha",  Op_Invalid },
    { "pushad", Op_Invalid },
    { "pushfd", Op_Invalid },
    { "pushfq", Op_SizedStack },
    { "pushq",  Op_pushq },
    { "retn",   Op_retf },
    { "rsm",    Op_Invalid },
    { "salq",   Op_DstSrcNT  },
    { "scasq",  Op_scasX },
    { "stosq",  Op_stosX },
    { "subq",   Op_DstSrcSSE },
    { "syscall", Op_0 },
    { "sysret",  Op_0 },
    { "swapgs", Op_0 },
    { "sysretq",Op_0 },
    { "xorq",   Op_DstSrcNT },
};

enum PtrType
{
  Default_Ptr = 0,
  Byte_Ptr = 1,
  Short_Ptr = 2,
  Int_Ptr = 4,
  Long_Ptr =  8,
  Float_Ptr = 4,
  Double_Ptr = 8,
  Extended_Ptr = 10,
  Near_Ptr = 98,
  Far_Ptr = 99,
  N_PtrTypes
};

static const int N_PtrNames = 8;
static const char * ptrTypeNameTable[N_PtrNames] =
{
  "word", "dword", "qword",
  "float", "double", "extended",
  "near",  "far"
};

static Identifier * ptrTypeIdentTable[N_PtrNames];
static const PtrType ptrTypeValueTable[N_PtrNames] =
{
  Short_Ptr, Int_Ptr, Long_Ptr,
  Float_Ptr, Double_Ptr, Extended_Ptr,
  Near_Ptr, Far_Ptr
};

enum OperandClass
{
  Opr_Invalid,
  Opr_Immediate,
  Opr_Reg,
  Opr_Mem
};

/* kill inlining if we reference a local? */

/* DMD seems to allow only one 'symbol' per operand .. include __LOCAL_SIZE */

/* DMD offset usage: <parm>[<reg>] seems to always be relative to EBP+8 .. even
   if naked.. */

// mov eax, 4
// mov eax, fs:4
// -- have to assume we know wheter or not to use '$'

struct AsmProcessor
{
  struct Operand {
      int inBracket;
      int hasBracket;
      int hasNumber;
      int isOffset;
      
      Reg segmentPrefix;
      Reg reg;
      sinteger_t constDisplacement; // use to build up.. should be int constant in the end..
      Expressions symbolDisplacement;
      Reg baseReg;
      Reg indexReg;
      int scale;
      
      OperandClass cls;
      PtrType dataSize;
      PtrType dataSizeHint; // DMD can use the type of a referenced variable
  };
  
  static const unsigned Max_Operands = 3;
  
  AsmStatement * stmt;
  Scope * sc;
  
  Token * token;
  OutBuffer * insnTemplate;
  
  AsmOp       op;
  const AsmOpInfo * opInfo;
  Operand operands[Max_Operands];
  Identifier * opIdent;
  Operand * operand;
  
  AsmProcessor(Scope * sc, AsmStatement * stmt);
  
  void run();
  void nextToken();
  Token * peekToken();
  void expectEnd();
  void parse();
  
  AsmOp searchOpdata(Identifier * opIdent, const AsmOpEnt * opdata, int nents);
  AsmOp parseOpcode();
  void doInstruction();
  void setAsmCode();
  bool matchOperands(unsigned nOperands);
  void addOperand(const char * fmt, AsmArgType type, Expression * e,
		  AsmCode * asmcode, AsmArgMode mode = Mode_Input);
  
  void addLabel(unsigned n);
  void classifyOperand(Operand * operand);
  OperandClass classifyOperand1(Operand * operand);
  void writeReg(Reg reg);
  bool opTakesLabel();
  bool getTypeChar(TypeNeeded needed, PtrType ptrtype, char & type_char);
  bool formatInstruction(int nOperands, AsmCode * asmcode);
  
  bool isIntExp(Expression * exp);
  bool isRegExp(Expression * exp);
  bool isLocalSize(Expression * exp);
  bool isDollar(Expression * exp);
  PtrType isPtrType(Token * tok);
  
  bool tryScale(Expression * e1, Expression * e2);
  
  Expression * newRegExp(int regno);
  Expression * newIntExp(sinteger_t v /* %% type */);
  void slotExp(Expression * exp);
  void invalidExpression();
  Expression * intOp(TOK op, Expression * e1, Expression * e2);
  
  void parseOperand();
  Expression * parseAsmExp();
  Expression * parseShiftExp();
  Expression * parseAddExp();
  Expression * parseMultExp();
  Expression * parseBrExp();
  Expression * parseUnaExp();
  Expression * parsePrimaryExp();
  
  void doAlign();
  void doEven();
  void doNaked();
  void doData();
};

