// asmstmt.cc -- D frontend for GCC.
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

#include "d-gcc-includes.h"
#include "statement.h"
#include "scope.h"
#include "id.h"

#include "d-lang.h"
#include "d-codegen.h"

enum AsmArgType
{
  Arg_Integer,
  Arg_Pointer,
  Arg_Memory,
  Arg_FrameRelative,
  Arg_LocalSize,
  Arg_Label,
  Arg_Dollar
};

enum AsmArgMode
{
  Mode_Input,
  Mode_Output,
  Mode_Update
};

struct AsmArg
{
  AsmArgType   type;
  Expression * expr;
  AsmArgMode   mode;

  AsmArg (AsmArgType type, Expression * expr, AsmArgMode mode)
    {
      this->type = type;
      this->expr = expr;
      this->mode = mode;
    }
};

typedef ArrayBase<struct AsmArg> AsmArgs;


struct AsmCode
{
  char *   insnTemplate;
  unsigned insnTemplateLen;
  AsmArgs args;
  unsigned * clbregs; // list of clobbered registers
  unsigned dollarLabel;
  int      clobbersMemory;

  AsmCode (unsigned n_regs)
    {
      insnTemplate = NULL;
      insnTemplateLen = 0;
      clbregs = new unsigned[n_regs];
      memset (clbregs, 0, sizeof (unsigned)*n_regs);
      dollarLabel = 0;
      clobbersMemory = 0;
    }
};

#ifdef TARGET_80387
#include "d-asm-i386.h"
#else
#define D_NO_INLINE_ASM_AT_ALL
#endif

/* Apple GCC extends ASM_EXPR to five operands; cannot use build4. */
tree
d_build_asm_stmt (tree t1, tree t2, tree t3, tree t4, tree t5)
{
  tree t = make_node (ASM_EXPR);
  TREE_TYPE (t) = void_type_node;
  SET_EXPR_LOCATION (t, input_location);
  TREE_OPERAND (t,0) = t1;     // STRING
  TREE_OPERAND (t,1) = t2;     // OUTPUTS
  TREE_OPERAND (t,2) = t3;     // INPUTS
  TREE_OPERAND (t,3) = t4;     // CLOBBERS
  TREE_OPERAND (t,4) = t5;     // LABELS
  TREE_SIDE_EFFECTS (t) = 1;
  return t;
}

static bool
getFrameRelativeValue (tree decl, HOST_WIDE_INT * result)
{
  /* Using this instead of DECL_RTL for struct args seems like a
     good way to get hit by a truck because it may not agree with
     non-asm access, but asm code wouldn't know what GCC does anyway. */
  rtx r = NULL_RTX;

  if (TREE_CODE (decl) == PARM_DECL)
    {
      r = DECL_INCOMING_RTL (decl);
    }
  else
    {
      // Local variables don't have DECL_INCOMING_RTL
      if (TREE_STATIC (decl) || TREE_PUBLIC (decl))
	r = DECL_RTL (decl);
    }

  if (r != NULL_RTX && GET_CODE (r) == MEM /* && r->frame_related */)
    {
      r = XEXP (r, 0);
      if (GET_CODE (r) == PLUS)
	{
	  rtx e1 = XEXP (r, 0);
	  rtx e2 = XEXP (r, 1);
	  if (e1 == virtual_incoming_args_rtx && GET_CODE (e2) == CONST_INT)
	    {
	      *result = INTVAL (e2) + 8; // %% 8 is 32-bit specific...
	      return true;
	    }
	  else if (e1 == virtual_stack_vars_rtx && GET_CODE (e2) == CONST_INT)
	    {
	      *result = INTVAL (e2); // %% 8 is 32-bit specific...
	      return true;
	    }
	}
      else if (r == virtual_incoming_args_rtx)
	{
	  *result = 8;
	  return true; // %% same as above
	}
      // shouldn't have virtual_stack_vars_rtx by itself
    }

  return false;
}

/* GCC does not support jumps from asm statements.  When optimization
   is turned on, labels referenced only from asm statements will not
   be output at the correct location.  There are ways around this:

   1) Reference the label with a reachable goto statement
   2) Have reachable computed goto in the function
   3) Hack cfgbuild.c to act as though there is a computed goto.

   These are all pretty bad, but if would be nice to be able to tell
   GCC not to optimize in this case (even on per label/block basis).

   The current solution is output our own private labels (as asm
   statements) along with the "real" label.  If the label happens to
   be referred to by a goto statement, the "real" label will also be
   output in the correct location.

   Also had to add 'asmLabelNum' to LabelDsymbol to indicate it needs
   special processing.

   (junk) d-lang.cc:916:case LABEL_DECL:
   C doesn't do this.  D needs this for referencing labels in
   inline assembler since there may be not goto referencing it.

   ------------------------------
   %% Fix for GCC-4.5+
   GCC now accepts a 5th operand, ASM_LABELS.
   If present, this indicates various destinations for the asm expr,
   this in turn is used in tree-cfg.c to determine whether or not a
   expr may affect the flowgraph analysis by jumping to said label.

   For prior versions of gcc, this requires a backpatch.

   %% TODO: Add support for label operands in GDC Extended Assembler.

*/

static unsigned d_priv_asm_label_serial = 0;

// may need to make this target-specific
static void
d_format_priv_asm_label (char * buf, unsigned n)
{
  //ASM_GENERATE_INTERNAL_LABEL (buf, "LDASM", n);//inserts a '*' for use with assemble_name
  sprintf (buf, ".LDASM%u", n);
}

void
d_expand_priv_asm_label (IRState * irs, unsigned n)
{
  char buf[64];
  d_format_priv_asm_label (buf, n);
  strcat (buf, ":");
  tree insnt = build_string (strlen (buf), buf);
  tree t = d_build_asm_stmt (insnt, NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
  ASM_VOLATILE_P (t) = 1;
  ASM_INPUT_P (t) = 1; // what is this doing?
  irs->addExp (t);
}

ExtAsmStatement::ExtAsmStatement (Loc loc, Expression *insnTemplate, Expressions *args, Identifiers *argNames,
			      	  Expressions *argConstraints, int nOutputArgs, Expressions *clobbers)
    : Statement (loc)
{
  this->insnTemplate = insnTemplate;
  this->args = args;
  this->argNames = argNames;
  this->argConstraints = argConstraints;
  this->nOutputArgs = nOutputArgs;
  this->clobbers = clobbers;
}

Statement *
ExtAsmStatement::syntaxCopy ()
{
  /* insnTemplate, argConstraints, and clobbers would be
     semantically static in GNU C. */
  Expression *insnTemplate = this->insnTemplate->syntaxCopy ();
  Expressions * args = Expression::arraySyntaxCopy (this->args);
  // argNames is an array of identifiers
  Expressions * argConstraints = Expression::arraySyntaxCopy (this->argConstraints);
  Expressions * clobbers = Expression::arraySyntaxCopy (this->clobbers);
  return new ExtAsmStatement (loc, insnTemplate, args, argNames,
			      argConstraints, nOutputArgs, clobbers);
}

Statement *
ExtAsmStatement::semantic (Scope *sc)
{
  insnTemplate = insnTemplate->semantic (sc);
  insnTemplate = insnTemplate->optimize (WANTvalue);
#if V2
  if (sc->func)
    {
      if (sc->func->setUnsafe ())
	error ("extended assembler not allowed in @safe function %s", sc->func->toChars ());
    }
#endif
  if (insnTemplate->op != TOKstring || ((StringExp *)insnTemplate)->sz != 1)
    error ("instruction template must be a constant char string");
  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Expression * e = args->tdata()[i];
	  e = e->semantic (sc);
	  if (i < nOutputArgs)
	    e = e->modifiableLvalue (sc, NULL);
	  else
	    e = e->optimize (WANTvalue);
	  args->tdata()[i] = e;

	  e = argConstraints->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("constraint must be a constant char string");
	  argConstraints->tdata()[i] = e;
	}
    }
  if (clobbers)
    {
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * e = clobbers->tdata()[i];
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue);
	  if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
	    error ("clobber specification must be a constant char string");
	  clobbers->tdata()[i] = e;
	}
    }
  return this;
}

void
ExtAsmStatement::toCBuffer (OutBuffer *buf)
{
  buf->writestring ("gcc asm { ");
  if (insnTemplate)
    buf->writestring (insnTemplate->toChars ());
  buf->writestring (" : ");
  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Identifier * name = argNames->tdata()[i];
	  Expression * constr = argConstraints->tdata()[i];
	  Expression * arg = args->tdata()[i];

	  if (name)
	    {
	      buf->writestring ("[");
	      buf->writestring (name->toChars ());
	      buf->writestring ("] ");
	    }
	  if (constr)
	    {
	      buf->writestring (constr->toChars ());
	      buf->writestring (" ");
	    }
	  if (arg)
	    {
	      buf->writestring (arg->toChars ());
	    }

	  if (i < nOutputArgs - 1)
	    buf->writestring (", ");
	  else if (i == nOutputArgs - 1)
	    buf->writestring (" : ");
	  else if (i < args->dim - 1)
	    buf->writestring (", ");
	}
    }
  if (clobbers)
    {
      buf->writestring (" : ");
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * clobber = clobbers->tdata()[i];
	  buf->writestring (clobber->toChars ());
	  if (i < clobbers->dim - 1)
	    buf->writestring (", ");
	}
    }
  buf->writestring ("; }");
  buf->writenl ();
}

