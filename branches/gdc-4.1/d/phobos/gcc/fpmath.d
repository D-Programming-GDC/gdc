// Implementation may be internal/fpmath.d or gcc/cbridge_math.c
module gcc.fpmath;

int signbit(float f);
int signbit(double f);
int signbit(real f);

int fpclassify(float f);
int fpclassify(double f);
int fpclassify(real f);
