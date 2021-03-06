/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

#include "../util.hpp"

namespace El {
namespace socp {
namespace affine {

//
// Despite the fact that the CVXOPT documentation [1] suggests a single-stage
// procedure for initializing (x,y,z,s), a post-processed two-stage procedure 
// is currently used by the code [2]:
//
// 1) Minimize || G x - h ||^2, s.t. A x = b  by solving
//
//    | 0 A^T G^T | |  x |   | 0 |
//    | A  0   0  | |  u | = | b |,
//    | G  0  -I  | | -s |   | h |
//
//   where 'u' is an unused dummy variable.
//
// 2) Minimize || z ||^2, s.t. A^T y + G^T z + c = 0 by solving
//
//    | 0 A^T G^T | | u |   | -c |
//    | A  0   0  | | y | = |  0 |,
//    | G  0  -I  | | z |   |  0 |
//
//    where 'u' is an unused dummy variable.
//
// 3) Set 
//
//      alpha_p := -min_i min eig(s_i),
//      alpha_d := -min_i min eig(z_i),
//
//    where min eig(s_i) is the minimum (Jordan) eigenvalue of s restricted to
//    its i'th subcone.
//
//    Then shift s and z according to the rules:
//
//      s := ( alpha_p > -sqrt(eps)*Max(1,||s||_2) ? s + (1+alpha_p)e : s )
//      z := ( alpha_d > -sqrt(eps)*Max(1,||z||_2) ? z + (1+alpha_d)e : z ),
//
//    where 'eps' is the machine precision and 'e' is the identity of the 
//    product cone.
//
//    TODO: 
//    Since the post-processing in step (3) has a large discontinuity as the 
//    minimum entry approaches sqrt(eps)*Max(1,||q||_2), we can also provide
//    the ability to instead use an per-subcone lower clip.
//
// [1] L. Vandenberghe
//     "The CVXOPT linear and quadratic cone program solvers"
//     <http://www.seas.ucla.edu/~vandenbe/publications/coneprog.pdf>
//
// [2] L. Vandenberghe
//     CVXOPT's source file, "src/python/coneprog.py"
//     <https://github.com/cvxopt/cvxopt/blob/f3ca94fb997979a54b913f95b816132f7fd44820/src/python/coneprog.py>
//

template<typename Real>
void Initialize
( const Matrix<Real>& A, 
  const Matrix<Real>& G,
  const Matrix<Real>& b, 
  const Matrix<Real>& c,
  const Matrix<Real>& h,
  const Matrix<Int>& orders,
  const Matrix<Int>& firstInds,
  const Matrix<Int>& labels,
        Matrix<Real>& x, 
        Matrix<Real>& y,
        Matrix<Real>& z, 
        Matrix<Real>& s,
  bool primalInit, bool dualInit, bool standardShift )
{
    DEBUG_ONLY(CSE cse("socp::affine::Initialize"))
    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = G.Height();
    if( primalInit )
    {
        if( x.Height() != n || x.Width() != 1 )
            LogicError("x was of the wrong size");
        if( s.Height() != k || s.Width() != 1 )
            LogicError("s was of the wrong size");
    }
    if( dualInit )
    {
        if( y.Height() != m || y.Width() != 1 )
            LogicError("y was of the wrong size");
        if( z.Height() != k || z.Width() != 1 )
            LogicError("z was of the wrong size");
    }
    if( primalInit && dualInit )
    {
        // TODO: Perform a consistency check
        return;
    }

    // Form the KKT matrix
    // ===================
    Matrix<Real> J, e;
    SOCIdentity( e, orders, firstInds );
    KKT( A, G, e, orders, firstInds, labels, J );

    // Factor the KKT matrix
    // =====================
    Matrix<Real> dSub;
    Matrix<Int> p;
    LDL( J, dSub, p, false );

    // w should be equal to the identity element, and so should sqrt(w)
    Matrix<Real> rc, rb, rh, rmu, u, d;
    Zeros( rmu, k, 1 );
    if( !primalInit )
    {
        // Minimize || G x - h ||^2, s.t. A x = b  by solving
        //
        //    | 0 A^T G^T | |  x |   | 0 |
        //    | A  0   0  | |  u | = | b |,
        //    | G  0  -I  | | -s |   | h |
        //
        //   where 'u' is an unused dummy variable.
        Zeros( rc, n, 1 );
        rb = b;
        Scale( Real(-1), rb );
        rh = h;
        Scale( Real(-1), rh );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d );
        ldl::SolveAfter( J, dSub, p, d, false );
        qp::affine::ExpandCoreSolution( m, n, k, d, x, u, s );
        Scale( Real(-1), s );
    }
    if( !dualInit )
    {
        // Minimize || z ||^2, s.t. A^T y + G^T z + c = 0 by solving
        //
        //    | 0 A^T G^T | | u |   | -c |
        //    | A  0   0  | | y | = |  0 |,
        //    | G  0  -I  | | z |   |  0 |
        //
        //    where 'u' is an unused dummy variable.
        rc = c;
        Zeros( rb, m, 1 );
        Zeros( rh, k, 1 );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d );
        ldl::SolveAfter( J, dSub, p, d, false );
        qp::affine::ExpandCoreSolution( m, n, k, d, u, y, z );
    }