int
ExtAsmStatement::blockExit (bool mustNotThrow)
{
  if (mustNotThrow)
    error ("asm statements are assumed to throw", toChars ());
  // TODO: Be smarter about this
  return BEany;
}


// StringExp::toIR usually adds a NULL.  We don't want that...

static tree
naturalString (Expression * e)
{
  // don't fail, just an error?
  gcc_assert (e->op == TOKstring);
  StringExp * s = (StringExp *) e;
  gcc_assert (s->sz == 1);
  return build_string (s->len, (char *) s->string);
}

void
ExtAsmStatement::toIR (IRState *irs)
{
  ListMaker outputs;
  ListMaker inputs;
  ListMaker tree_clobbers;

  gen.doLineNote (loc);

  if (args)
    {
      for (size_t i = 0; i < args->dim; i++)
	{
	  Identifier * name = argNames->tdata()[i];
	  Expression * constr = argConstraints->tdata()[i];
	  tree p = tree_cons (name ? build_string (name->len, name->string) : NULL_TREE,
	      		      naturalString (constr), NULL_TREE);
	  tree v = (args->tdata()[i])->toElem (irs);

	  if (i < nOutputArgs)
	    outputs.cons (p, v);
	  else
	    inputs.cons (p, v);
	}
    }
  if (clobbers)
    {
      for (size_t i = 0; i < clobbers->dim; i++)
	{
	  Expression * clobber = clobbers->tdata()[i];
	  tree_clobbers.cons (NULL_TREE, naturalString (clobber));
	}
    }
  irs->doAsm (naturalString (insnTemplate), outputs.head, inputs.head, tree_clobbers.head);
}


#ifndef D_NO_INLINE_ASM_AT_ALL

bool d_have_inline_asm () { return true; }

Statement *
AsmStatement::semantic (Scope *sc)
{
#if V2
  if (sc->func && sc->func->isSafe ())
    error ("inline assembler not allowed in @safe function %s", sc->func->toChars ());
#endif
  sc->func->inlineStatusExp = ILSno; // %% not sure
  sc->func->inlineStatusStmt = ILSno; // %% not sure
  // %% need to set DECL_UNINLINABLE too?

#if V1
  // DMD assumes that inline assembly sets return argument.
  // This avoids "missing return expression" assertion.
  FuncDeclaration *fd = sc->parent->isFuncDeclaration ();
  gcc_assert (fd);
  fd->inlineAsm = 1;
#endif

  sc->func->hasReturnExp |= 8; // %% DMD does this, apparently...

  // empty statement -- still do the above things because they might be expected?
  if (! tokens)
    return this;

  AsmProcessor ap (sc, this);
  ap.run ();
  return this;
}

void
AsmStatement::toIR (IRState * irs)
{
  gen.doLineNote (loc);

  if (! asmcode)
    return;

  static tree i_cns = 0;
  static tree p_cns = 0;
  static tree m_cns = 0;
  static tree mw_cns = 0;
  static tree mrw_cns = 0;
  static tree memory_name = 0;

  if (! i_cns)
    {
      i_cns = build_string (1, "i");
      p_cns = build_string (1, "p");
      m_cns = build_string (1, "m");
      mw_cns  = build_string (2, "=m");
      mrw_cns = build_string (2, "+m");
      memory_name = build_string (6, "memory");
      d_keep (i_cns);
      d_keep (p_cns);
      d_keep (m_cns);
    }

  AsmCode * code = (AsmCode *) asmcode;
  ListMaker inputs;
  ListMaker outputs;
  ListMaker clobbers;
  ListMaker labels;
  HOST_WIDE_INT var_frame_offset; // "frame_offset" is a macro
  bool clobbers_mem = code->clobbersMemory;
  int input_idx = 0;
  int n_outputs = 0;
  int arg_map[10];

  gcc_assert (code->args.dim <= 10);

  for (size_t i = 0; i < code->args.dim; i++)
    {
      AsmArg * arg = code->args[i];

      bool is_input = true;
      TOK arg_op = arg->expr->op;
      tree arg_val = NULL_TREE;
      tree cns = NULL_TREE;

      switch (arg->type)
	{
	case Arg_Integer:
	  arg_val = arg->expr->toElem (irs);
	  cns = i_cns;
	  break;
	case Arg_Pointer:
	  if (arg_op == TOKvar)
	    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
	  else if (arg_op == TOKdsymbol)
	    {
	      arg_val = irs->getLabelTree((LabelDsymbol *) ((DsymbolExp *) arg->expr)->s);
	      labels.cons (NULL_TREE, arg_val);
	    }
	  else
	    arg_val = arg->expr->toElem (irs);

	  if (arg_op != TOKaddress && arg_op != TOKsymoff && arg_op != TOKadd)
	    arg_val = irs->addressOf (arg_val);

	  cns = p_cns;
	  break;
	case Arg_Memory:
	  if (arg_op == TOKvar)
	    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
	  else if (arg_op == TOKfloat64)
	    {
	      /* Constant scalar value.  In order to reference it as memory,
		 create an anonymous static var. */
	      tree cnst = build_decl (UNKNOWN_LOCATION, VAR_DECL, NULL_TREE,
				      arg->expr->type->toCtype ());
	      g.ofile->giveDeclUniqueName (cnst);
	      DECL_INITIAL (cnst) = arg->expr->toElem (irs);
	      TREE_STATIC (cnst) = 1;
	      TREE_CONSTANT (cnst) = 1;
	      TREE_READONLY (cnst) = 1;
	      TREE_PRIVATE (cnst) = 1;
	      DECL_ARTIFICIAL (cnst) = 1;
	      DECL_IGNORED_P (cnst) = 1;
	      g.ofile->rodc (cnst, 1);
	      arg_val = cnst;
	    }
	  else
	    arg_val = arg->expr->toElem (irs);
	  if (DECL_P (arg_val))
	    TREE_ADDRESSABLE (arg_val) = 1;
	  switch (arg->mode)
	    {
	    case Mode_Input:  cns = m_cns; break;
	    case Mode_Output: cns = mw_cns;  is_input = false; break;
	    case Mode_Update: cns = mrw_cns; is_input = false; break;
	    default: gcc_unreachable (); break;
	    }
	  break;
	case Arg_FrameRelative:
	  if (arg_op == TOKvar)
	    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
	  else
	    gcc_unreachable ();
	  if (getFrameRelativeValue (arg_val, & var_frame_offset))
	    {
	      arg_val = irs->integerConstant (var_frame_offset);
	      cns = i_cns;
	    }
	  else
	    {
	      this->error ("%s", "argument not frame relative");
	      return;
	    }
	  if (arg->mode != Mode_Input)
	    clobbers_mem = true;
	  break;
	case Arg_LocalSize:
	  var_frame_offset = frame_offset;
	  if (var_frame_offset < 0)
	    var_frame_offset = - var_frame_offset;
	  arg_val = irs->integerConstant (var_frame_offset);
	  cns = i_cns;
	  break;
	case Arg_Label:
	  /* Just add label, no further processing needed.  */
	  arg_val = irs->getLabelTree((LabelDsymbol *) ((DsymbolExp *) arg->expr)->s);
	  labels.cons (NULL_TREE, arg_val);
	  continue;
	default:
	  gcc_unreachable ();
	}

      if (is_input)
	{
	  arg_map[i] = --input_idx;
	  inputs.cons (tree_cons (NULL_TREE, cns, NULL_TREE), arg_val);
	}
      else
	{
	  arg_map[i] = n_outputs++;
	  outputs.cons (tree_cons (NULL_TREE, cns, NULL_TREE), arg_val);
	}
    }

  // Telling GCC that callee-saved registers are clobbered makes it preserve
  // those registers.   This changes the stack from what a naked function
  // expects.

  if (! irs->func->naked)
    {
      for (size_t i = 0; i < (size_t) N_Regs; i++)
	{
	  if (code->clbregs[i])
	    clobbers.cons (NULL_TREE, regInfo[i].gccName);
	}
      if (clobbers_mem)
	clobbers.cons (NULL_TREE, memory_name);
    }

  // Remap argument numbers
  for (size_t i = 0; i < code->args.dim; i++)
    {
      if (arg_map[i] < 0)
	arg_map[i] = -arg_map[i] - 1 + n_outputs;
    }

  bool pct = false;
  char * p = code->insnTemplate;
  char * q = p + code->insnTemplateLen;
  //printf ("start: %.*s\n", code->insnTemplateLen, code->insnTemplate);
  while (p < q)
    {
      if (pct)
	{
	  if (*p >= '0' && *p <= '9')
	    {
	      // %% doesn't check against nargs
	      *p = '0' + arg_map[*p - '0'];
	      pct = false;
	    }
	  else if (*p == '%')
	    {
	      pct = false;
	    }
	  //gcc_assert (*p == '%');// could be 'a', etc. so forget it..
	}
      else if (*p == '%')
	{
	  pct = true;
	}
      ++p;
    }

  //printf ("final: %.*s\n", code->insnTemplateLen, code->insnTemplate);
  tree insnt = build_string (code->insnTemplateLen, code->insnTemplate);
  tree t = d_build_asm_stmt (insnt, outputs.head, inputs.head, clobbers.head, labels.head);
  ASM_VOLATILE_P (t) = 1;
  irs->addExp (t);
  if (code->dollarLabel)
    d_expand_priv_asm_label (irs, code->dollarLabel);
}


