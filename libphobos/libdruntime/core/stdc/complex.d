
/**
 * D header file for C99.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */

/*          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
module core.stdc.complex;

extern (C):
nothrow:

alias creal complex;
alias ireal imaginary;

version (GNU)
{
    import gcc.builtins;

    alias __builtin_cacos cacos;
    alias __builtin_cacosf cacosf;
    alias __builtin_cacosl cacosl;

    alias __builtin_casin casin;
    alias __builtin_casinf casinf;
    alias __builtin_casinl casinl;

    alias __builtin_catan catan;
    alias __builtin_catanf catanf;
    alias __builtin_catanl catanl;

    alias __builtin_ccos ccos;
    alias __builtin_ccosf ccosf;
    alias __builtin_ccosl ccosl;

    alias __builtin_csin csin;
    alias __builtin_csinf csinf;
    alias __builtin_csinl csinl;

    alias __builtin_ctan ctan;
    alias __builtin_ctanf ctanf;
    alias __builtin_ctanl ctanl;

    alias __builtin_cacosh cacosh;
    alias __builtin_cacoshf cacoshf;
    alias __builtin_cacoshl cacoshl;

    alias __builtin_casinh casinh;
    alias __builtin_casinhf casinhf;
    alias __builtin_casinhl casinhl;

    alias __builtin_catanh catanh;
    alias __builtin_catanhf catanhf;
    alias __builtin_catanhl catanhl;

    alias __builtin_ccosh ccosh;
    alias __builtin_ccoshf ccoshf;
    alias __builtin_ccoshl ccoshl;

    alias __builtin_csinh csinh;
    alias __builtin_csinhf csinhf;
    alias __builtin_csinhl csinhl;

    alias __builtin_ctanh ctanh;
    alias __builtin_ctanhf ctanhf;
    alias __builtin_ctanhl ctanhl;

    alias __builtin_cexp cexp;
    alias __builtin_cexpf cexpf;
    alias __builtin_cexpl cexpl;

    alias __builtin_clog clog;
    alias __builtin_clogf clogf;
    alias __builtin_clogl clogl;

    alias __builtin_cabs cabs;
    alias __builtin_cabsf cabsf;
    alias __builtin_cabsl cabsl;

    alias __builtin_cpow cpow;
    alias __builtin_cpowf cpowf;
    alias __builtin_cpowl cpowl;

    alias __builtin_csqrt csqrt;
    alias __builtin_csqrtf csqrtf;
    alias __builtin_csqrtl csqrtl;

    alias __builtin_carg carg;
    alias __builtin_cargf cargf;
    alias __builtin_cargl cargl;

    alias __builtin_cimag cimag;
    alias __builtin_cimagf cimagf;
    alias __builtin_cimagl cimagl;

    alias __builtin_conj conj;
    alias __builtin_conjf conjf;
    alias __builtin_conjl conjl;

    alias __builtin_cproj cproj;
    alias __builtin_cprojf cprojf;
    alias __builtin_cprojl cprojl;

    //alias __builtin_creal creal;
    alias __builtin_crealf crealf;
    alias __builtin_creall creall;
}
else
{
cdouble cacos(cdouble z);
cfloat  cacosf(cfloat z);
creal   cacosl(creal z);

cdouble casin(cdouble z);
cfloat  casinf(cfloat z);
creal   casinl(creal z);

cdouble catan(cdouble z);
cfloat  catanf(cfloat z);
creal   catanl(creal z);

cdouble ccos(cdouble z);
cfloat  ccosf(cfloat z);
creal   ccosl(creal z);

cdouble csin(cdouble z);
cfloat  csinf(cfloat z);
creal   csinl(creal z);

cdouble ctan(cdouble z);
cfloat  ctanf(cfloat z);
creal   ctanl(creal z);

cdouble cacosh(cdouble z);
cfloat  cacoshf(cfloat z);
creal   cacoshl(creal z);

cdouble casinh(cdouble z);
cfloat  casinhf(cfloat z);
creal   casinhl(creal z);

cdouble catanh(cdouble z);
cfloat  catanhf(cfloat z);
creal   catanhl(creal z);

cdouble ccosh(cdouble z);
cfloat  ccoshf(cfloat z);
creal   ccoshl(creal z);

cdouble csinh(cdouble z);
cfloat  csinhf(cfloat z);
creal   csinhl(creal z);

cdouble ctanh(cdouble z);
cfloat  ctanhf(cfloat z);
creal   ctanhl(creal z);

cdouble cexp(cdouble z);
cfloat  cexpf(cfloat z);
creal   cexpl(creal z);

cdouble clog(cdouble z);
cfloat  clogf(cfloat z);
creal   clogl(creal z);

 double cabs(cdouble z);
 float  cabsf(cfloat z);
 real   cabsl(creal z);

cdouble cpow(cdouble x, cdouble y);
cfloat  cpowf(cfloat x, cfloat y);
creal   cpowl(creal x, creal y);

cdouble csqrt(cdouble z);
cfloat  csqrtf(cfloat z);
creal   csqrtl(creal z);

 double carg(cdouble z);
 float  cargf(cfloat z);
 real   cargl(creal z);

 double cimag(cdouble z);
 float  cimagf(cfloat z);
 real   cimagl(creal z);

cdouble conj(cdouble z);
cfloat  conjf(cfloat z);
creal   conjl(creal z);

cdouble cproj(cdouble z);
cfloat  cprojf(cfloat z);
creal   cprojl(creal z);

// double creal(cdouble z);
 float  crealf(cfloat z);
 real   creall(creal z);
}