    const Real epsilon = Epsilon<Real>();
    const Real sNorm = Nrm2( s );
    const Real zNorm = Nrm2( z );
    const Real gammaPrimal = Sqrt(epsilon)*Max(sNorm,Real(1));
    const Real gammaDual   = Sqrt(epsilon)*Max(zNorm,Real(1));
    if( standardShift )
    {
        // alpha_p := min { alpha : s + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaPrimal = -SOCMinEig( s, orders, firstInds );
        if( alphaPrimal >= Real(0) && primalInit )
            RuntimeError("initialized s was non-positive");

        // alpha_d := min { alpha : z + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaDual = -SOCMinEig( z, orders, firstInds );
        if( alphaDual >= Real(0) && dualInit )
            RuntimeError("initialized z was non-positive");

        if( alphaPrimal >= -gammaPrimal )
            SOCShift( s, alphaPrimal+1, orders, firstInds );
        if( alphaDual >= -gammaDual )
            SOCShift( z, alphaDual+1, orders, firstInds );
    }
    else
    {
        ForceIntoSOC( s, orders, firstInds, gammaPrimal );
        ForceIntoSOC( z, orders, firstInds, gammaDual );
    }
}

template<typename Real>
void Initialize
( const AbstractDistMatrix<Real>& A, 
  const AbstractDistMatrix<Real>& G,
  const AbstractDistMatrix<Real>& b, 
  const AbstractDistMatrix<Real>& c,
  const AbstractDistMatrix<Real>& h,
  const AbstractDistMatrix<Int>& orders,
  const AbstractDistMatrix<Int>& firstInds,
  const AbstractDistMatrix<Int>& labels,
        AbstractDistMatrix<Real>& x, 
        AbstractDistMatrix<Real>& y,
        AbstractDistMatrix<Real>& z, 
        AbstractDistMatrix<Real>& s,
  bool primalInit, bool dualInit, bool standardShift,
  Int cutoff )
{
    DEBUG_ONLY(CSE cse("socp::affine::Initialize"))
    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = G.Height();
    const Grid& g = A.Grid();
    if( primalInit ) 
    {
        if( x.Height() != n || x.Width() != 1 )
            LogicError("x was of the wrong size");
        if( s.Height() != k || s.Width() != 1 )
            LogicError("s was of the wrong size");
    }
    if( dualInit )
    {
        if( y.Height() != m || y.Width() != 1 )
            LogicError("y was of the wrong size");
        if( z.Height() != k || z.Width() != 1 )
            LogicError("z was of the wrong size");
    }
    if( primalInit && dualInit )
    {
        // TODO: Perform a consistency check
        return;
    }

    const bool onlyLower = true;

    // Form the KKT matrix
    // ===================
    DistMatrix<Real> J(g), e(g);
    SOCIdentity( e, orders, firstInds );
    KKT( A, G, e, orders, firstInds, labels, J, onlyLower, cutoff );

    // Factor the KKT matrix
    // =====================
    DistMatrix<Real> dSub(g);
    DistMatrix<Int> p(g);
    LDL( J, dSub, p, false );

    DistMatrix<Real> rc(g), rb(g), rh(g), rmu(g), d(g), u(g);
    Zeros( rmu, k, 1 );
    if( !primalInit )
    {
        // Minimize || G x - h ||^2, s.t. A x = b  by solving
        //
        //    | 0 A^T G^T | |  x |   | 0 |
        //    | A  0   0  | |  u | = | b |,
        //    | G  0  -I  | | -s |   | h |
        //
        //   where 'u' is an unused dummy variable.
        Zeros( rc, n, 1 );
        rb = b;
        Scale( Real(-1), rb );
        rh = h;
        Scale( Real(-1), rh );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d, cutoff );
        ldl::SolveAfter( J, dSub, p, d, false );
        qp::affine::ExpandCoreSolution( m, n, k, d, x, u, s );
        Scale( Real(-1), s );
    }
    if( !dualInit )
    {
        // Minimize || z ||^2, s.t. A^T y + G^T z + c = 0 by solving
        //
        //    | 0 A^T G^T | | u |   | -c |
        //    | A  0   0  | | y | = |  0 |,
        //    | G  0  -I  | | z |   |  0 |
        //
        //    where 'u' is an unused dummy variable.
        rc = c;
        Zeros( rb, m, 1 );
        Zeros( rh, k, 1 );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d, cutoff );
        ldl::SolveAfter( J, dSub, p, d, false );
        qp::affine::ExpandCoreSolution( m, n, k, d, u, y, z );
    }

    const Real epsilon = Epsilon<Real>();
    const Real sNorm = Nrm2( s );
    const Real zNorm = Nrm2( z );
    const Real gammaPrimal = Sqrt(epsilon)*Max(sNorm,Real(1));
    const Real gammaDual   = Sqrt(epsilon)*Max(zNorm,Real(1));
    if( standardShift )
    {
        // alpha_p := min { alpha : s + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaPrimal = -SOCMinEig( s, orders, firstInds, cutoff );
        if( alphaPrimal >= Real(0) && primalInit )
            RuntimeError("initialized s was non-positive");

        // alpha_d := min { alpha : z + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaDual = -SOCMinEig( z, orders, firstInds, cutoff );
        if( alphaDual >= Real(0) && dualInit )
            RuntimeError("initialized z was non-positive");

        if( alphaPrimal >= -gammaPrimal )
            SOCShift( s, alphaPrimal+1, orders, firstInds );
        if( alphaDual >= -gammaDual )
            SOCShift( z, alphaDual+1, orders, firstInds );
    }
    else
    {
        ForceIntoSOC( s, orders, firstInds, gammaPrimal, cutoff );
        ForceIntoSOC( z, orders, firstInds, gammaDual, cutoff );
    }
}