// D inline asm implementation.
static Token eof_tok;
static Expression * Handled;
static Identifier * ident_seg;

AsmProcessor::AsmProcessor (Scope * sc, AsmStatement * stmt)
{
  this->sc = sc;
  this->stmt = stmt;
  token = stmt->tokens;
  insnTemplate = new OutBuffer;

  opInfo = NULL;

  if (! regInfo[0].ident)
    {
      char buf[8], *p;

      for (size_t i = 0; i < (size_t) N_Regs; i++)
	{
	  strncpy (buf, regInfo[i].name, sizeof (buf) - 1);
	  for (p = buf; *p; p++)
	    *p = TOLOWER (*p);
	  regInfo[i].gccName = build_string (p - buf, buf);
	  d_keep (regInfo[i].gccName);
	  if ((i <= Reg_ST || i > Reg_ST7) && i != Reg_EFLAGS)
	    regInfo[i].ident = Lexer::idPool (regInfo[i].name);
	}

      for (size_t i = 0; i < (size_t) N_PtrNames; i++)
	ptrTypeIdentTable[i] = Lexer::idPool (ptrTypeNameTable[i]);

      Handled = new Expression (0, TOKvoid, sizeof (Expression));

      ident_seg = Lexer::idPool ("seg");

      eof_tok.value = TOKeof;
      eof_tok.next = 0;
    }
}

void
AsmProcessor::run ()
{
  parse ();
}

void
AsmProcessor::nextToken ()
{
  if (token->next)
    token = token->next;
  else
    token = & eof_tok;
}

Token *
AsmProcessor::peekToken ()
{
  if (token->next)
    return token->next;

  return & eof_tok;
}

void
AsmProcessor::expectEnd ()
{
  if (token->value != TOKeof)
    stmt->error ("expected end of statement"); // %% extra at end...
}

void
AsmProcessor::parse ()
{
  op = parseOpcode ();

  switch (op)
    {
    case Op_Align:
      doAlign ();
      expectEnd ();
      break;

    case Op_Even:
      doEven ();
      expectEnd ();
      break;

    case Op_Naked:
      doNaked ();
      expectEnd ();
      break;

    case Op_Invalid:
      stmt->error ("unknown opcode '%s'", opIdent->string);
      break;

    default:
      if (op >= Op_db && op <= Op_de)
	doData ();
      else
	doInstruction ();
    }
}

AsmOp
AsmProcessor::searchOpdata (Identifier * opIdent, const AsmOpEnt * opdata, int nents)
{
  int low = 0, high = nents;
  do
    {
      // %% okay to use bsearch?
      int pos = (low + high) / 2;
      int cmp = strcmp (opIdent->string, opdata[pos].inMnemonic);
      if (! cmp)
	return opdata[pos].asmOp;
      else if (cmp < 0)
	high = pos;
      else
	low = pos + 1;
    } while (low != high);

  return Op_Invalid;
}

AsmOp
AsmProcessor::parseOpcode ()
{
  switch (token->value)
    {
    case TOKalign:
      nextToken ();
      return Op_Align;

    case TOKin:
      nextToken ();
      opIdent = Id::___in;
      return Op_in;

    case TOKint32: // "int"
      nextToken ();
      opIdent = Id::__int;
      return Op_SrcImm;

    case TOKout:
      nextToken ();
      opIdent = Id::___out;
      return Op_out;

    case TOKidentifier:
      // search for mnemonic below
      break;

    default:
      stmt->error ("expected opcode");
      return Op_Invalid;
    }

  opIdent = token->ident;
  nextToken ();

  static const int N_ents = sizeof (opData)/sizeof (AsmOpEnt);
  static const int N_ents64 = sizeof (opData64)/sizeof (AsmOpEnt);

  // Search 64bit opcodes first.
  if (global.params.is64bit)
    {
      AsmOp op = searchOpdata (opIdent, opData64, N_ents64);
      if (op != Op_Invalid)
	return op;
    }

  return searchOpdata (opIdent, opData, N_ents);
}

// need clobber information.. use information is good too...
void
AsmProcessor::doInstruction ()
{
  unsigned operand_i = 0;

  opInfo = & asmOpInfo[op];
  memset (operands, 0, sizeof (operands));

  if (token->value == TOKeof && op == Op_FMath0)
    return;

  while (token->value != TOKeof)
    {
      if (operand_i < Max_Operands)
	{
	  operand = & operands[operand_i];
	  operand->reg = operand->baseReg = operand->indexReg =
	    operand->segmentPrefix = Reg_Invalid;
	  parseOperand ();
	  operand_i++;
	}
      else
	{
	  stmt->error ("too many operands for instruction");
	  break;
	}

      if (token->value == TOKcomma)
	{
	  nextToken ();
	}
      else if (token->value != TOKeof)
	{
	  stmt->error ("expected comma after operand");
	  return;
	}
    }

  if (matchOperands (operand_i))
    {
      AsmCode * asmcode = new AsmCode (N_Regs);

      if (formatInstruction (operand_i, asmcode))
	stmt->asmcode = (code *) asmcode;
    }
}

void
AsmProcessor::setAsmCode ()
{
  AsmCode * asmcode = new AsmCode (N_Regs);
  asmcode->insnTemplateLen = insnTemplate->offset;
  asmcode->insnTemplate = (char*) insnTemplate->extractData ();
  stmt->asmcode = (code*) asmcode;
}

// note: doesn't update AsmOp op
bool
AsmProcessor::matchOperands (unsigned nOperands)
{
  bool wrong_number = true;

  for (size_t i = 0; i < nOperands; i++)
    classifyOperand (& operands[i]);

  while (1)
    {
      if (nOperands == opInfo->nOperands ())
	{
	  wrong_number = false;
	  /*  Cases in which number of operands is not
	      enough for a match: Op_FCmp/Op_FCmp1,
	      Op_FCmpP/Op_FCmpP1 */
	  for (size_t i = 0; i < nOperands; i++)
	    {
	      Operand * operand = & operands[i];

	      if (opInfo->operands[i] & OprC_Mem) // no FPMem currently
		{
		  if (operand->cls != Opr_Mem)
		    goto no_match;
		}
	      else if (opInfo->operands[i] & OprC_RFP)
		{
		  if (! (operand->reg >= Reg_ST
			 && operand->reg <= Reg_ST7))
		    goto no_match;
		}
	    }
	  return true;
	}

    no_match:
      if (opInfo->linkType == Next_Form)
	opInfo = & asmOpInfo[ op = (AsmOp) opInfo->link ];
      else
	break;
    }

  if (wrong_number)
    stmt->error ("wrong number of operands");
  else
    stmt->error ("wrong operand types");

  return false;
}

void
AsmProcessor::addOperand (const char * fmt, AsmArgType type, Expression * e,
 			  AsmCode * asmcode, AsmArgMode mode)
{
  insnTemplate->writestring (fmt);
  insnTemplate->printf ("%d", asmcode->args.dim);
  asmcode->args.push (new AsmArg (type, e, mode));
}

void
AsmProcessor::addLabel (unsigned n)
{
  // No longer taking the address of the actual label -- doesn't seem like it would help.
  char buf[64];
  d_format_priv_asm_label (buf, n);
  insnTemplate->writestring (buf);
}

/* Determines whether the operand is a register, memory reference
   or immediate.  Immediate addresses are currently classified as
   memory.  This function is called before the exact instructions
   is known and thus, should not use opInfo. */
void
AsmProcessor::classifyOperand (Operand * operand)
{
  operand->cls = classifyOperand1 (operand);
}

OperandClass
AsmProcessor::classifyOperand1 (Operand * operand)
{
  bool is_localsize = false;
  bool really_have_symbol = false;

  if (operand->symbolDisplacement.dim)
    {
      is_localsize = isLocalSize (operand->symbolDisplacement[0]);
      really_have_symbol = ! is_localsize;
    }

  if (operand->isOffset && ! operand->hasBracket)
    return Opr_Immediate;

  if (operand->hasBracket || really_have_symbol)
    {
      // %% redo for 'offset' function
      if (operand->reg != Reg_Invalid)
	{
	  invalidExpression ();
	  return Opr_Invalid;
	}

      return Opr_Mem;
    }

  if (operand->reg != Reg_Invalid && operand->constDisplacement != 0)
    {
      invalidExpression ();
      return Opr_Invalid;
    }

  if (operand->segmentPrefix != Reg_Invalid)
    {
      if (operand->reg != Reg_Invalid)
	{
	  invalidExpression ();
	  return Opr_Invalid;
	}

      return Opr_Mem;
    }

  if (operand->reg != Reg_Invalid && ! operand->hasNumber)
    return Opr_Reg;

  // should check immediate given (operand->hasNumber);
  //
  if (operand->hasNumber || is_localsize)
    {
      // size determination not correct if there are symbols Opr_Immediate
      if (operand->dataSize == Default_Ptr)
	{
	  if (operand->constDisplacement < 0x100)
	    operand->dataSize = Byte_Ptr;
	  else if (operand->constDisplacement < 0x10000)
	    operand->dataSize = Short_Ptr;
	  else if (operand->constDisplacement < 0xFFFFFFFF)
	    operand->dataSize = Int_Ptr;
	  else
	    operand->dataSize = global.params.is64bit
	      ? Long_Ptr
	      : Int_Ptr;   // %% 32-bit overflow
	}

      return Opr_Immediate;
    }

  // probably a bug,?
  stmt->error ("invalid operand");
  return Opr_Invalid;
}

