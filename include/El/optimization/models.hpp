/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_OPTIMIZATION_MODELS_HPP
#define EL_OPTIMIZATION_MODELS_HPP

namespace El {

namespace RegularizationNS {
enum Regularization {
  NO_PENALTY,
  L1_PENALTY,
  L2_PENALTY
};
}
using namespace RegularizationNS;

// Basis pursuit: min || x ||_1 such that A x = b
// ==============================================
// TODO: Generalize to complex after SOCP support

namespace bp {

template<typename Real>
struct ADMMCtrl {
  Real rho=1;
  Real alpha=1.2;
  Int maxIter=500;
  Real absTol=1e-6;
  Real relTol=1e-4;
  bool usePinv=false;
  Real pinvTol=0;
  bool progress=true;
};

} // namespace bp

template<typename Real>
struct BPCtrl {
  bool useIPM=true;
  bool useSOCP=false;
  // NOTE: The ADMM implementation is still a prototype
  bp::ADMMCtrl<Real> admmCtrl;
  lp::direct::Ctrl<Real> lpIPMCtrl;
  socp::direct::Ctrl<Real> socpIPMCtrl;

  BPCtrl( bool sparse ) : lpIPMCtrl(sparse) { }
};

template<typename Real>
struct BPCtrl<Complex<Real>> {
  socp::direct::Ctrl<Real> ipmCtrl;
  BPCtrl( bool sparse=true ) { }
};

template<typename F>
void BP
( const Matrix<F>& A, 
  const Matrix<F>& b,
        Matrix<F>& x,
  const BPCtrl<F>& ctrl=BPCtrl<F>(false) );
template<typename F>
void BP
( const AbstractDistMatrix<F>& A, 
  const AbstractDistMatrix<F>& b,
        AbstractDistMatrix<F>& x,
  const BPCtrl<F>& ctrl=BPCtrl<F>(false) );
template<typename F>
void BP
( const SparseMatrix<F>& A, 
  const Matrix<F>& b,
        Matrix<F>& x,
  const BPCtrl<F>& ctrl=BPCtrl<F>(true) );
template<typename F>
void BP
( const DistSparseMatrix<F>& A, 
  const DistMultiVec<F>& b,
        DistMultiVec<F>& x,
  const BPCtrl<F>& ctrl=BPCtrl<F>(true) );

// Chebyshev point: min || A x - b||_oo
// ====================================
template<typename Real>
void CP
( const Matrix<Real>& A, const Matrix<Real>& b,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void CP
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b,
        AbstractDistMatrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void CP
( const SparseMatrix<Real>& A, const Matrix<Real>& b,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void CP
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& b,
        DistMultiVec<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );

// Least Absolute Value: min || A x - b||_1
// ========================================
// TODO: Generalize to complex after SOCP support
template<typename Real>
void LAV
( const Matrix<Real>& A, const Matrix<Real>& b,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void LAV
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b,
        AbstractDistMatrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void LAV
( const SparseMatrix<Real>& A, const Matrix<Real>& b,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void LAV
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& b,
        DistMultiVec<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );

// Dantzig selector: min || x ||_1 s.t. || A^T (b - A x) ||_oo <= lambda
// =====================================================================
// TODO: 
//  Add the ability to switch between the (DS1) and (DS2) affine LP
//  formulations described in 
//
//  Michael Friedlander and Michael Saunders,
//  "Discussion: The Dantzig Selector: Statistical estimation when p is much
//   larger than n",
//  The Annals of Statistics, Vol. 35, No. 6, pp. 2385--2391, 2007.
//
// Elemental currently defaults to (DS1) for dense matrices and (DS2) for
// sparse matrices.
//
// TODO: Generalize to complex after SOCP support

template<typename Real>
void DS
( const Matrix<Real>& A, const Matrix<Real>& b,
        Real lambda,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void DS
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b,
        Real lambda,
        AbstractDistMatrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void DS
( const SparseMatrix<Real>& A, const Matrix<Real>& b,
        Real lambda,
        Matrix<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );
template<typename Real>
void DS
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& b,
        Real lambda,
        DistMultiVec<Real>& x,
  const lp::affine::Ctrl<Real>& ctrl=lp::affine::Ctrl<Real>() );

// Fit a model with using a loss function plus regularization
// ==========================================================

template<typename Real>
struct ModelFitCtrl {
  Real rho=1;
  Int maxIter=500;
  bool inv=true;
  bool progress=true;
};

// NOTE: This routine is still a prototype
template<typename Real>
Int ModelFit
( function<void(Matrix<Real>&,Real)> lossProx,
  function<void(Matrix<Real>&,Real)> regProx,
  const Matrix<Real>& A, const Matrix<Real>& b, Matrix<Real>& w,
  const ModelFitCtrl<Real>& ctrl=ModelFitCtrl<Real>() );
template<typename Real>
Int ModelFit
( function<void(DistMatrix<Real>&,Real)> lossProx,
  function<void(DistMatrix<Real>&,Real)> regProx,
  const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b, 
        AbstractDistMatrix<Real>& w,
  const ModelFitCtrl<Real>& ctrl=ModelFitCtrl<Real>() );

// Logistic Regression
// ===================
// NOTE: This routine is still a prototype
// TODO: Use an exponential cone IPM (e.g., that from Santiago Akle's work)

template<typename Real>
Int LogisticRegression
( const Matrix<Real>& G, const Matrix<Real>& q, Matrix<Real>& z,
  Real gamma, Regularization penalty=L1_PENALTY,
  const ModelFitCtrl<Real>& ctrl=ModelFitCtrl<Real>() );
template<typename Real>
Int LogisticRegression
( const AbstractDistMatrix<Real>& G, const AbstractDistMatrix<Real>& q, 
        AbstractDistMatrix<Real>& z,
  Real gamma, Regularization penalty=L1_PENALTY,
  const ModelFitCtrl<Real>& ctrl=ModelFitCtrl<Real>() );

// Non-negative least squares
// ==========================
// TODO: Generalize to complex
// NOTE: The following can solve a *sequence* of NNLS problems

template<typename Real>
struct NNLSCtrl {
  // NOTE: The ADMM implementation is still a prototype
  bool useIPM=true;
  qp::box::ADMMCtrl<Real> admmCtrl;
  qp::direct::Ctrl<Real> ipmCtrl;    
};

template<typename Real>
void NNLS
( const Matrix<Real>& A, const Matrix<Real>& B, 
        Matrix<Real>& X,
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );
template<typename Real>
void NNLS
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& B, 
        AbstractDistMatrix<Real>& X, 
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );
template<typename Real>
void NNLS
( const SparseMatrix<Real>& A, const Matrix<Real>& B, 
        Matrix<Real>& X,
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );
template<typename Real>
void NNLS
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& B, 
        DistMultiVec<Real>& X,
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );

// Non-negative matrix factorization
// =================================
template<typename Real>
void NMF
( const Matrix<Real>& A, 
        Matrix<Real>& X, Matrix<Real>& Y,
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );
template<typename Real>
void NMF
( const AbstractDistMatrix<Real>& A, 
        AbstractDistMatrix<Real>& X, AbstractDistMatrix<Real>& Y,
  const NNLSCtrl<Real>& ctrl=NNLSCtrl<Real>() );
// TODO: Sparse versions

// Basis pursuit denoising (BPDN), a.k.a.,
// Least absolute selection and shrinkage operator (Lasso):
//   min (1/2) || b - A x ||_2^2 + lambda || x ||_1
// ================================================
// TODO: Generalize to complex after SOCP support

namespace bpdn {

template<typename Real>
struct ADMMCtrl {
  Real rho=1;
  Real alpha=1.2;
  Int maxIter=500;
  Real absTol=1e-6;
  Real relTol=1e-4;
  bool inv=true;
  bool progress=true;
};

} // namespace bpdn

template<typename Real>
struct BPDNCtrl {
  bool useIPM=true;
  // NOTE: The ADMM implementation is still a prototype
  bpdn::ADMMCtrl<Real> admmCtrl;
  qp::affine::Ctrl<Real> ipmCtrl;
};

template<typename Real>
void BPDN
( const Matrix<Real>& A, const Matrix<Real>& b, 
        Real lambda,
        Matrix<Real>& x,
  const BPDNCtrl<Real>& ctrl=BPDNCtrl<Real>() );
template<typename Real>
void BPDN
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b,
        Real lambda,
        AbstractDistMatrix<Real>& x,
  const BPDNCtrl<Real>& ctrl=BPDNCtrl<Real>() );
