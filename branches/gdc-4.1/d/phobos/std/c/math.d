
/**
 * C's &lt;math.h&gt;
 * Authors: Walter Bright, Digital Mars, www.digitalmars.com
 * License: Public Domain
 * Macros:
 *	WIKI=Phobos/StdCMath
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.

   Modified by David Friedman, February 2007
*/

module std.c.math;

private import std.stdint;

extern (C):

alias float float_t;	///
alias double double_t;	///

const double HUGE_VAL  = double.infinity;	///
const float HUGE_VALF = float.infinity;	/// ditto
const real HUGE_VALL = real.infinity;	/// ditto

const float INFINITY = float.infinity;	///
const float NAN = float.nan;	///

enum
{
    FP_NANS,	// extension
    FP_NANQ,	// extension
    FP_INFINITE,	///
    FP_NAN = FP_NANQ,	///
    FP_NORMAL = 3,	///
    FP_SUBNORMAL = 4,	///
    FP_ZERO = 5,	///
    FP_EMPTY = 6,	// extension
    FP_UNSUPPORTED = 7, // extension
}

enum
{
    FP_FAST_FMA  = 0,	///
    FP_FAST_FMAF = 0,	///
    FP_FAST_FMAL = 0,	///
}

const int FP_ILOGB0   = int.min;	///
const int FP_ILOGBNAN = int.min;	///

const int MATH_ERRNO     = 1;	///
const int MATH_ERREXCEPT = 2;	///
const int math_errhandling   = MATH_ERRNO | MATH_ERREXCEPT;	///