void
AsmProcessor::writeReg (Reg reg)
{
  insnTemplate->writestring ("%%");
  insnTemplate->write (TREE_STRING_POINTER (regInfo[reg].gccName),
		       TREE_STRING_LENGTH (regInfo[reg].gccName));
}

bool
AsmProcessor::opTakesLabel ()
{
  switch (op)
    {
    case Op_Branch:
    case Op_CBranch:
    case Op_Loop:
      return true;

    default:
      return false;
    }
}

bool
AsmProcessor::getTypeChar (TypeNeeded needed, PtrType ptrtype, char & type_char)
{
  switch (needed)
    {
    case Byte_NoType:
      return (ptrtype == Byte_Ptr);

    case Word_Types:
      if (ptrtype == Byte_Ptr)
	return false;
      // Drop through...

    case Int_Types:
      switch (ptrtype)
	{
	case Byte_Ptr:
	  type_char = 'b';
	  break;

	case Short_Ptr:
	  type_char = 'w';
	  break;

	case Int_Ptr:
	  type_char = 'l';
	  break;

	case Long_Ptr:
	  if (global.params.is64bit)
	    type_char = 'q';
	  else
	    type_char = 'l';
	  break;

	default:
	  return false;
	}
      break;

    case FPInt_Types:
      switch (ptrtype)
	{
	case Short_Ptr:
	  type_char = 0;
	  break;

	case Int_Ptr:
	case Long_Ptr:
	  type_char = 'l';
	  break;

	default:
	  return false;
	}
      break;

    case FP_Types:
      switch (ptrtype)
	{
	case Float_Ptr:
	  type_char = 's';
	  break;

	case Double_Ptr:
	  type_char = 'l';
	  break;

	case Extended_Ptr:
	  type_char = 't';
	  break;

	default:
	  return false;
	}
      break;

    default:
      return false;
    }

  return true;
}