template<typename Real>
void BPDN
( const SparseMatrix<Real>& A, const Matrix<Real>& b,
        Real lambda,
        Matrix<Real>& x,
  const BPDNCtrl<Real>& ctrl=BPDNCtrl<Real>() );
template<typename Real>
void BPDN
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& b,
        Real lambda,
        DistMultiVec<Real>& x,
  const BPDNCtrl<Real>& ctrl=BPDNCtrl<Real>() );

// Elastic net (EN): 
//   min || b - A x ||_2^2 + lambda_1 || x ||_1 + lambda_2 || x ||_2^2
// ===================================================================
// TODO: Generalize to complex after SOCP support

template<typename Real>
void EN
( const Matrix<Real>& A, const Matrix<Real>& b, 
        Real lambda1,          Real lambda2,
        Matrix<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );
template<typename Real>
void EN
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& b,
        Real lambda1,                      Real lambda2,
        AbstractDistMatrix<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );
template<typename Real>
void EN
( const SparseMatrix<Real>& A, const Matrix<Real>& b,
        Real lambda1,                Real lambda2,
        Matrix<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );
template<typename Real>
void EN
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& b,
        Real lambda1,                    Real lambda2,
        DistMultiVec<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );

// Robust Principal Component Analysis (RPCA)
// ==========================================