template<typename Real>
void Initialize
( const SparseMatrix<Real>& A, 
  const SparseMatrix<Real>& G,
  const Matrix<Real>& b, 
  const Matrix<Real>& c,
  const Matrix<Real>& h,
  const Matrix<Int>& orders,
  const Matrix<Int>& firstInds,
  const Matrix<Int>& labels,
        Matrix<Real>& x, 
        Matrix<Real>& y,
        Matrix<Real>& z, 
        Matrix<Real>& s,
        vector<Int>& map, 
        vector<Int>& invMap, 
        ldl::Separator& rootSep, 
        ldl::NodeInfo& info,
  bool primalInit, bool dualInit, bool standardShift,
  const RegQSDCtrl<Real>& qsdCtrl )
{
    DEBUG_ONLY(CSE cse("socp::affine::Initialize"))
    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = G.Height();
    if( primalInit ) 
    {
        if( x.Height() != n || x.Width() != 1 )
            LogicError("x was of the wrong size");
        if( s.Height() != k || s.Width() != 1 )
            LogicError("s was of the wrong size");
    }
    if( dualInit )
    {
        if( y.Height() != m || y.Width() != 1 )
            LogicError("y was of the wrong size");
        if( z.Height() != k || z.Width() != 1 )
            LogicError("z was of the wrong size");
    }
    if( primalInit && dualInit )
    {
        // TODO: Perform a consistency check
        return;
    }

    // Form the KKT matrix
    // ===================
    SparseMatrix<Real> JOrig;
    Matrix<Real> e;
    SOCIdentity( e, orders, firstInds );
    KKT( A, G, e, orders, firstInds, labels, JOrig, false );
    auto J = JOrig;

    // (Approximately) factor the KKT matrix
    // =====================================
    Matrix<Real> reg;
    reg.Resize( n+m+k, 1 );
    for( Int i=0; i<reg.Height(); ++i )
    {
        if( i < n )
            reg.Set( i, 0, qsdCtrl.regPrimal );
        else
            reg.Set( i, 0, -qsdCtrl.regDual );
    }
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    NestedDissection( J.LockedGraph(), map, rootSep, info );
    InvertMap( map, invMap );

    ldl::Front<Real> JFront;
    JFront.Pull( J, map, info );
    LDL( info, JFront );

    Matrix<Real> rc, rb, rh, rmu, u, d;
    Zeros( rmu, k, 1 );
    if( !primalInit )
    {
        // Minimize || G x - h ||^2, s.t. A x = b  by solving
        //
        //    | 0 A^T G^T | |  x |   | 0 |
        //    | A  0   0  | |  u | = | b |,
        //    | G  0  -I  | | -s |   | h |
        //
        //   where 'u' is an unused dummy variable.
        Zeros( rc, n, 1 );
        rb = b;
        Scale( Real(-1), rb );
        rh = h;
        Scale( Real(-1), rh );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d );

        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, d, qsdCtrl );
        qp::affine::ExpandCoreSolution( m, n, k, d, x, u, s );
        Scale( Real(-1), s );
    }
    if( !dualInit )
    {
        // Minimize || z ||^2, s.t. A^T y + G^T z + c = 0 by solving
        //
        //    | 0 A^T G^T | | u |   | -c |
        //    | A  0   0  | | y | = |  0 |,
        //    | G  0  -I  | | z |   |  0 |
        //
        //    where 'u' is an unused dummy variable.
        rc = c;
        Zeros( rb, m, 1 );
        Zeros( rh, k, 1 );
        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d );

        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, d, qsdCtrl );
        qp::affine::ExpandCoreSolution( m, n, k, d, u, y, z );
    }

    const Real epsilon = Epsilon<Real>();
    const Real sNorm = Nrm2( s );
    const Real zNorm = Nrm2( z );
    const Real gammaPrimal = Sqrt(epsilon)*Max(sNorm,Real(1));
    const Real gammaDual   = Sqrt(epsilon)*Max(zNorm,Real(1));
    if( standardShift )
    {
        // alpha_p := min { alpha : s + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaPrimal = -SOCMinEig( s, orders, firstInds );
        if( alphaPrimal >= Real(0) && primalInit )
            RuntimeError("initialized s was non-positive");

        // alpha_d := min { alpha : z + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaDual = -SOCMinEig( z, orders, firstInds );
        if( alphaDual >= Real(0) && dualInit )
            RuntimeError("initialized z was non-positive");

        if( alphaPrimal >= -gammaPrimal )
            SOCShift( s, alphaPrimal+1, orders, firstInds );
        if( alphaDual >= -gammaDual )
            SOCShift( z, alphaDual+1, orders, firstInds );
    }
    else
    {
        ForceIntoSOC( s, orders, firstInds, gammaPrimal );
        ForceIntoSOC( z, orders, firstInds, gammaDual );
    }
}