// also set impl clobbers
bool
AsmProcessor::formatInstruction (int nOperands, AsmCode * asmcode)
{
  const char *fmt;
  const char *mnemonic;
  size_t mlen;
  char type_char = 0;
  bool use_star;
  AsmArgMode mode;
  PtrType op1_size;

  const bool is64bit = global.params.is64bit;

  insnTemplate = new OutBuffer;
  if (opInfo->linkType == Out_Mnemonic)
    mnemonic = alternateMnemonics[opInfo->link];
  else
    mnemonic = opIdent->string;

  // handle two-operand form where second arg is ignored.
  // must be done before type_char detection
  if (op == Op_FidR_P || op == Op_fxch || op == Op_FfdRR_P)
    {
      if (operands[1].cls == Opr_Reg && operands[1].reg == Reg_ST)
	nOperands = 1;
      else
	stmt->error ("instruction allows only ST as second argument");
    }

  if (opInfo->needsType)
    {
      PtrType exact_type = Default_Ptr;
      PtrType min_type = Default_Ptr;
      PtrType hint_type = Default_Ptr;

      /* Default types: This attempts to match the observed behavior of DMD */
      switch (opInfo->needsType)
	{
	case Int_Types:
	  min_type = Byte_Ptr;
	  break;

	case Word_Types:
	  min_type = Short_Ptr;
	  break;

	case FPInt_Types:
	  if (op == Op_Fis_ST) // integer math instructions
	    min_type = Int_Ptr;
	  else // compare, load, store
	    min_type = Short_Ptr;
	  break;

	case FP_Types:
	  min_type = Float_Ptr;
	  break;
	}
      if (operands[0].cls == Opr_Immediate)
	{
	  if (op == Op_push)
	    min_type = Int_Ptr;
	  else if (op == Op_pushq || op == Op_DstQ)
	    min_type = Long_Ptr;
	}

      for (size_t i = 0; i < (size_t) nOperands; i++)
	{
	  if (hint_type == Default_Ptr &&
	      ! (opInfo->operands[i] & Opr_NoType))
	    {
	      hint_type = operands[i].dataSizeHint;
	    }

	  if ((opInfo->operands[i] & Opr_NoType) ||
	      operands[i].dataSize == Default_Ptr)
	    {
	      continue;
	    }

	  if (operands[i].cls == Opr_Immediate)
	    {
	      min_type = operands[i].dataSize > min_type ?
		operands[i].dataSize : min_type;
	    }
	  else
	    {
	      exact_type = operands[i].dataSize; // could check for conflicting types
	      break;
	    }
	}

      bool type_ok = false;
      if (exact_type == Default_Ptr)
	{
	  type_ok = getTypeChar((TypeNeeded) opInfo->needsType, hint_type, type_char);
	  if (! type_ok)
	    type_ok = getTypeChar((TypeNeeded) opInfo->needsType, min_type, type_char);
	}
      else
	{
	  type_ok = getTypeChar((TypeNeeded) opInfo->needsType, exact_type, type_char);
	}

      if (! type_ok)
	{
	  stmt->error ("invalid operand size");
	  return false;
	}
    }
  else if (op == Op_Branch)
    {
      if (operands[0].dataSize == Far_Ptr) // %% type=Far_Ptr not set by Seg:Ofss OTOH, we don't support that..
	insnTemplate->writebyte ('l');
    }
  else if (op == Op_FMath0 || op == Op_FdST0ST1)
    {
      operands[0].cls = Opr_Reg;
      operands[0].reg = Reg_ST1;
      operands[1].cls = Opr_Reg;
      operands[1].reg = Reg_ST;
      nOperands = 2;
    }
  else if (op == Op_fxch)
    {
      // gas won't accept the two-operand form
      if (operands[1].cls == Opr_Reg && operands[1].reg == Reg_ST)
	{
	  nOperands = 1;
	}
      else
	{
	  stmt->error ("invalid operands");
	  return false;
	}
    }

  switch (op)
    {
    case Op_SizedStack:
      mlen = strlen (mnemonic);
      if (mnemonic[mlen-1] == 'd')
	insnTemplate->write (mnemonic, mlen-1);
      else
	{
	  insnTemplate->writestring (mnemonic);
	  insnTemplate->writebyte ('w');
	}
      break;

    case Op_cmpsd:
    case Op_insX:
    case Op_lodsX:
    case Op_movsd:
    case Op_outsX:
    case Op_scasX:
    case Op_stosX:
      mlen = strlen (mnemonic);
      if (mnemonic[mlen-1] == 'd')
	{
	  insnTemplate->write (mnemonic, mlen-1);
	  insnTemplate->writebyte ('l');
	}
      else
	{
	  insnTemplate->writestring (mnemonic);
	}
      break;

    case Op_movsx:
    case Op_movzx:
      mlen = strlen (mnemonic);
      op1_size = operands[1].dataSize;
      if (op1_size == Default_Ptr)
	op1_size = operands[1].dataSizeHint;
      // Need type char for source arg
      switch (op1_size)
	{
	case Byte_Ptr:
	case Default_Ptr:
	case Short_Ptr:
	  break;

	default:
	  stmt->error ("invalid operand size/type");
	  return false;
	}
      gcc_assert (type_char != 0);
      insnTemplate->write (mnemonic, mlen-1);
      insnTemplate->writebyte (op1_size == Short_Ptr ? 'w' : 'b');
      insnTemplate->writebyte (type_char);
      break;

    default:
      // special case for fdiv, fsub
      if ((strncmp (mnemonic, "fsub", 4) == 0 ||
	   strncmp (mnemonic, "fdiv", 4) == 0) &&
	  operands[0].reg != Reg_ST && op != Op_FMath)
	{
	  // replace:
	  //  f{sub,div}r{p,} <-> f{sub,div}{p,}
	  if (mnemonic[4] == 'r')
	    {
	      insnTemplate->write (mnemonic, 4);
	      insnTemplate->write (mnemonic + 5, strlen (mnemonic) - 5);
	    }
	  else
	    {
	      insnTemplate->write (mnemonic, 4);
	      insnTemplate->writebyte ('r');
	      insnTemplate->write (mnemonic + 4, strlen (mnemonic) - 4);
	    }
	}
      // special case for fild - accept 64 bit operand.
      else if (strcmp (mnemonic, "fild") == 0)
	{
	  op1_size = operands[0].dataSizeHint;
	  if (type_char == 'l' && op1_size == Long_Ptr)
	    type_char = 'q';
	  insnTemplate->writestring (mnemonic);
	}
      else
	{
	  insnTemplate->writestring (mnemonic);
	}

      // the no-operand versions of floating point ops always pop
      if (op == Op_FMath0)
	insnTemplate->writebyte ('p');

      if (type_char)
	insnTemplate->writebyte (type_char);
      break;
    }

  switch (opInfo->implicitClobbers & Clb_DXAX_Mask)
    {
    case Clb_SizeAX:
    case Clb_EAX:
      asmcode->clbregs[is64bit ? Reg_RAX : Reg_EAX] = 1;
      break;

    case Clb_SizeDXAX:
      asmcode->clbregs[is64bit ? Reg_RAX : Reg_EAX] = 1;
      if (type_char != 'b')
	asmcode->clbregs[is64bit ? Reg_RDX : Reg_EDX] = 1;
      break;

    default:
      break;  // nothing
    }

  if (opInfo->implicitClobbers & Clb_DI)
    asmcode->clbregs[is64bit ? Reg_RDI : Reg_EDI] = 1;
  if (opInfo->implicitClobbers & Clb_SI)
    asmcode->clbregs[is64bit ? Reg_RSI : Reg_ESI] = 1;
  if (opInfo->implicitClobbers & Clb_CX)
    asmcode->clbregs[is64bit ? Reg_RCX : Reg_ECX] = 1;
  if (opInfo->implicitClobbers & Clb_SP)
    asmcode->clbregs[is64bit ? Reg_RSP : Reg_ESP] = 1;
  if (opInfo->implicitClobbers & Clb_ST)
    {
      /* Can't figure out how to tell GCC that an
	 asm statement leaves an arg pushed on the stack.
	 Maybe if the statment had and input or output
	 operand it would work...  In any case, clobbering
	 all FP prevents incorrect code generation. */
      asmcode->clbregs[Reg_ST] = 1;
      asmcode->clbregs[Reg_ST1] = 1;
      asmcode->clbregs[Reg_ST2] = 1;
      asmcode->clbregs[Reg_ST3] = 1;
      asmcode->clbregs[Reg_ST4] = 1;
      asmcode->clbregs[Reg_ST5] = 1;
      asmcode->clbregs[Reg_ST6] = 1;
      asmcode->clbregs[Reg_ST7] = 1;
    }
  if (opInfo->implicitClobbers & Clb_Flags)
    asmcode->clbregs[Reg_EFLAGS] = 1;
  if (op == Op_cpuid)
    {
      asmcode->clbregs[Reg_EAX] = 1;
      asmcode->clbregs[Reg_EBX] = 1;
      asmcode->clbregs[Reg_ECX] = 1;
      asmcode->clbregs[Reg_EDX] = 1;
    }

  insnTemplate->writebyte (' ');
  for (size_t n = 0; n < (size_t) nOperands; n++)
    {
      int i;
      if (n != 0)
	insnTemplate->writestring (", ");

      fmt = "%";

      switch (op)
	{
	case Op_mul:
	  // gas won't accept the two-operand form; skip to the source operand
	  n = 1;
	  // drop through

	case Op_bound:
	case Op_enter:
	  i = n;
	  break;

	default:
	  i = nOperands - 1 - n; // operand = & operands[ nOperands - 1 - i ];
	  break;
	}
      operand = & operands[i];

      switch (operand->cls)
	{
	case Opr_Immediate:
	  // for implementing offset:
	  // $var + $7 // fails
	  // $var + 7  // ok
	  // $7 + $var // ok

	  // DMD doesn't seem to allow this
	  /*
	     if (opInfo->takesLabel ())  tho... (near ptr <Number> would be abs?)
	     fmt = "%a"; // GAS won't accept "jmp $42"; must be "jmp 42" (rel) or "jmp *42" (abs)
	     */
	  if (opTakesLabel ()/*opInfo->takesLabel ()*/)
	    {
	      // "relative addressing not allowed in branch instructions" ..
	      stmt->error ("integer constant not allowed in branch instructions");
	      return false;
	    }

	  if (operand->symbolDisplacement.dim &&
	      isLocalSize (operand->symbolDisplacement[0]))
	    {
	      // handle __LOCAL_SIZE, which in this constant, is an immediate
	      // should do this in slotexp..
	      addOperand ("%", Arg_LocalSize,
			  operand->symbolDisplacement[0], asmcode);
	      if (operand->constDisplacement)
		insnTemplate->writebyte ('+');
	      else
		break;
	    }

	  if (operand->symbolDisplacement.dim)
	    {
	      fmt = "%a";
	      addOperand ("%", Arg_Pointer,
			  operand->symbolDisplacement[0],
			  asmcode);

	      if (operand->constDisplacement)
		insnTemplate->writebyte ('+');
	      else
		// skip the addOperand (fmt, Arg_Integer...) below
		break;
	    }
	  addOperand (fmt, Arg_Integer, newIntExp (operand->constDisplacement), asmcode);
	  break;

	case Opr_Reg:
	  if (opInfo->operands[i] & Opr_Dest)
	    {
	      Reg clbr_reg = (Reg) regInfo[operand->reg].baseReg;
	      if (clbr_reg != Reg_Invalid)
		asmcode->clbregs[clbr_reg] = 1;
	    }

	  if (opTakesLabel ()/*opInfo->takesLabel ()*/)
	    insnTemplate->writebyte ('*');
	  writeReg (operand->reg);
	  /*
	     insnTemplate->writestring ("%%");
	     insnTemplate->writestring (regInfo[operand->reg].name);
	     */
	  break;

	case Opr_Mem:
	  // better: use output operands for simple variable references
	  if (opInfo->operands[i] & Opr_Update)
	    mode = Mode_Update;
	  else if (opInfo->operands[i] & Opr_Dest)
	    mode = Mode_Output;
	  else
	    mode = Mode_Input;

	  use_star = opTakesLabel ();//opInfo->takesLabel ();

	  if (operand->segmentPrefix != Reg_Invalid)
	    {
	      writeReg (operand->segmentPrefix);
	      insnTemplate->writebyte (':');
	    }

	  if ((operand->segmentPrefix != Reg_Invalid || operand->constDisplacement) &&
	      operand->symbolDisplacement.dim == 0)
	    {
	      addOperand ("%a", Arg_Integer, newIntExp (operand->constDisplacement), asmcode);
	      operand->constDisplacement = 0;
	      if (opInfo->operands[i] & Opr_Dest)
		asmcode->clobbersMemory = 1;
	    }

	  if (operand->symbolDisplacement.dim)
	    {
	      Expression * e = operand->symbolDisplacement[0];
	      Declaration * decl = NULL;

	      /* We are generating a memory reference, but the
		 operand could be a floating point constant.  If this
		 is the case, it will be turned into a const static
		 VAR_DECL later in AsmStatement::toIR. */

	      if (e->op == TOKvar)
		decl = ((VarExp *) e)->var;

	      /* The GCC will error (or just warn) on references to
		 nonlocal vars, but it cannot catch closure vars. */

	      /* TODO: Could allow this for the frame-relative
		 case if it's a closure var.  We do not know if
		 it is a closure var at this point, however.
		 Just accept it for now and return false from
		 getFrameRelativeOffset if it turns out not to be.
		 */
	      if (decl && ! (decl->isDataseg () || decl->isCodeseg ()) &&
		  decl->toParent2 () != sc->func)
		stmt->error ("cannot access nonlocal variable %s", decl->toChars ());

	      if (operand->baseReg != Reg_Invalid &&
		  decl && ! decl->isDataseg ())
		{
		  // Use the offset from frame pointer

		  /* GCC doesn't give the front end access to stack offsets
		     when optimization is turned on (3.x) or at all (4.x).

		     Try to convert var[EBP] (or var[ESP] for naked funcs) to
		     a memory expression that does not require us to know
		     the stack offset.
		     */
		  bool isBaseRegBP = operand->baseReg == Reg_EBP ||
		    (global.params.is64bit && operand->baseReg == Reg_RBP);
		  bool isBaseRegSP = operand->baseReg == Reg_ESP ||
		    (global.params.is64bit && operand->baseReg == Reg_RSP);

		  if (operand->indexReg == Reg_Invalid
		      && decl->isVarDeclaration ()
		      && ((isBaseRegBP && ! sc->func->naked)
			  || (isBaseRegSP && sc->func->naked)))
		    {
		      e = new AddrExp (0, e);
		      e->type = decl->type->pointerTo ();

		      /* DMD uses the same frame offsets for naked functions. */
		      if (sc->func->naked)
			operand->constDisplacement += 4;

		      if (operand->constDisplacement)
			{
			  e = new AddExp (0, e,
					  new IntegerExp (0, operand->constDisplacement,
							  Type::tptrdiff_t));
			  e->type = decl->type->pointerTo ();
			}
		      e = new PtrExp (0, e);
		      e->type = decl->type;

		      operand->constDisplacement = 0;
		      operand->baseReg = Reg_Invalid;

		      addOperand (fmt, Arg_Memory, e, asmcode, mode);
		    }
		  else
		    {
		      addOperand ("%a", Arg_FrameRelative, e, asmcode);
		    }

		  if (opInfo->operands[i] & Opr_Dest)
		    asmcode->clobbersMemory = 1;
		}
	      else
		{
		  // Plain memory reference to variable

		  /* If in a reg, DMD moves to memory.. even with -O, so we'll do the same
		     by always using the "m" contraint.

		     In order to get the correct output for function and label symbols,
		     the %an format must be used with the "p" constraint.
		     */
		  if (isDollar (e))
		    {
		      unsigned lbl_num = ++d_priv_asm_label_serial;
		      addLabel (lbl_num);
		      use_star = false;
		      // could make the dollar label part of the same asm..
		      asmcode->dollarLabel = lbl_num;
		    }
		  else if (e->op == TOKdsymbol)
		    {
		      LabelDsymbol * lbl = (LabelDsymbol *) ((DsymbolExp *) e)->s;
		      if (! lbl->asmLabelNum)
			lbl->asmLabelNum = ++d_priv_asm_label_serial;
		      use_star = false;
		      addLabel (lbl->asmLabelNum);
		      // Push label as argument. Not really emitted.
		      asmcode->args.push (new AsmArg (Arg_Label, e, Mode_Input));
		    }
		  else if ((decl && decl->isCodeseg ()))
		    {
		      // if function or label
		      use_star = false;
		      addOperand ("%a", Arg_Pointer, e, asmcode);
		    }
		  else
		    {
		      if (use_star)
			{
			  insnTemplate->writebyte ('*');
			  use_star = false;
			}

		      if (operand->constDisplacement == 0)
			addOperand (fmt, Arg_Memory, e, asmcode, mode);
		      else
			{
			  Expression * offset = newIntExp (operand->constDisplacement);

			  if (decl->isDataseg ())
			    {
			      // Displacement can only come after symbol
			      addOperand (fmt, Arg_Memory, e, asmcode, mode);
			      insnTemplate->writebyte ('+');
			      addOperand ("%a", Arg_Integer, offset, asmcode);
			    }
			  else
			    {
			      // Displacement cannot come after symbol.
			      addOperand ("%a", Arg_Integer, offset, asmcode);
			      insnTemplate->writebyte ('+');
			      addOperand (fmt, Arg_Memory, e, asmcode, mode);
			    }
			}
		    }

		  if (operand->constDisplacement)
		    {
		      // If a memory reference was displaced, tell GCC to not keep
		      // memory values cached in registers across the instruction.
		      if (opInfo->operands[i] & Opr_Dest)
			asmcode->clobbersMemory = 1;
		      operand->constDisplacement = 0;
		    }
		}
	    }

	  if (use_star)
	    insnTemplate->writebyte ('*');

	  if (operand->baseReg != Reg_Invalid || operand->indexReg != Reg_Invalid)
	    {
	      insnTemplate->writebyte ('(');
	      if (operand->baseReg != Reg_Invalid)
		writeReg (operand->baseReg);

	      if (operand->indexReg != Reg_Invalid)
		{
		  insnTemplate->writebyte (',');
		  writeReg (operand->indexReg);
		  if (operand->scale)
		    {
		      insnTemplate->printf (",%d", operand->scale);
		    }
		}

	      insnTemplate->writebyte (')');
	      if (opInfo->operands[i] & Opr_Dest)
		asmcode->clobbersMemory = 1;
	    }
	  break;

	case Opr_Invalid:
	  return false;
	}
    }

  asmcode->insnTemplateLen = insnTemplate->offset;
  asmcode->insnTemplate = (char*) insnTemplate->extractData ();
  return true;
}