template<typename Real>
struct RPCACtrl
{
    bool useALM=true;
    bool usePivQR=false;
    bool progress=true;

    Int numPivSteps=75;
    Int maxIts=1000;

    Real tau=0;
    Real beta=1;
    Real rho=6;
    Real tol=1e-5;
};

template<typename F>
void RPCA
( const Matrix<F>& M, Matrix<F>& L, Matrix<F>& S,
  const RPCACtrl<Base<F>>& ctrl=RPCACtrl<Base<F>>() );

template<typename F>
void RPCA
( const AbstractDistMatrix<F>& M, AbstractDistMatrix<F>& L, 
        AbstractDistMatrix<F>& S,
  const RPCACtrl<Base<F>>& ctrl=RPCACtrl<Base<F>>() );

// Sparse inverse covariance selection
// ===================================
template<typename Real>
struct SparseInvCovCtrl
{
    Real rho=1.;
    Real alpha=1.2;
    Int maxIter=500;
    Real absTol=1e-6;
    Real relTol=1e-4;
    bool progress=true;
};

template<typename F>
Int SparseInvCov
( const Matrix<F>& D, Base<F> lambda, Matrix<F>& Z,
  const SparseInvCovCtrl<Base<F>>& ctrl=SparseInvCovCtrl<Base<F>>() );
template<typename F>
Int SparseInvCov
( const AbstractDistMatrix<F>& D, Base<F> lambda, AbstractDistMatrix<F>& Z,
  const SparseInvCovCtrl<Base<F>>& ctrl=SparseInvCovCtrl<Base<F>>() );

// Support Vector Machine (soft-margin)
// ====================================
// TODO: Use the formulation described in 
//       <http://abel.ee.ucla.edu/cvxopt/applications/svm/>
//
// min_{w,beta,z} (1/2) || w ||_2^2 + lambda 1^T z
//
// s.t. | -diag(d) A, -d, -I | | w    | <= | -1 |
//      |       0,     0, -I | | beta |    |  0 |
//                             | z    |
//
// The output, x, is set to the concatenation of w and beta, x := [w; beta].
//

template<typename Real>
struct SVMCtrl 
{
    bool useIPM=true;
    ModelFitCtrl<Real> modelFitCtrl;
    qp::affine::Ctrl<Real> ipmCtrl; 
};

// TODO: Switch to explicitly returning w, beta, and z, as it is difficult
//       for users to unpack a DistMultiVec
template<typename Real>
void SVM
( const Matrix<Real>& A, const Matrix<Real>& d, 
        Real lambda,           Matrix<Real>& x,
  const SVMCtrl<Real>& ctrl=SVMCtrl<Real>() );
template<typename Real>
void SVM
( const AbstractDistMatrix<Real>& A, const AbstractDistMatrix<Real>& d, 
        Real lambda,                       AbstractDistMatrix<Real>& x,
  const SVMCtrl<Real>& ctrl=SVMCtrl<Real>() );
template<typename Real>
void SVM
( const SparseMatrix<Real>& A, const Matrix<Real>& d, 
        Real lambda,                 Matrix<Real>& x,
  const SVMCtrl<Real>& ctrl=SVMCtrl<Real>() );
template<typename Real>
void SVM
( const DistSparseMatrix<Real>& A, const DistMultiVec<Real>& d, 
        Real lambda,                     DistMultiVec<Real>& x,
  const SVMCtrl<Real>& ctrl=SVMCtrl<Real>() );

// 1D total variation denoising (TV):
//
//   min (1/2) || b - x ||_2^2 + lambda || D x ||_1,
//
// where D is the 1D finite-difference operator.
// =================================================
// We follow the formulation used within CVXOPT:
//
//   min (1/2) || b - x ||_2^2 + lambda 1^T y
//   s.t. -y <= D x <= y,
//
// where x is in R^n and y is in R^(n-1).
//
// TODO: Generalize to complex after SOCP support

template<typename Real>
void TV
( const AbstractDistMatrix<Real>& b,
        Real lambda,
        AbstractDistMatrix<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );
template<typename Real>
void TV
( const Matrix<Real>& b,
        Real lambda,
        Matrix<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );
template<typename Real>
void TV
( const DistMultiVec<Real>& b,
        Real lambda,
        DistMultiVec<Real>& x,
  const qp::affine::Ctrl<Real>& ctrl=qp::affine::Ctrl<Real>() );

} // namespace El

#endif // ifndef EL_OPTIMIZATION_MODELS_HPP