template<typename Real>
void Initialize
( const DistSparseMatrix<Real>& A, 
  const DistSparseMatrix<Real>& G,
  const DistMultiVec<Real>& b, 
  const DistMultiVec<Real>& c,
  const DistMultiVec<Real>& h,
  const DistMultiVec<Int>& orders,
  const DistMultiVec<Int>& firstInds,
  const DistMultiVec<Int>& labels,
        DistMultiVec<Real>& x, 
        DistMultiVec<Real>& y,
        DistMultiVec<Real>& z, 
        DistMultiVec<Real>& s,
        DistMap& map,
        DistMap& invMap, 
        ldl::DistSeparator& rootSep, 
        ldl::DistNodeInfo& info,
  bool primalInit, bool dualInit, bool standardShift, Int cutoff,
  const RegQSDCtrl<Real>& qsdCtrl )
{
    DEBUG_ONLY(CSE cse("socp::affine::Initialize"))
    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = G.Height();
    mpi::Comm comm = A.Comm();
    if( primalInit ) 
    {
        if( x.Height() != n || x.Width() != 1 )
            LogicError("x was of the wrong size");
        if( s.Height() != k || s.Width() != 1 )
            LogicError("s was of the wrong size");
    }
    if( dualInit )
    {
        if( y.Height() != m || y.Width() != 1 )
            LogicError("y was of the wrong size");
        if( z.Height() != k || z.Width() != 1 )
            LogicError("z was of the wrong size");
    }
    if( primalInit && dualInit )
    {
        // TODO: Perform a consistency check
        return;
    }

    const bool onlyLower = false;

    // Form the KKT matrix
    // ===================
    DistSparseMatrix<Real> JOrig(comm);
    DistMultiVec<Real> e(comm);
    SOCIdentity( e, orders, firstInds );
    KKT( A, G, e, orders, firstInds, labels, JOrig, onlyLower, cutoff );
    auto J = JOrig;

    // (Approximately) factor the KKT matrix
    // =====================================
    DistMultiVec<Real> reg(comm);
    reg.Resize( n+m+k, 1 );
    for( Int iLoc=0; iLoc<reg.LocalHeight(); ++iLoc )
    {
        const Int i = reg.FirstLocalRow() + iLoc;
        if( i < n )
            reg.SetLocal( iLoc, 0, qsdCtrl.regPrimal );
        else
            reg.SetLocal( iLoc, 0, -qsdCtrl.regDual );
    }
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    NestedDissection( J.LockedDistGraph(), map, rootSep, info );
    InvertMap( map, invMap );

    ldl::DistFront<Real> JFront;
    JFront.Pull( J, map, rootSep, info );
    LDL( info, JFront, LDL_2D );

    DistMultiVec<Real> rc(comm), rb(comm), rh(comm), rmu(comm), u(comm),
                       d(comm);
    Zeros( rmu, k, 1 );
    if( !primalInit )
    {
        // Minimize || G x - h ||^2, s.t. A x = b  by solving
        //
        //    | 0 A^T G^T | |  x |   | 0 |
        //    | A  0   0  | |  u | = | b |,
        //    | G  0  -I  | | -s |   | h |
        //
        //   where 'u' is an unused dummy variable.
        Zeros( rc, n, 1 );
        rb = b;
        Scale( Real(-1), rb );
        rh = h;
        Scale( Real(-1), rh );

        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d, cutoff );
        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, d, qsdCtrl );
        qp::affine::ExpandCoreSolution( m, n, k, d, x, u, s );
        Scale( Real(-1), s );
    }
    if( !dualInit )
    {
        // Minimize || z ||^2, s.t. A^T y + G^T z + c = 0 by solving
        //
        //    | 0 A^T G^T | | u |   | -c |
        //    | A  0   0  | | y | = |  0 |,
        //    | G  0  -I  | | z |   |  0 |
        //
        //    where 'u' is an unused dummy variable.
        rc = c;
        Zeros( rb, m, 1 );
        Zeros( rh, k, 1 );

        KKTRHS( rc, rb, rh, rmu, e, orders, firstInds, labels, d, cutoff );
        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, d, qsdCtrl );
        qp::affine::ExpandCoreSolution( m, n, k, d, u, y, z );
    }

    const Real epsilon = Epsilon<Real>();
    const Real sNorm = Nrm2( s );
    const Real zNorm = Nrm2( z );
    const Real gammaPrimal = Sqrt(epsilon)*Max(sNorm,Real(1));
    const Real gammaDual   = Sqrt(epsilon)*Max(zNorm,Real(1));
    if( standardShift )
    {
        // alpha_p := min { alpha : s + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaPrimal = -SOCMinEig( s, orders, firstInds, cutoff );
        if( alphaPrimal >= Real(0) && primalInit )
            RuntimeError("initialized s was non-positive");

        // alpha_d := min { alpha : z + alpha*e >= 0 }
        // -------------------------------------------
        const Real alphaDual = -SOCMinEig( z, orders, firstInds, cutoff );
        if( alphaDual >= Real(0) && dualInit )
            RuntimeError("initialized z was non-positive");

        if( alphaPrimal >= -gammaPrimal )
            SOCShift( s, alphaPrimal+1, orders, firstInds );
        if( alphaDual >= -gammaDual )
            SOCShift( z, alphaDual+1, orders, firstInds );
    }
    else
    {
        ForceIntoSOC( s, orders, firstInds, gammaPrimal, cutoff );
        ForceIntoSOC( z, orders, firstInds, gammaDual, cutoff );
    }
}