bool
AsmProcessor::isIntExp (Expression * exp)
{
  if (exp->op == TOKint64)
    return 1;

  return 0;
}

bool
AsmProcessor::isRegExp (Expression * exp)
{
  // ewww.%%
  return exp->op == TOKmod;
}

bool
AsmProcessor::isLocalSize (Expression * exp)
{
  // cleanup: make a static var
  return exp->op == TOKidentifier && ((IdentifierExp *) exp)->ident == Id::__LOCAL_SIZE;
}

bool
AsmProcessor::isDollar (Expression * exp)
{
  return exp->op == TOKidentifier && ((IdentifierExp *) exp)->ident == Id::__dollar;
}

Expression *
AsmProcessor::newRegExp (int regno)
{
  IntegerExp * e = new IntegerExp (regno);
  e->op = TOKmod;
  return e;
}

Expression *
AsmProcessor::newIntExp (sinteger_t v /* %% type */)
{
  // Only handles 32-bit numbers as this is IA-32.
  return new IntegerExp (stmt->loc, v, Type::tptrdiff_t);
}

void
AsmProcessor::slotExp (Expression * exp)
{
  /*
     if offset, make a note

     if integer, add to immediate
     if reg:
     if not in bracket, set reg (else error)
     if in bracket:
     if not base, set base
     if not index, set index
     else, error
     if symbol:
     set symbol field
   */

  bool is_offset = false;
  exp = exp->optimize (WANTvalue);
  if (exp->op == TOKaddress)
    {
      exp = ((AddrExp *) exp)->e1;
      is_offset = true;
    }
  else if (exp->op == TOKsymoff)
    is_offset = true;

  if (isIntExp (exp))
    {
      if (is_offset)
	invalidExpression ();

      operand->constDisplacement += exp->toInteger ();
      if (! operand->inBracket)
	operand->hasNumber = 1;
    }
  else if (isRegExp (exp))
    {
      if (is_offset)
	invalidExpression ();

      if (! operand->inBracket)
	{
	  if (operand->reg == Reg_Invalid)
	    operand->reg = (Reg) exp->toInteger ();
	  else
	    stmt->error ("too many registers in operand (use brackets)");
	}
      else
	{
	  if (operand->baseReg == Reg_Invalid)
	    {
	      operand->baseReg = (Reg) exp->toInteger ();
	    }
	  else if (operand->indexReg == Reg_Invalid)
	    {
	      operand->indexReg = (Reg) exp->toInteger ();
	      operand->scale = 1;
	    }
	  else
	    {
	      stmt->error ("too many registers memory operand");
	    }
	}
    }
  else if (exp->op == TOKvar || exp->op == TOKsymoff)
    {
      VarDeclaration * v;

      if (exp->op == TOKvar)
	v = ((VarExp *) exp)->var->isVarDeclaration ();
      else
	v = ((SymOffExp *) exp)->var->isVarDeclaration ();

      if (v && v->storage_class & STCfield)
	{
	  operand->constDisplacement += v->offset;
	  if (! operand->inBracket)
	    operand->hasNumber = 1;
	}
      else
	{
	  if (v && v->type->isscalar ())
	    {
	      // DMD doesn't check Tcomplex*, and counts Tcomplex32 as Tfloat64
	      TY ty = v->type->toBasetype()->ty;
	      operand->dataSizeHint = ty == Tfloat80 || ty == Timaginary80 ?
		Extended_Ptr : (PtrType) v->type->size (0);
	    }

	  if (! operand->symbolDisplacement.dim)
	    {
	      if (is_offset && ! operand->inBracket)
		operand->isOffset = 1;
	      operand->symbolDisplacement.push (exp);
	    }
	  else
	    {
	      stmt->error ("too many symbols in operand");
	    }
	}
    }
  else if (exp->op == TOKidentifier || exp->op == TOKdsymbol)
    {
      // %% localsize could be treated as a simple constant..
      // change to addSymbolDisp (e)
      if (! operand->symbolDisplacement.dim)
	{
	  operand->symbolDisplacement.push (exp);
	}
      else
	{
	  stmt->error ("too many symbols in operand");
	}
    }
  else if (exp->op == TOKfloat64)
    {
      if (exp->type && exp->type->isscalar ())
	{
	  // DMD doesn't check Tcomplex*, and counts Tcomplex32 as Tfloat64
	  TY ty = exp->type->toBasetype()->ty;
	  operand->dataSizeHint = (ty == Tfloat80 || ty == Timaginary80 ?
				   Extended_Ptr : (PtrType) exp->type->size (0));
	}
      // Will be converted to a VAR_DECL in AsmStatement::toIR
      operand->symbolDisplacement.push (exp);
    }
  else if (exp->op == TOKtuple)
    {
      TupleExp * te = ((TupleExp *) exp);
      sinteger_t index = operand->constDisplacement;

      if (index >= te->exps->dim)
	{
	  stmt->error ("tuple index %u exceeds length %u", index, te->exps->dim);
	}
      else
	{
	  Expression * e = te->exps->tdata()[index];
	  if (e->op == TOKvar || e->op == TOKfunction)
	    {
	      operand->symbolDisplacement.push (e);
	      operand->constDisplacement = 0;
	    }
	  else
	    stmt->error ("invalid asm operand %s", e->toChars ());
	}
    }
  else if (exp != Handled)
    {
      stmt->error ("invalid operand %s", exp->toChars ());
    }
}

void
AsmProcessor::invalidExpression ()
{
  // %% report operand number
  stmt->error ("invalid expression");
}

