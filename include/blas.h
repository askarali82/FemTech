#ifndef INCLUDE_BLAS_H_
#define INCLUDE_BLAS_H_

extern "C" {
 extern double ddot_(int *n, double *dx, int *incx, double *dy, int *incy);
 extern void dgemm_(char *transa, char *transb, int *m, int *n, int *k, \
     double *alpha, double *a, int *lda, double *b, int *ldb, double *beta, \
     double *c, int *ldc);
}

#endif // INCLUDE_BLAS_H_