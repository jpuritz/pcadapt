/******************************************************************************/

#include <pcadapt/bed-acc.h>
#include <pcadapt/mat-acc.h>

/******************************************************************************/

template <class C>
ListOf<NumericVector> get_sumX_denoX(C macc, const NumericVector& af) {
  
  size_t n = macc.nrow();
  size_t p = macc.ncol();
  size_t i, j;
  
  double x;
  NumericVector sumX(p);
  NumericVector denoX(p);
  
  for (j = 0; j < p; j++) {
    for (i = 0; i < n; i++) {
      x = macc(i, j);
      if (x != 3) { // Checking a 3 is much faster that checking a NA
        sumX[j]  += x;
        // TODO: replace af with lookup_scale
        denoX[j] += (x - 2 * af[j]) * (x - 2 * af[j]);
      } 
    }
  }
  
  return List::create(_["sumX"] = sumX, _["denoX"] = denoX);
}

/******************************************************************************/

// Dispatch function for get_sumX_denoX
// [[Rcpp::export]]
ListOf<NumericVector> get_sumX_denoX(SEXP obj,
                                     const NumericMatrix& lookup_scale,
                                     const IntegerMatrix& lookup_byte,
                                     const IntegerVector& ind_col,
                                     const NumericVector& af) {
  
  if (Rf_isMatrix(obj)) {
    matAcc macc(obj, lookup_scale, ind_col);
    return get_sumX_denoX(macc, af);
  } else {
    XPtr<bed> xpMat(obj);
    bedAcc macc(xpMat, lookup_scale, lookup_byte, ind_col);
    return get_sumX_denoX(macc, af);
  }
}

/******************************************************************************/

template <class C>
LogicalVector clumping(C macc,
                       const IntegerVector &ord,
                       LogicalVector &remain,
                       const NumericVector &sumX,
                       const NumericVector &denoX,
                       int size, 
                       double thr) {
  
  int n = macc.nrow();
  int p = macc.ncol();
  LogicalVector keep(p);
  
  for (int k = 0; k < p; k++) {
    int j0 = ord[k] - 1; // C++ index
    if (remain[j0]) {
      remain[j0] = false;
      keep[j0] = true;
      int j_min = std::max(0, j0 - size);
      int j_max = std::min(p, j0 + size + 1);
      for (int j = j_min; j < j_max; j++) {
        if (remain[j]) {
          double xySum = 0.0;
          for (int i = 0; i < n; i++) {
            xySum += macc(i, j) * macc(i, j0);  
          }
          double num = xySum - sumX[j] * sumX[j0] / n;
          double r2 = num * num / (denoX[j] * denoX[j0]);
          if (r2 > thr) {
            remain[j] = false;
          }
        }
      }
    }
  }
  
  return(keep);
}

/******************************************************************************/

// Dispatch function for clumping
// [[Rcpp::export]]
LogicalVector clumping(SEXP obj,
                       const NumericMatrix& lookup,
                       const IntegerMatrix& lookup_byte,
                       const IntegerVector& colInd,
                       const IntegerVector& ord,
                       LogicalVector& remain,
                       const NumericVector& sumX,
                       const NumericVector& denoX,
                       int size, 
                       double thr) {
  
  if (Rf_isMatrix(obj)) {
    matAcc macc(obj, lookup, colInd);
    return clumping(macc, ord, remain, sumX, denoX, size, thr);
  } else {
    XPtr<bed> xpMat(obj);
    bedAcc macc(xpMat, lookup, lookup_byte, colInd);
    return clumping(macc, ord, remain, sumX, denoX, size, thr);
  }
}

/******************************************************************************/