Expression *
AsmProcessor::intOp (TOK op, Expression * e1, Expression * e2)
{
  Expression * e;
  if (isIntExp (e1) && (! e2 || isIntExp (e2)))
    {
      switch (op)
	{
	case TOKadd:
	  if (e2)
	    e = new AddExp (stmt->loc, e1, e2);
	  else
	    e = e1;
	  break;

	case TOKmin:
	  if (e2)
	    e = new MinExp (stmt->loc, e1, e2);
	  else
	    e = new NegExp (stmt->loc, e1);
	  break;

	case TOKmul:
	  e = new MulExp (stmt->loc, e1, e2);
	  break;

	case TOKdiv:
	  e = new DivExp (stmt->loc, e1, e2);
	  break;

	case TOKmod:
	  e = new ModExp (stmt->loc, e1, e2);
	  break;

	case TOKshl:
	  e = new ShlExp (stmt->loc, e1, e2);
	  break;

	case TOKshr:
	  e = new ShrExp (stmt->loc, e1, e2);
	  break;

	case TOKushr:
	  e = new UshrExp (stmt->loc, e1, e2);
	  break;

	case TOKnot:
	  e = new NotExp (stmt->loc, e1);
	  break;

	case TOKtilde:
	  e = new ComExp (stmt->loc, e1);
	  break;

	default:
	  gcc_unreachable ();
	}
      e = e->semantic (sc);
      return e->optimize (WANTvalue | WANTinterpret);
    }
  else
    {
      stmt->error ("expected integer operand (s) for '%s'", Token::tochars[op]);
      return newIntExp (0);
    }
}

void
AsmProcessor::parseOperand ()
{
  Expression * exp = parseAsmExp ();
  slotExp (exp);
  if (isRegExp (exp))
    operand->dataSize = (PtrType) regInfo[exp->toInteger ()].size;
}

Expression *
AsmProcessor::parseAsmExp ()
{
  return parseShiftExp ();
}

Expression *
AsmProcessor::parseShiftExp ()
{
  Expression * e1 = parseAddExp ();
  Expression * e2;

  while (1)
    {
      TOK tv = token->value;
      switch (tv)
	{
	case TOKshl:
	case TOKshr:
	case TOKushr:
	  nextToken ();
	  e2 = parseAddExp ();
	  e1 = intOp (tv, e1, e2);
	  continue;

	default:
	  break;
	}
      break;
    }
  return e1;
}

Expression *
AsmProcessor::parseAddExp ()
{
  Expression * e1 = parseMultExp ();
  Expression * e2;

  while (1)
    {
      TOK tv = token->value;
      switch (tv)
	{
	case TOKadd:
	  nextToken ();
	  e2 = parseMultExp ();
	  if (isIntExp (e1) && isIntExp (e2))
	    {
	      e1 = intOp (tv, e1, e2);
	    }
	  else
	    {
	      slotExp (e1);
	      slotExp (e2);
	      e1 = Handled;
	    }
	  continue;

	case TOKmin:
	  // Note: no support for symbol address difference
	  nextToken ();
	  e2 = parseMultExp ();
	  if (isIntExp (e1) && isIntExp (e2))
	    {
	      e1 = intOp (tv, e1, e2);
	    }
	  else
	    {
	      slotExp (e1);
	      e2 = intOp (TOKmin, e2, NULL); // verifies e2 is an int
	      slotExp (e2);
	      e1 = Handled;
	    }
	  continue;

	default:
	  break;
	}
      break;
    }
  return e1;
}

bool
AsmProcessor::tryScale (Expression * e1, Expression * e2)
{
  if (isIntExp (e1) && isRegExp (e2))
    {
      // swap reg and int expressions.
      Expression * et = e1;
      e1 = e2;
      e2 = et;
    }

  if (isRegExp (e1) && isIntExp (e2))
    {
      if (! operand->inBracket)
	{
	  invalidExpression (); // maybe should allow, e.g. DS:EBX+EAX*4
	}

      if (operand->scale || operand->indexReg != Reg_Invalid)
	{
	  invalidExpression ();
	  return true;
	}

      operand->indexReg = (Reg) e1->toInteger ();
      operand->scale = e2->toInteger ();
      if (operand->scale != 1 && operand->scale != 2 &&
	  operand->scale != 4 && operand->scale != 8)
	{
	  stmt->error ("invalid index register scale '%d'", operand->scale);
	  return true;
	}

      return true;
    }
  return false;
}

Expression *
AsmProcessor::parseMultExp ()
{
  Expression * e1 = parseBrExp ();
  Expression * e2;

  while (1)
    {
      TOK tv = token->value;
      switch (tv)
	{
	case TOKmul:
	  nextToken ();
	  e2 = parseMultExp ();
	  if (isIntExp (e1) && isIntExp (e2))
	    e1 = intOp (tv, e1, e2);
	  else if (tryScale (e1,e2))
	    e1 = Handled;
	  else
	    invalidExpression ();
	  continue;

	case TOKdiv:
	case TOKmod:
	  nextToken ();
	  e2 = parseMultExp ();
	  e1 = intOp (tv, e1, e2);
	  continue;

	default:
	  break;
	}
      break;
    }
  return e1;
}

Expression *
AsmProcessor::parseBrExp ()
{
  // %% check (why is bracket lower precends..)
  // 3+4[eax] -> 3 + (4 [EAX]) ..

  // only one bracked allowed, so this doesn't quite handle
  // the spec'd syntax
  Expression * e;

  if (token->value == TOKlbracket)
    e = Handled;
  else
    e = parseUnaExp ();

  // DMD allows multiple bracket expressions.
  while (token->value == TOKlbracket)
    {
      nextToken ();

      operand->inBracket = operand->hasBracket = 1;
      slotExp (parseAsmExp ());
      operand->inBracket = 0;

      if (token->value == TOKrbracket)
	nextToken ();
      else
	stmt->error ("missing ']'");
    }
  return e;
}

PtrType
AsmProcessor::isPtrType (Token * tok)
{
  switch (tok->value)
    {
    case TOKint8:
      return Byte_Ptr;

    case TOKint16:
      return Short_Ptr;

    case TOKint32:
      return Int_Ptr;

    case TOKint64:
      // 'long ptr' isn't accepted? (it is now - qword)
      return Long_Ptr;

    case TOKfloat32:
      return Float_Ptr;

    case TOKfloat64:
      return Double_Ptr;

    case TOKfloat80:
      return Extended_Ptr;

    case TOKidentifier:
      for (size_t i = 0; i < (size_t) N_PtrNames; i++)
	{
	  if (tok->ident == ptrTypeIdentTable[i])
	    return ptrTypeValueTable[i];
	}
      break;

    default:
      break;
    }
  return Default_Ptr;
}

Expression *
AsmProcessor::parseUnaExp ()
{
  Expression * e = NULL;
  PtrType ptr_type;

  // First, check for type prefix.
  if (token->value != TOKeof &&
      peekToken ()->value == TOKidentifier &&
      peekToken ()->ident == Id::ptr)
    {

      ptr_type = isPtrType (token);
      if (ptr_type != Default_Ptr)
	{
	  if (operand->dataSize == Default_Ptr)
	    operand->dataSize = ptr_type;
	  else
	    stmt->error ("multiple specifications of operand size");
	}
      else
	stmt->error ("unknown operand size '%s'", token->toChars ());

      nextToken ();
      nextToken ();
      return parseAsmExp ();
    }

  TOK tv = token->value;
  switch (tv)
    {
    case TOKidentifier:
      if (token->ident == ident_seg)
	{
	  nextToken ();
	  stmt->error ("'seg' not supported");
	  e = parseAsmExp ();
	}
      else if (token->ident == Id::offset ||
	       token->ident == Id::offsetof)
	{
	  if (token->ident == Id::offset && ! global.params.useDeprecated)
	    stmt->error ("offset deprecated, use offsetof");

	  nextToken ();
	  e = parseAsmExp ();
	  e = new AddrExp (stmt->loc, e);
	  e = e->semantic (sc);
	}
      else
	{
	  // primary exp
	  break;
	}

      return e;

    case TOKadd:
    case TOKmin:
    case TOKnot:
    case TOKtilde:
      nextToken ();
      e = parseUnaExp ();
      return intOp (tv, e, NULL);

    default:
      // primary exp
      break;
    }

  return parsePrimaryExp ();
}