#define PROTO(Real) \
  template void Initialize \
  ( const Matrix<Real>& A, \
    const Matrix<Real>& G, \
    const Matrix<Real>& b, \
    const Matrix<Real>& c, \
    const Matrix<Real>& h, \
    const Matrix<Int>& orders, \
    const Matrix<Int>& firstInds, \
    const Matrix<Int>& labels, \
          Matrix<Real>& x, \
          Matrix<Real>& y, \
          Matrix<Real>& z, \
          Matrix<Real>& s, \
    bool primalInit, bool dualInit, bool standardShift ); \
  template void Initialize \
  ( const AbstractDistMatrix<Real>& A, \
    const AbstractDistMatrix<Real>& G, \
    const AbstractDistMatrix<Real>& b, \
    const AbstractDistMatrix<Real>& c, \
    const AbstractDistMatrix<Real>& h, \
    const AbstractDistMatrix<Int>& orders, \
    const AbstractDistMatrix<Int>& firstInds, \
    const AbstractDistMatrix<Int>& labels, \
          AbstractDistMatrix<Real>& x, \
          AbstractDistMatrix<Real>& y, \
          AbstractDistMatrix<Real>& z, \
          AbstractDistMatrix<Real>& s, \
    bool primalInit, bool dualInit, bool standardShift, Int cutoff ); \
  template void Initialize \
  ( const SparseMatrix<Real>& A, \
    const SparseMatrix<Real>& G, \
    const Matrix<Real>& b, \
    const Matrix<Real>& c, \
    const Matrix<Real>& h, \
    const Matrix<Int>& orders, \
    const Matrix<Int>& firstInds, \
    const Matrix<Int>& labels, \
          Matrix<Real>& x, \
          Matrix<Real>& y, \
          Matrix<Real>& z, \
          Matrix<Real>& s, \
          vector<Int>& map, \
          vector<Int>& invMap, \
          ldl::Separator& rootSep, \
          ldl::NodeInfo& info, \
    bool primalInit, bool dualInit, bool standardShift, \
    const RegQSDCtrl<Real>& qsdCtrl ); \
  template void Initialize \
  ( const DistSparseMatrix<Real>& A, \
    const DistSparseMatrix<Real>& G, \
    const DistMultiVec<Real>& b, \
    const DistMultiVec<Real>& c, \
    const DistMultiVec<Real>& h, \
    const DistMultiVec<Int>& orders, \
    const DistMultiVec<Int>& firstInds, \
    const DistMultiVec<Int>& labels, \
          DistMultiVec<Real>& x, \
          DistMultiVec<Real>& y, \
          DistMultiVec<Real>& z, \
          DistMultiVec<Real>& s, \
          DistMap& map, \
          DistMap& invMap, \
          ldl::DistSeparator& rootSep, \
          ldl::DistNodeInfo& info, \
    bool primalInit, bool dualInit, bool standardShift, Int cutoff,\
    const RegQSDCtrl<Real>& qsdCtrl );

#define EL_NO_INT_PROTO
#define EL_NO_COMPLEX_PROTO
#include "El/macros/Instantiate.h"

} // namespace affine
} // namespace socp
} // namespace El
