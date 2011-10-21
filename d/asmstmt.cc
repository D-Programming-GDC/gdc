/* GDC -- D front-end for GCC
   Copyright (C) 2004 David Friedman

   Modified by
    Michael Parrott, (C) 2009
    Iain Buclaw, (C) 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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

    AsmArg(AsmArgType type, Expression * expr, AsmArgMode mode)
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

    AsmCode(unsigned n_regs)
    {
        insnTemplate = NULL;
        insnTemplateLen = 0;
        clbregs = new unsigned[n_regs];
        memset(clbregs, 0, sizeof(unsigned)*n_regs);
        dollarLabel = 0;
        clobbersMemory = 0;
    }
};

/* Apple GCC extends ASM_EXPR to five operands; cannot use build4. */
tree
d_build_asm_stmt(tree t1, tree t2, tree t3, tree t4, tree t5)
{
    tree t = make_node(ASM_EXPR);
    TREE_TYPE(t) = void_type_node;
    SET_EXPR_LOCATION(t, input_location);
    TREE_OPERAND(t,0) = t1;     // STRING
    TREE_OPERAND(t,1) = t2;     // OUTPUTS
    TREE_OPERAND(t,2) = t3;     // INPUTS
    TREE_OPERAND(t,3) = t4;     // CLOBBERS
    TREE_OPERAND(t,4) = t5;     // LABELS
    TREE_SIDE_EFFECTS(t) = 1;
    return t;
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
d_format_priv_asm_label(char * buf, unsigned n)
{
    //ASM_GENERATE_INTERNAL_LABEL(buf, "LDASM", n);//inserts a '*' for use with assemble_name
    sprintf(buf, ".LDASM%u", n);
}

void
d_expand_priv_asm_label(IRState * irs, unsigned n)
{
    char buf[64];
    d_format_priv_asm_label(buf, n);
    strcat(buf, ":");
    tree insnt = build_string(strlen(buf), buf);
    tree t = d_build_asm_stmt(insnt, NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
    ASM_VOLATILE_P(t) = 1;
    ASM_INPUT_P(t) = 1; // what is this doing?
    irs->addExp(t);
}

ExtAsmStatement::ExtAsmStatement(Loc loc, Expression *insnTemplate, Expressions *args, Identifiers *argNames,
    Expressions *argConstraints, int nOutputArgs, Expressions *clobbers)
    : Statement(loc)
{
    this->insnTemplate = insnTemplate;
    this->args = args;
    this->argNames = argNames;
    this->argConstraints = argConstraints;
    this->nOutputArgs = nOutputArgs;
    this->clobbers = clobbers;
}

Statement *
ExtAsmStatement::syntaxCopy()
{
    /* insnTemplate, argConstraints, and clobbers would be
       semantically static in GNU C. */
    Expression *insnTemplate = this->insnTemplate->syntaxCopy();
    Expressions * args = Expression::arraySyntaxCopy(this->args);
    // argNames is an array of identifiers
    Expressions * argConstraints = Expression::arraySyntaxCopy(this->argConstraints);
    Expressions * clobbers = Expression::arraySyntaxCopy(this->clobbers);
    return new ExtAsmStatement(loc, insnTemplate, args, argNames,
                               argConstraints, nOutputArgs, clobbers);
}

Statement *
ExtAsmStatement::semantic(Scope *sc)
{
    insnTemplate = insnTemplate->semantic(sc);
    insnTemplate = insnTemplate->optimize(WANTvalue);
#if V2
    if (sc->func)
    {
        if (sc->func->setUnsafe())
            error("extended assembler not allowed in @safe function %s", sc->func->toChars());
    }
#endif
    if (insnTemplate->op != TOKstring || ((StringExp *)insnTemplate)->sz != 1)
        error("instruction template must be a constant char string");
    if (args)
    {
        for (size_t i = 0; i < args->dim; i++)
        {
            Expression * e = args->tdata()[i];
            e = e->semantic(sc);
            if (i < nOutputArgs)
                e = e->modifiableLvalue(sc, NULL);
            else
                e = e->optimize(WANTvalue);
            args->tdata()[i] = e;

            e = argConstraints->tdata()[i];
            e = e->semantic(sc);
            e = e->optimize(WANTvalue);
            if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
                error("constraint must be a constant char string");
            argConstraints->tdata()[i] = e;
        }
    }
    if (clobbers)
    {
        for (size_t i = 0; i < clobbers->dim; i++)
        {
            Expression * e = clobbers->tdata()[i];
            e = e->semantic(sc);
            e = e->optimize(WANTvalue);
            if (e->op != TOKstring || ((StringExp *)e)->sz != 1)
                error("clobber specification must be a constant char string");
            clobbers->tdata()[i] = e;
        }
    }
    return this;
}

void
ExtAsmStatement::toCBuffer(OutBuffer *buf, HdrGenState *hgs)
{
    buf->writestring("gcc asm { ");
    if (insnTemplate)
        buf->writestring(insnTemplate->toChars());
    buf->writestring(" : ");
    if (args)
    {
        for (size_t i = 0; i < args->dim; i++)
        {
            Identifier * name = argNames->tdata()[i];
            Expression * constr = argConstraints->tdata()[i];
            Expression * arg = args->tdata()[i];

            if (name)
            {
                buf->writestring("[");
                buf->writestring(name->toChars());
                buf->writestring("] ");
            }
            if (constr)
            {
                buf->writestring(constr->toChars());
                buf->writestring(" ");
            }
            if (arg)
            {
                buf->writestring(arg->toChars());
            }

            if (i < nOutputArgs - 1)
                buf->writestring(", ");
            else if(i == nOutputArgs - 1)
                buf->writestring(" : ");
            else if(i < args->dim - 1)
                buf->writestring(", ");
        }
    }
    if (clobbers)
    {
        buf->writestring(" : ");
        for (size_t i = 0; i < clobbers->dim; i++)
        {
            Expression * clobber = clobbers->tdata()[i];
            buf->writestring(clobber->toChars());
            if (i < clobbers->dim - 1)
                buf->writestring(", ");
        }
    }
    buf->writestring("; }");
    buf->writenl();
}

int
ExtAsmStatement::blockExit(bool mustNotThrow)
{
    if (mustNotThrow)
        error("asm statements are assumed to throw", toChars());
    // TODO: Be smarter about this
    return BEany;
}


// StringExp::toIR usually adds a NULL.  We don't want that...

static tree
naturalString(Expression * e)
{
    // don't fail, just an error?
    gcc_assert(e->op == TOKstring);
    StringExp * s = (StringExp *) e;
    gcc_assert(s->sz == 1);
    return build_string(s->len, (char *) s->string);
}

void ExtAsmStatement::toIR(IRState *irs)
{
    ListMaker outputs;
    ListMaker inputs;
    ListMaker tree_clobbers;

    gen.doLineNote(loc);

    if (args)
    {
        for (size_t i = 0; i < args->dim; i++)
        {
            Identifier * name = argNames->tdata()[i]; 
            Expression * constr = argConstraints->tdata()[i];
            tree p = tree_cons(name ? build_string(name->len, name->string) : NULL_TREE,
                naturalString(constr), NULL_TREE);
            tree v = (args->tdata()[i])->toElem(irs);

            if (i < nOutputArgs)
                outputs.cons(p, v);
            else
                inputs.cons(p, v);
        }
    }
    if (clobbers)
    {
        for (size_t i = 0; i < clobbers->dim; i++)
        {
            Expression * clobber = clobbers->tdata()[i];
            tree_clobbers.cons(NULL_TREE, naturalString(clobber));
        }
    }
    irs->doAsm(naturalString(insnTemplate), outputs.head, inputs.head, tree_clobbers.head);
}

#ifdef TARGET_80387
#include "d-asm-i386.h"
#else
#define D_NO_INLINE_ASM_AT_ALL
#endif

#ifndef D_NO_INLINE_ASM_AT_ALL

bool d_have_inline_asm() { return true; }

Statement *
AsmStatement::semantic(Scope *sc)
{
#if V2
    if (sc->func && sc->func->isSafe())
        error("inline assembler not allowed in @safe function %s", sc->func->toChars());
#endif
    sc->func->inlineStatus = ILSno; // %% not sure
    // %% need to set DECL_UNINLINABLE too?
    
#if V1
    // DMD assumes that inline assembly sets return argument.
    // This avoids "missing return expression" assertion.
    FuncDeclaration *fd = sc->parent->isFuncDeclaration();
    gcc_assert(fd);    
    fd->inlineAsm = 1;
#endif
    
    sc->func->hasReturnExp |= 8; // %% DMD does this, apparently...

    // empty statement -- still do the above things because they might be expected?
    if (! tokens)
        return this;

    AsmProcessor ap(sc, this);
    ap.run();
    return this;
}

void
AsmStatement::toIR(IRState * irs)
{
    gen.doLineNote(loc);

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
        i_cns = build_string(1, "i");
        p_cns = build_string(1, "p");
        m_cns = build_string(1, "m");
        mw_cns  = build_string(2, "=m");
        mrw_cns = build_string(2, "+m");
        memory_name = build_string(6, "memory");
        dkeep(i_cns);
        dkeep(p_cns);
        dkeep(m_cns);
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

    gcc_assert(code->args.dim <= 10);

    for (size_t i = 0; i < code->args.dim; i++)
    {
        AsmArg * arg = code->args.tdata()[i];

        bool is_input = true;
        TOK arg_op = arg->expr->op;
        tree arg_val = NULL_TREE;
        tree cns = NULL_TREE;

        switch (arg->type)
        {
            case Arg_Integer:
                arg_val = arg->expr->toElem(irs);
                cns = i_cns;
                break;
            case Arg_Pointer:
                if (arg_op == TOKvar)
                    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
                else if (arg_op == TOKdsymbol)
                {
                    arg_val = irs->getLabelTree((LabelDsymbol *) ((DsymbolExp *) arg->expr)->s);
                    labels.cons(NULL_TREE, arg_val);
                }
                else
                    arg_val = arg->expr->toElem(irs);

                if (arg_op != TOKaddress && arg_op != TOKsymoff && arg_op != TOKadd)
                    arg_val = irs->addressOf(arg_val);

                cns = p_cns;
                break;
            case Arg_Memory:
                if (arg_op == TOKvar)
                    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
                else if (arg_op == TOKfloat64)
                {
                    /* Constant scalar value.  In order to reference it as memory,
                       create an anonymous static var. */
                    tree cnst = d_build_decl(VAR_DECL, NULL_TREE, arg->expr->type->toCtype());
                    g.ofile->giveDeclUniqueName(cnst);
                    DECL_INITIAL(cnst) = arg->expr->toElem(irs);
                    TREE_STATIC(cnst) = TREE_CONSTANT(cnst) = TREE_READONLY(cnst) =
                        TREE_PRIVATE(cnst) = DECL_ARTIFICIAL(cnst) = DECL_IGNORED_P(cnst) = 1;
                    g.ofile->rodc(cnst, 1);
                    arg_val = cnst;
                }
                else
                    arg_val = arg->expr->toElem(irs);
                if (DECL_P(arg_val))
                    TREE_ADDRESSABLE(arg_val) = 1;
                switch (arg->mode)
                {
                    case Mode_Input:  cns = m_cns; break;
                    case Mode_Output: cns = mw_cns;  is_input = false; break;
                    case Mode_Update: cns = mrw_cns; is_input = false; break;
                    default: gcc_unreachable(); break;
                }
                break;
            case Arg_FrameRelative:
                if (arg_op == TOKvar)
                    arg_val = ((VarExp *) arg->expr)->var->toSymbol()->Stree;
                else
                    gcc_unreachable();
                if (getFrameRelativeValue(arg_val, & var_frame_offset))
                {
                    arg_val = irs->integerConstant(var_frame_offset);
                    cns = i_cns;
                }
                else
                {
                    this->error("%s", "argument not frame relative");
                    return;
                }
                if (arg->mode != Mode_Input)
                    clobbers_mem = true;
                break;
            case Arg_LocalSize:
                var_frame_offset = frame_offset;
                if (var_frame_offset < 0)
                    var_frame_offset = - var_frame_offset;
                arg_val = irs->integerConstant(var_frame_offset);
                cns = i_cns;
                break;
            default:
                gcc_unreachable();
        }

        if (is_input)
        {
            arg_map[i] = --input_idx;
            inputs.cons(tree_cons(NULL_TREE, cns, NULL_TREE), arg_val);
        }
        else
        {
            arg_map[i] = n_outputs++;
            outputs.cons(tree_cons(NULL_TREE, cns, NULL_TREE), arg_val);
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
                clobbers.cons(NULL_TREE, regInfo[i].gccName);
        }
        if (clobbers_mem)
            clobbers.cons(NULL_TREE, memory_name);
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
    //printf("start: %.*s\n", code->insnTemplateLen, code->insnTemplate);
    while (p < q)
    {
        if (pct)
        {
            if (*p >= '0' && *p <= '9')
            {   // %% doesn't check against nargs
                *p = '0' + arg_map[*p - '0'];
                pct = false;
            }
            else if (*p == '%')
            {
                pct = false;
            }
            //gcc_assert(*p == '%');// could be 'a', etc. so forget it..
        }
        else if (*p == '%')
        {
            pct = true;
        }
        ++p;
    }

    //printf("final: %.*s\n", code->insnTemplateLen, code->insnTemplate);
    tree insnt = build_string(code->insnTemplateLen, code->insnTemplate);
    tree t = d_build_asm_stmt(insnt, outputs.head, inputs.head, clobbers.head, labels.head);
    ASM_VOLATILE_P(t) = 1;
    irs->addExp(t);
    if (code->dollarLabel)
        d_expand_priv_asm_label(irs, code->dollarLabel);
}

#else

bool d_have_inline_asm() { return false; }

Statement *
AsmStatement::semantic(Scope *sc)
{
    sc->func->hasReturnExp |= 8;
    return Statement::semantic(sc);
}

void
AsmStatement::toIR(IRState *)
{
    sorry("assembler statements are not supported on this target");
}

#endif