Expression *
AsmProcessor::parsePrimaryExp ()
{
  Expression * e;
  Identifier * ident = NULL;

  // get rid of short/long prefixes for branches
  if (opTakesLabel () && (token->value == TOKint16 || token->value == TOKint64))
    nextToken ();

  switch (token->value)
    {
    case TOKint32v:
    case TOKuns32v:
    case TOKint64v:
    case TOKuns64v:
      // semantic here?
      // %% for tok64 really should use 64bit type
      e = new IntegerExp (stmt->loc, token->uns64value, Type::tptrdiff_t);
      nextToken ();
      break;

    case TOKfloat32v:
    case TOKfloat64v:
    case TOKfloat80v:
      // %% need different types?
      e = new RealExp (stmt->loc, token->float80value, Type::tfloat80);
      nextToken ();
      break;

    case TOKidentifier:
    case TOKthis:
      ident = token->ident;
      nextToken ();

      if (ident == Id::__LOCAL_SIZE)
	{
	  return new IdentifierExp (stmt->loc, ident);
	}
      else if (ident == Id::__dollar)
	{
	  return new IdentifierExp (stmt->loc, ident);
	}
      else
	{
	  e = new IdentifierExp (stmt->loc, ident);
	}

      // If this is more than one component ref, it gets complicated: * (&Field + n)
      // maybe just do one step at a time..
      // simple case is Type.f -> VarDecl (field)
      // actually, DMD only supports on level...
      // X.y+Y.z[EBX] is supported, tho..
      // %% doesn't handle properties (check%%)
      while (token->value == TOKdot)
	{
	  nextToken ();
	  if (token->value == TOKidentifier)
	    {
	      e = new DotIdExp (stmt->loc, e, token->ident);
	      nextToken ();
	    }
	  else
	    {
	      stmt->error ("expected identifier");
	      return Handled;
	    }
	}

      // check for reg first then dotexp is an error?
      if (e->op == TOKidentifier)
	{
	  for (size_t i = 0; i < (size_t) N_Regs; i++)
	    {
	      if (ident != regInfo[i].ident)
		continue;

	      if ((Reg) i == Reg_ST && token->value == TOKlparen)
		{
		  nextToken ();
		  switch (token->value)
		    {
		    case TOKint32v: case TOKuns32v:
		    case TOKint64v: case TOKuns64v:
		      if (token->uns64value < 8)
			{
			  e = newRegExp((Reg) (Reg_ST + token->uns64value));
			}
		      else
			{
			  stmt->error ("invalid floating point register index");
			  e = Handled;
			}

		      nextToken ();
		      if (token->value == TOKrparen)
			nextToken ();
		      else
			stmt->error ("expected ')'");

		      return e;

		    default:
		      break;
		    }

		  invalidExpression ();
		  return Handled;
		}
	      else if (token->value == TOKcolon)
		{
		  nextToken ();
		  if (operand->segmentPrefix != Reg_Invalid)
		    stmt->error ("too many segment prefixes");
		  else if (i >= Reg_CS && i <= Reg_GS)
		    operand->segmentPrefix = (Reg) i;
		  else
		    stmt->error ("'%s' is not a segment register", ident->string);

		  return parseAsmExp ();
		}
	      else
		{
		  return newRegExp((Reg) i);
		}
	    }
	}

      if (opTakesLabel ()/*opInfo->takesLabel ()*/ && e->op == TOKidentifier)
	{
	  // DMD uses labels secondarily to other symbols, so check
	  // if IdentifierExp::semantic won't find anything.
	  Dsymbol *scopesym;

	  if (! sc->search (stmt->loc, ident, & scopesym))
	    return new DsymbolExp (stmt->loc,
				   sc->func->searchLabel (ident));
	}

      e = e->semantic (sc);

      /* Special case for floating point literals: Try to
	 resolve to a memory reference.  If not possible,
	 leave it as a literal and generate an anonymous
	 var in AsmStatement::toIR.
	 */
      if (e->op == TOKfloat64)
	{
	  Dsymbol * sym = sc->search (stmt->loc, ident, NULL);
	  if (sym)
	    {
	      VarDeclaration *v = sym->isVarDeclaration ();
	      if (v && v->isDataseg ())
		{
		  Expression *ve = new VarExp (stmt->loc, v);
		  ve->type = e->type;
		  e = ve;
		}
	    }
	}
      return e;

    case TOKdollar:
      nextToken ();
      return new IdentifierExp (stmt->loc, Id::__dollar);

    default:
      if (op == Op_FMath0 || op == Op_FdST0ST1 || op == Op_FMath)
	return Handled;

      invalidExpression ();
      return Handled;

    }

  return e;
}

void
AsmProcessor::doAlign ()
{
  // .align bits vs. bytes...
  // apparently a.out platforms use bits instead of bytes...

  // parse primary: DMD allows 'MyAlign' (const int) but not '2+2'
  // GAS is padding with NOPs last time I checked.
  Expression * e = parseAsmExp ()->optimize (WANTvalue | WANTinterpret);
  dinteger_t align = e->toInteger ();

  if ((align & -align) == align)
    {
      // %% is this printf portable?
#ifdef HAVE_GAS_BALIGN_AND_P2ALIGN
      insnTemplate->printf (".balign %u", (unsigned) align);
#else
      insnTemplate->printf (".align %u", (unsigned) align);
#endif
    }
  else
    {
      stmt->error ("alignment must be a power of 2");
    }
  setAsmCode ();
}

void
AsmProcessor::doEven ()
{
  // .align for GAS is in bits, others probably use bytes..
#ifdef HAVE_GAS_BALIGN_AND_P2ALIGN
  insnTemplate->writestring (".balign 2");
#else
  insnTemplate->writestring (".align 2");
#endif
  setAsmCode ();
}

void
AsmProcessor::doNaked ()
{
  // %% can we assume sc->func != 0?
  sc->func->naked = 1;
}

void
AsmProcessor::doData ()
{
  machine_mode mode;
  static const char * directives[] =
    { ".byte", ".short", ".long", ".long", "", "", "" };

  while (1)
    {
      // DMD is pretty strict here, not even constant expressions are allowed.
      d_uns64 intvalue;
      unsigned char* strvalue;
      size_t strlength;
      real_value fltvalue;
      long words[3];

      if (token->value == TOKidentifier)
	{
	  Expression * e = new IdentifierExp (stmt->loc, token->ident);
	  e = e->semantic (sc);
	  e = e->optimize (WANTvalue | WANTinterpret);
	  if (e->op == TOKint64)
	    {
	      intvalue = e->toInteger ();
	      goto op_integer;
	    }
	  else if (e->op == TOKfloat64)
	    {
	      fltvalue = e->toReal().rv ();
	      goto op_float;
	    }
	  else if (e->op == TOKstring)
	    {
	      StringExp * s = (StringExp *) e;
	      strvalue = (unsigned char *) s->string;
	      strlength = s->len;
	      goto op_string;
	    }
	}

      if (token->value == TOKint32v || token->value == TOKuns32v ||
	  token->value == TOKint64v || token->value == TOKuns64v)
	{
	  intvalue = token->uns64value;

	op_integer:
	  switch (op)
	    {
	    case Op_db:
	    case Op_ds:
	    case Op_di:
	    case Op_dl:
	      insnTemplate->writestring (directives[op - Op_db]);
	      insnTemplate->writebyte (' ');

	      if (op != Op_dl)
		insnTemplate->printf ("%u", (d_uns32) intvalue);
	      else
		{
		  // Output two .longs.  GAS has .quad, but would have to rely on 'L' format ..
		  // just need to use HOST_WIDE_INT_PRINT_DEC
		  insnTemplate->printf ("%u,%u", (d_uns32) intvalue, (d_uns32) (intvalue >> 32));
		}
	      break;

	    default:
	      stmt->error ("floating point expected");
	    }
	}
      else if (token->value == TOKfloat32v || token->value == TOKfloat64v ||
	       token->value == TOKfloat80v)
	{
	  fltvalue = token->float80value.rv ();

	op_float:
	  switch (op)
	    {
	    case Op_df:
	      mode = SFmode;
	      goto do_float;

	    case Op_dd:
	      mode = DFmode;
	      goto do_float;

	    case Op_de:
#ifndef TARGET_80387
#define XFmode TFmode
#endif
	      mode = XFmode; // not TFmode

	do_float:
	      real_to_target (words, & fltvalue, mode);
	      insnTemplate->writestring (directives[op - Op_db]);
	      insnTemplate->writebyte (' ');

	      // don't use directives..., just use .long like GCC
	      insnTemplate->printf (".long %u", words[0]);
	      if (mode != SFmode)
		insnTemplate->printf (",%u", words[1]);

	      // DMD outputs 10 bytes, so we need to switch to .short here
	      if (mode == XFmode)
		insnTemplate->printf ("\n\t .short %u", words[2]);

	      break;

	    default:
	      stmt->error ("integer constant expected");
	    }
	}
      else if (token->value == TOKstring)
	{
	  strvalue = token->ustring;
	  strlength = token->len;
	op_string:
	  // Loop through string values.
	  for (size_t i = 0; i < strlength; i++)
	    {
	      // %% Check for character truncation?
	      intvalue = strvalue[i];

	      switch (op)
		{
		case Op_db:
		case Op_ds:
		case Op_di:
		case Op_dl:
		  insnTemplate->writestring (directives[op - Op_db]);
		  insnTemplate->writebyte (' ');

		  if (op != Op_dl)
		    insnTemplate->printf ("%u", (d_uns32) intvalue);
		  else
		    {
		      // As above, output two .longs
		      insnTemplate->printf ("%u,%u", (d_uns32) intvalue,
					    (d_uns32) (intvalue >> 32));
		    }

		  insnTemplate->writestring ("\n\t");
		  break;

		default:
		  stmt->error ("floating point expected");
		}
	    }
	}
      else
	{
	  stmt->error ("const initializer expected");
	}

      nextToken ();
      if (token->value == TOKcomma)
	{
	  // GAS can only do one data instruction per line,
	  // using commas doesn't work here.
	  insnTemplate->writestring ("\n\t");
	  nextToken ();
	}
      else if (token->value == TOKeof)
	{
	  break;
	}
      else
	{
	  stmt->error ("expected comma");
	}
    }

  setAsmCode ();
}


#else

bool d_have_inline_asm () { return false; }

Statement *
AsmStatement::semantic (Scope *sc)
{
  sc->func->hasReturnExp |= 8;
  return Statement::semantic (sc);
}

void
AsmStatement::toIR (IRState *)
{
  sorry ("assembler statements are not supported on this target");
}

#endif