version (GNU)
{
    private import gcc.builtins;
    
    double acos(double x);
    float  acosf(float x);
    
    double asin(double x);
    float  asinf(float x);
    
    double atan(double x);
    float  atanf(float x);
    
    double atan2(double y, double x);
    float  atan2f(float y, float x);
    
    double cos(double x);
    float  cosf(float x);
    
    double sin(double x);
    float  sinf(float x);
    
    double tan(double x);
    float  tanf(float x);
    
    double acosh(double x);
    float  acoshf(float x);
    
    double asinh(double x);
    float  asinhf(float x);
    
    double atanh(double x);
    float  atanhf(float x);
    
    double cosh(double x);
    float  coshf(float x);
    
    double sinh(double x);
    float  sinhf(float x);
    
    double tanh(double x);
    float  tanhf(float x);
    
    double exp(double x);
    float  expf(float x);
    
    double exp2(double x);
    float  exp2f(float x);
    
    double expm1(double x);
    float  expm1f(float x);
    
    double frexp(double value, int *exp);
    float  frexpf(float value, int *exp);
    
    int    ilogb(double x);
    int    ilogbf(float x);
    
    double ldexp(double x, int exp);
    float  ldexpf(float x, int exp);
    
    double log(double x);
    float  logf(float x);
    
    double log10(double x);
    float  log10f(float x);
    
    double log1p(double x);
    float  log1pf(float x);
    
    double log2(double x);
    float  log2f(float x);
    
    double logb(double x);
    float  logbf(float x);
    
    double modf(double value, double *iptr);
    float  modff(float value, float *iptr);
    
    double scalbn(double x, int n);
    float  scalbnf(float x, int n);
    
    double scalbln(double x, int n);
    float  scalblnf(float x, int n);
    
    double cbrt(double x);
    float  cbrtf(float x);
    
    double fabs(double x);
    float  fabsf(float x);
    
    double hypot(double x, double y);
    float  hypotf(float x, float y);
    
    double pow(double x, double y);
    float  powf(float x, float y);
    
    double sqrt(double x);
    float  sqrtf(float x);
    
    double erf(double x);
    float  erff(float x);
    
    double erfc(double x);
    float  erfcf(float x);
    
    double lgamma(double x);
    float  lgammaf(float x);
    
    double tgamma(double x);
    float  tgammaf(float x);
    
    double ceil(double x);
    float  ceilf(float x);
    
    double floor(double x);
    float  floorf(float x);
    
    double nearbyint(double x);
    float  nearbyintf(float x);
    
    double rint(double x);
    float  rintf(float x);
    
    Clong_t lrint(double x);
    Clong_t lrintf(float x);
    
    long   llrint(double x);
    long   llrintf(float x);
    
    double round(double x);
    float  roundf(float x);
    
    Clong_t lround(double x);
    Clong_t lroundf(float x);
    
    long   llround(double x);
    long   llroundf(float x);
    
    double trunc(double x);
    float  truncf(float x);
    
    double fmod(double x, double y);
    float  fmodf(float x, float y);
    
    double remainder(double x, double y);
    float  remainderf(float x, float y);
    
    double remquo(double x, double y, int *quo);
    float  remquof(float x, float y, int *quo);
    
    double copysign(double x, double y);
    float  copysignf(float x, float y);
    
    double nan(char *tagp);
    float  nanf(char *tagp);
    
    double nextafter(double x, double y);
    float  nextafterf(float x, float y);
    
    double nexttoward(double x, real y);
    float  nexttowardf(float x, real y);
    
    double fdim(double x, double y);
    float  fdimf(float x, float y);
    
    double fmax(double x, double y);
    float  fmaxf(float x, float y);
    
    double fmin(double x, double y);
    float  fminf(float x, float y);
    
    double fma(double x, double y, double z);
    float  fmaf(float x, float y, float z);
    
    private import gcc.config;
    // unfortunately, these do not always exist in a library
    alias gcc.config.acosl acosl;
    alias gcc.config.asinl asinl;
    alias gcc.config.atanl atanl;
    alias gcc.config.atan2l atan2l;
    alias gcc.config.cosl cosl;
    alias gcc.config.sinl sinl;
    alias gcc.config.tanl tanl;
    alias gcc.config.acoshl acoshl;
    alias gcc.config.asinhl asinhl;
    alias gcc.config.atanhl atanhl;
    alias gcc.config.coshl coshl;
    alias gcc.config.sinhl sinhl;
    alias gcc.config.tanhl tanhl;
    alias gcc.config.expl expl;
    alias gcc.config.exp2l exp2l;
    alias gcc.config.expm1l expm1l;
    alias gcc.config.frexpl frexpl;
    alias gcc.config.ilogbl ilogbl;
    alias gcc.config.ldexpl ldexpl;
    alias gcc.config.logl logl;
    alias gcc.config.log10l log10l;
    alias gcc.config.log1pl log1pl;
    alias gcc.config.log2l log2l;
    alias gcc.config.logbl logbl;
    alias gcc.config.modfl modfl;
    alias gcc.config.scalbnl scalbnl;
    alias gcc.config.scalblnl scalblnl;
    alias gcc.config.cbrtl cbrtl;
    alias gcc.config.fabsl fabsl;
    alias gcc.config.hypotl hypotl;
    alias gcc.config.powl powl;
    alias gcc.config.sqrtl sqrtl;
    alias gcc.config.erfl erfl;
    alias gcc.config.erfcl erfcl;
    alias gcc.config.lgammal lgammal;
    alias gcc.config.tgammal tgammal;
    alias gcc.config.ceill ceill;
    alias gcc.config.floorl floorl;
    alias gcc.config.nearbyintl nearbyintl;
    alias gcc.config.rintl rintl;
    alias gcc.config.lrintl lrintl;
    alias gcc.config.llrintl llrintl;
    alias gcc.config.roundl roundl;
    alias gcc.config.lroundl lroundl;
    alias gcc.config.llroundl llroundl;
    alias gcc.config.truncl truncl;
    alias gcc.config.fmodl fmodl;
    alias gcc.config.remainderl remainderl;
    alias gcc.config.remquol remquol;
    alias gcc.config.copysignl copysignl;
    alias gcc.config.nanl nanl;//doesn't work...
    alias gcc.config.nextafterl nextafterl;
    alias gcc.config.nexttowardl nexttowardl;
    alias gcc.config.fdiml fdiml;
    alias gcc.config.fmaxl fmaxl;
    alias gcc.config.fminl fminl;
    alias gcc.config.fmal fmal;
} else {
double acos(double x);	///
float  acosf(float x);	/// ditto
real   acosl(real x);	/// ditto

double asin(double x);	///
float  asinf(float x);	/// ditto
real   asinl(real x);	/// ditto

double atan(double x);	///
float  atanf(float x);	/// ditto
real   atanl(real x);	/// ditto

double atan2(double y, double x);	///
float  atan2f(float y, float x);	/// ditto
real   atan2l(real y, real x);		/// ditto

double cos(double x);	///
float  cosf(float x);	/// ditto
real   cosl(real x);	/// ditto

double sin(double x);	///
float  sinf(float x);	/// ditto
real   sinl(real x);	/// ditto

double tan(double x);	///
float  tanf(float x);	/// ditto
real   tanl(real x);	/// ditto

double acosh(double x);	///
float  acoshf(float x);	/// ditto
real   acoshl(real x);	/// ditto

double asinh(double x);	///
float  asinhf(float x);	/// ditto
real   asinhl(real x);	/// ditto

double atanh(double x);	///
float  atanhf(float x);	/// ditto
real   atanhl(real x);	/// ditto

double cosh(double x);	///
float  coshf(float x);	/// ditto
real   coshl(real x);	/// ditto

double sinh(double x);	///
float  sinhf(float x);	/// ditto
real   sinhl(real x);	/// ditto

double tanh(double x);	///
float  tanhf(float x);	/// ditto
real   tanhl(real x);	/// ditto

double exp(double x);	///
float  expf(float x);	/// ditto
real   expl(real x);	/// ditto

double exp2(double x);	///
float  exp2f(float x);	/// ditto
real   exp2l(real x);	/// ditto

double expm1(double x);	///
float  expm1f(float x);	/// ditto
real   expm1l(real x);	/// ditto

double frexp(double value, int *exp);	///
float  frexpf(float value, int *exp);	/// ditto
real   frexpl(real value, int *exp);	/// ditto

int    ilogb(double x);	///
int    ilogbf(float x);	/// ditto
int    ilogbl(real x);	/// ditto

double ldexp(double x, int exp);	///
float  ldexpf(float x, int exp);	/// ditto
real   ldexpl(real x, int exp);		/// ditto

double log(double x);	///
float  logf(float x);	/// ditto
real   logl(real x);	/// ditto

double log10(double x);	///
float  log10f(float x);	/// ditto
real   log10l(real x);	/// ditto

double log1p(double x);	///
float  log1pf(float x);	/// ditto
real   log1pl(real x);	/// ditto

double log2(double x);	///
float  log2f(float x);	/// ditto
real   log2l(real x);	/// ditto

double logb(double x);	///
float  logbf(float x);	/// ditto
real   logbl(real x);	/// ditto

double modf(double value, double *iptr);	///
float  modff(float value, float *iptr);		/// ditto
real   modfl(real value, real *iptr);		/// ditto

double scalbn(double x, int n);	///
float  scalbnf(float x, int n);	/// ditto
real   scalbnl(real x, int n);	/// ditto

double scalbln(double x, int n);	///
float  scalblnf(float x, int n);	/// ditto
real   scalblnl(real x, int n);		/// ditto

double cbrt(double x);	///
float  cbrtf(float x);	/// ditto
real   cbrtl(real x);	/// ditto

double fabs(double x);	///
float  fabsf(float x);	/// ditto
real   fabsl(real x);	/// ditto

double hypot(double x, double y);	///
float  hypotf(float x, float y);	/// ditto
real   hypotl(real x, real y);		/// ditto

double pow(double x, double y);	///
float  powf(float x, float y);	/// ditto
real   powl(real x, real y);	/// ditto

double sqrt(double x);	///
float  sqrtf(float x);	/// ditto
real   sqrtl(real x);	/// ditto

double erf(double x);	///
float  erff(float x);	/// ditto
real   erfl(real x);	/// ditto

double erfc(double x);	///
float  erfcf(float x);	/// ditto
real   erfcl(real x);	/// ditto

double lgamma(double x);	///
float  lgammaf(float x);	/// ditto
real   lgammal(real x);		/// ditto

double tgamma(double x);	///
float  tgammaf(float x);	/// ditto
real   tgammal(real x);		/// ditto

double ceil(double x);	///
float  ceilf(float x);	/// ditto
real   ceill(real x);	/// ditto

double floor(double x);	///
float  floorf(float x);	/// ditto
real   floorl(real x);	/// ditto

double nearbyint(double x);	///
float  nearbyintf(float x);	/// ditto
real   nearbyintl(real x);	/// ditto

double rint(double x);	///
float  rintf(float x);	/// ditto
real   rintl(real x);	/// ditto

Clong_t lrint(double x);	///
Clong_t lrintf(float x);	/// ditto
Clong_t lrintl(real x);	/// ditto

long   llrint(double x);	///
long   llrintf(float x);	/// ditto
long   llrintl(real x);		/// ditto

double round(double x);	///
float  roundf(float x);	/// ditto
real   roundl(real x);	/// ditto

Clong_t lround(double x);	///
Clong_t lroundf(float x);	/// ditto
Clong_t lroundl(real x);		/// ditto

long   llround(double x);	///
long   llroundf(float x);	/// ditto
long   llroundl(real x);	/// ditto

double trunc(double x);	///
float  truncf(float x);	/// ditto
real   truncl(real x);	/// ditto

double fmod(double x, double y);	///
float  fmodf(float x, float y);		/// ditto
real   fmodl(real x, real y);		/// ditto

double remainder(double x, double y);	///
float  remainderf(float x, float y);	/// ditto
real   remainderl(real x, real y);	/// ditto

double remquo(double x, double y, int *quo);	///
float  remquof(float x, float y, int *quo);	/// ditto
real   remquol(real x, real y, int *quo);	/// ditto

double copysign(double x, double y);	///
float  copysignf(float x, float y);	/// ditto
real   copysignl(real x, real y);	/// ditto

double nan(char *tagp);		///
float  nanf(char *tagp);	/// ditto
real   nanl(char *tagp);	/// ditto

double nextafter(double x, double y);	///
float  nextafterf(float x, float y);	/// ditto
real   nextafterl(real x, real y);	/// ditto

double nexttoward(double x, real y);	///
float  nexttowardf(float x, real y);	/// ditto
real   nexttowardl(real x, real y);	/// ditto

double fdim(double x, double y);	///
float  fdimf(float x, float y);		/// ditto
real   fdiml(real x, real y);		/// ditto

double fmax(double x, double y);	///
float  fmaxf(float x, float y);		/// ditto
real   fmaxl(real x, real y);		/// ditto

double fmin(double x, double y);	///
float  fminf(float x, float y);		/// ditto
real   fminl(real x, real y);		/// ditto

double fma(double x, double y, double z);	///
float  fmaf(float x, float y, float z);		/// ditto
real   fmal(real x, real y, real z);		/// ditto
}

///
int isgreater(real x, real y)		{ return !(x !>  y); }
///
int isgreaterequal(real x, real y)	{ return !(x !>= y); }
///
int isless(real x, real y)		{ return !(x !<  y); }
///
int islessequal(real x, real y)		{ return !(x !<= y); }
///
int islessgreater(real x, real y)	{ return !(x !<> y); }
///
int isunordered(real x, real y)		{ return (x !<>= y); }

