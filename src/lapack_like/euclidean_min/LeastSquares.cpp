/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

namespace ls {

template<typename F> 
void Overwrite
( Orientation orientation, 
  Matrix<F>& A, const Matrix<F>& B, 
  Matrix<F>& X )
{
    DEBUG_ONLY(CSE cse("ls::Overwrite"))

    Matrix<F> t;
    Matrix<Base<F>> d;

    const Int m = A.Height();
    const Int n = A.Width();
    if( m >= n )
    {
        QR( A, t, d );
        qr::SolveAfter( orientation, A, t, d, B, X );
    }
    else
    {
        LQ( A, t, d );
        lq::SolveAfter( orientation, A, t, d, B, X );
    }
}

template<typename F> 
void Overwrite
( Orientation orientation, 
  AbstractDistMatrix<F>& APre, const AbstractDistMatrix<F>& B, 
  AbstractDistMatrix<F>& X )
{
    DEBUG_ONLY(CSE cse("ls::Overwrite"))

    auto APtr = ReadProxy<F,MC,MR>( &APre );
    auto& A = *APtr;

    DistMatrix<F,MD,STAR> t(A.Grid());
    DistMatrix<Base<F>,MD,STAR> d(A.Grid());

    const Int m = A.Height();
    const Int n = A.Width();
    if( m >= n )
    {
        QR( A, t, d );
        qr::SolveAfter( orientation, A, t, d, B, X );
    }
    else
    {
        LQ( A, t, d );
        lq::SolveAfter( orientation, A, t, d, B, X );
    }
}

} // namespace ls

template<typename F> 
void LeastSquares
( Orientation orientation, 
  const Matrix<F>& A, const Matrix<F>& B, 
        Matrix<F>& X )
{
    DEBUG_ONLY(CSE cse("LeastSquares"))
    Matrix<F> ACopy( A );
    ls::Overwrite( orientation, ACopy, B, X );
}

template<typename F> 
void LeastSquares
( Orientation orientation, 
  const AbstractDistMatrix<F>& A, const AbstractDistMatrix<F>& B, 
        AbstractDistMatrix<F>& X )
{
    DEBUG_ONLY(CSE cse("LeastSquares"))
    DistMatrix<F> ACopy( A );
    ls::Overwrite( orientation, ACopy, B, X ); 
}

// The following routines solve either
//
//   Minimum length: 
//     min_X || X ||_F 
//     s.t. W X = B, or
//
//   Least squares:  
//     min_X || W X - B ||_F,
//
// where W=op(A) is either A, A^T, or A^H, via forming a Hermitian 
// quasi-semidefinite system 
//
//    | alpha*I  W | | R/alpha | = | B |,
//    |   W^H    0 | | X       |   | 0 |
//
// when height(W) >= width(W), or
//
//    | alpha*I  W^H | |     X   | = | 0 |,
//    |   W       0  | | alpha*Y |   | B |
//
// when height(W) < width(W).
//  
// The latter guarantees that W X = B and X in range(W^H), which shows that 
// X solves the minimum length problem. The former defines R = B - W X and 
// ensures that R is in the null-space of W^H (therefore solving the least 
// squares problem).
// 
// Note that, ideally, alpha is roughly the minimum (nonzero) singular value
// of W, which implies that the condition number of the quasi-semidefinite 
// system is roughly equal to the condition number of W (see the analysis of 
// Bjorck). If it is too expensive to estimate the minimum singular value, and 
// W is equilibrated to have a unit two-norm, a typical choice for alpha is 
// epsilon^0.25.
//
// The Hermitian quasi-semidefinite systems are solved by converting them into
// Hermitian quasi-definite form via a priori regularization, applying an 
// LDL^H factorization with static pivoting to the regularized system, and
// using the iteratively-refined solution of with the regularized factorization
// as a preconditioner for the original problem (defaulting to Flexible GMRES
// for now).
//
// This approach originated within 
//
//    Michael Saunders, 
//   "Chapter 8, Cholesky-based Methods for Sparse Least Squares:
//    The Benefits of Regularization",
//    in L. Adams and J.L. Nazareth (eds.), Linear and Nonlinear Conjugate
//    Gradient-Related Methods, SIAM, Philadelphia, 92--100 (1996).
//
// But note that SymmLQ and LSQR were used rather than flexible GMRES, and 
// iteratively refining *within* the preconditioner was not discussed.
//

// NOTE: The following routines are implemented as a special case of Tikhonov
//       regularization with either an m x 0 or 0 x n regularization matrix.

namespace ls {

template<typename F>
inline void Equilibrated
( const SparseMatrix<F>& A,  const Matrix<F>& B, 
        Matrix<F>& X,
  Base<F> alpha, const RegQSDCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(
      CSE cse("ls::Equilibrated");
      if( A.Height() != B.Height() )
          LogicError("Heights of A and B must match");
    )
    typedef Base<F> Real;

    const Int m = A.Height();
    const Int n = A.Width();
    const Int numRHS = B.Width();
    const Int numEntriesA = A.NumEntries();

    SparseMatrix<F> J;
    Zeros( J, m+n, m+n );
    J.Reserve( 2*numEntriesA + Max(m,n) );
    if( m >= n )
    {
        // Form J = [alpha*I, A; A^H, 0]
        // =============================
        for( Int e=0; e<numEntriesA; ++e )
        {
            J.QueueUpdate( A.Row(e),   A.Col(e)+m,      A.Value(e)  );
            J.QueueUpdate( A.Col(e)+m, A.Row(e),   Conj(A.Value(e)) );
        }
        for( Int e=0; e<m; ++e )
            J.QueueUpdate( e, e, alpha );
    }
    else
    {
        // Form J = [alpha, A^H; A, 0]
        // ===========================
        for( Int e=0; e<numEntriesA; ++e )
        {
            J.QueueUpdate( A.Col(e),   A.Row(e)+n, Conj(A.Value(e)) );
            J.QueueUpdate( A.Row(e)+n, A.Col(e),        A.Value(e)  );
        }
        for( Int e=0; e<n; ++e )
            J.QueueUpdate( e, e, alpha );
    }
    J.ProcessQueues();

    Matrix<F> D;
    Zeros( D, m+n, numRHS );
    if( m >= n )
    {
        // Form D = [B; 0]
        // ==================
        auto DT = D( IR(0,m), ALL );
        DT = B;
    }
    else
    {
        // Form D = [0; B]
        // ===============
        auto DB = D( IR(n,m+n), ALL );
        DB = B;
    }

    // Compute the regularized quasi-semidefinite fact of J
    // ====================================================
    Matrix<Real> reg;
    reg.Resize( m+n, 1 );
    for( Int i=0; i<Max(m,n); ++i )
        reg.Set( i, 0, ctrl.regPrimal );
    for( Int i=Max(m,n); i<m+n; ++i )
        reg.Set( i, 0, -ctrl.regDual );
    SparseMatrix<F> JOrig;
    JOrig = J;
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    vector<Int> map, invMap;
    ldl::NodeInfo info;
    ldl::Separator rootSep;
    ldl::NestedDissection( J.LockedGraph(), map, rootSep, info );
    InvertMap( map, invMap );
    ldl::Front<F> JFront( J, map, info );
    LDL( info, JFront );

    // Successively solve each of the linear systems
    // =============================================
    // TODO: Extend the iterative refinement to handle multiple RHS
    Matrix<F> u;
    Zeros( u, m+n, 1 );
    for( Int j=0; j<numRHS; ++j )
    {
        auto d = D( ALL, IR(j) );
        u = d;
        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, u, ctrl );
        d = u;
    }

    Zeros( X, n, numRHS );
    if( m >= n )
    {
        // Extract X from [R/alpha; X]
        // ===========================
        auto DB = D( IR(m,m+n), ALL );
        X = DB;
    }
    else
    {
        // Extract X from [X; alpha*Y]
        // ===========================
        auto DT = D( IR(0,n), ALL );
        X = DT;
    }
}

} // namespace ls

template<typename F>
void LeastSquares
( Orientation orientation,
  const SparseMatrix<F>& A, const Matrix<F>& B, 
        Matrix<F>& X,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(CSE cse("LeastSquares"))
    typedef Base<F> Real;

    SparseMatrix<F> ABar;
    if( orientation == NORMAL )
        ABar = A;
    else if( orientation == TRANSPOSE )
        Transpose( A, ABar );
    else
        Adjoint( A, ABar );
    auto BBar = B;
    const Int m = ABar.Height();
    const Int n = ABar.Width();

    // Equilibrate the matrix
    // ======================
    Matrix<Real> dR, dC;
    if( ctrl.equilibrate )
    {
        if( m >= n )
        {
            ColumnNorms( ABar, dC );
            DiagonalSolve( RIGHT, NORMAL, dC, ABar );
            Ones( dR, m, 1 );
        }
        else
        {
            RowNorms( ABar, dR );
            DiagonalSolve( LEFT, NORMAL, dR, ABar );
            Ones( dC, n, 1 );
        }
    }
    else
    {
        Ones( dR, m, 1 );
        Ones( dC, n, 1 );
    }
    Real normScale = 1;
    if( ctrl.scaleTwoNorm )
    {
        // Scale ABar down to roughly unit two-norm
        normScale = TwoNormEstimate( ABar, ctrl.basisSize ); 
        if( ctrl.progress )
            cout << "Estimated || A ||_2 ~= " << normScale << endl;
        Scale( F(1)/normScale, ABar );
        Scale( normScale, dR );
    }

    // Equilibrate the RHS
    // ===================
    DiagonalSolve( LEFT, NORMAL, dR, BBar );

    // Solve the equilibrated least squares problem
    // ============================================
    ls::Equilibrated( ABar, BBar, X, ctrl.alpha, ctrl.qsdCtrl );

    // Unequilibrate the solution
    // ==========================
    DiagonalSolve( LEFT, NORMAL, dC, X );
}

namespace ls {

template<typename F>
void Equilibrated
( const DistSparseMatrix<F>& A,  const DistMultiVec<F>& B, 
        DistMultiVec<F>& X,
  Base<F> alpha, const RegQSDCtrl<Base<F>>& ctrl, bool time )
{
    DEBUG_ONLY(
      CSE cse("ls::Equilibrated");
      if( A.Height() != B.Height() )
          LogicError("Heights of A and B must match");
    )
    typedef Base<F> Real;
    mpi::Comm comm = A.Comm();
    const int commSize = mpi::Size(comm);
    const int commRank = mpi::Rank(comm);
    Timer timer;

    const Int m = A.Height();
    const Int n = A.Width();
    const Int numRHS = B.Width();

    // J := [alpha*I,A;A^H,0] or [alpha*I,A^H;A,0]
    // ===========================================
    DistSparseMatrix<F> J(comm);
    Zeros( J, m+n, m+n );
    // Count the number of local alpha updates
    // ---------------------------------------
    Int numAlphaUpdates = 0;
    for( Int iLoc=0; iLoc<J.LocalHeight(); ++iLoc )
    {
        const Int i = J.GlobalRow(iLoc);
        if( i < Max(m,n) )
            ++numAlphaUpdates;
        else
            break;
    }
    const Int numLocalEntriesA = A.NumLocalEntries();
    {
        // Compute metadata
        // ----------------
        vector<int> sendCounts(commSize,0);
        for( Int e=0; e<numLocalEntriesA; ++e )
        {
            const Int i = A.Row(e);
            const Int j = A.Col(e);
            if( m >= n )
            {
                // Sending A
                ++sendCounts[ J.RowOwner(i) ];
                // Sending A^H
                ++sendCounts[ J.RowOwner(j+m) ];
            }
            else
            {
                // Sending A
                ++sendCounts[ J.RowOwner(i+n) ];
                // Sending A^H
                ++sendCounts[ J.RowOwner(j) ];
            }
        }
        // Pack
        // ----
        vector<int> sendOffs;
        const int totalSend = Scan( sendCounts, sendOffs );
        vector<Entry<F>> sendBuf(totalSend);
        auto offs = sendOffs;
        for( Int e=0; e<numLocalEntriesA; ++e )
        {
            const Int i = A.Row(e);
            const Int j = A.Col(e);
            const F value = A.Value(e);

            if( m >= n )
            {
                // Sending A
                sendBuf[offs[J.RowOwner(i)]++] = Entry<F>{i,j+m,value};
                // Sending A^H
                sendBuf[offs[J.RowOwner(j+m)]++] = Entry<F>{j+m,i,Conj(value)};
            }
            else
            {
                // Sending A
                sendBuf[offs[J.RowOwner(i+n)]++] = Entry<F>{i+n,j,value};
                // Sending A^H
                sendBuf[offs[J.RowOwner(j)]++] = Entry<F>{j,i+n,Conj(value)};
            }
        }
        // Exchange and unpack
        // -------------------
        auto recvBuf = mpi::AllToAll( sendBuf, sendCounts, sendOffs, comm );
        J.Reserve( recvBuf.size()+numAlphaUpdates );
        for( Int iLoc=0; iLoc<J.LocalHeight(); ++iLoc )
        {
            const Int i = J.GlobalRow(iLoc);
            if( i < Max(m,n) )
                J.QueueUpdate( i, i, alpha );
            else
                break;
        }
        for( auto& entry : recvBuf )
            J.QueueUpdate( entry );
        J.ProcessQueues();
    }

    // Set D to [B; 0] or [0; B]
    // =========================
    DistMultiVec<F> D(comm);
    Zeros( D, m+n, numRHS );
    {
        // Compute metadata
        // ----------------
        vector<int> sendCounts(commSize,0);
        for( Int iLoc=0; iLoc<B.LocalHeight(); ++iLoc )
        {
            const Int i = B.GlobalRow(iLoc);
            if( m >= n )
                sendCounts[ D.RowOwner(i) ] += numRHS;
            else
                sendCounts[ D.RowOwner(i+n) ] += numRHS;
        }
        // Pack
        // ----
        vector<int> sendOffs;
        const int totalSend = Scan( sendCounts, sendOffs );
        vector<Entry<F>> sendBuf(totalSend);
        auto offs = sendOffs;
        for( Int iLoc=0; iLoc<B.LocalHeight(); ++iLoc )
        {
            const Int i = B.GlobalRow(iLoc);
            if( m >= n )
            {
                int owner = D.RowOwner(i);
                for( Int j=0; j<numRHS; ++j )
                {
                    const F value = B.GetLocal(iLoc,j);
                    sendBuf[offs[owner]++] = Entry<F>{i,j,value};
                }
            }
            else
            {
                int owner = D.RowOwner(i+n);
                for( Int j=0; j<numRHS; ++j )
                {
                    const F value = B.GetLocal(iLoc,j);
                    sendBuf[offs[owner]++] = Entry<F>{i+n,j,value};
                }
            }
        }
        // Exchange and unpack
        // -------------------
        auto recvBuf = mpi::AllToAll( sendBuf, sendCounts, sendOffs, comm );
        for( auto& entry : recvBuf )
            D.Update( entry );
    }

    // Compute the regularized quasi-semidefinite fact of J
    // ====================================================
    DistMultiVec<Real> reg(comm);
    reg.Resize( m+n, 1 );
    for( Int iLoc=0; iLoc<reg.LocalHeight(); ++iLoc )
    {
        const Int i = reg.GlobalRow(iLoc);
        if( i < Max(m,n) )
            reg.SetLocal( iLoc, 0, ctrl.regPrimal );
        else
            reg.SetLocal( iLoc, 0, -ctrl.regDual );
    }
    DistSparseMatrix<F> JOrig(comm);
    JOrig = J;
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    DistMap map, invMap;
    ldl::DistNodeInfo info;
    ldl::DistSeparator rootSep;
    if( commRank == 0 && time )
        timer.Start();
    ldl::NestedDissection( J.LockedDistGraph(), map, rootSep, info );
    if( commRank == 0 && time )
        cout << "  ND: " << timer.Stop() << " secs" << endl;
    InvertMap( map, invMap );
    ldl::DistFront<F> JFront( J, map, rootSep, info );

    if( commRank == 0 && time )
        timer.Start();
    LDL( info, JFront, LDL_2D );
    if( commRank == 0 && time )
        cout << "  LDL: " << timer.Stop() << " secs" << endl;

    // Successively solve each of the k linear systems
    // ===============================================
    // TODO: Extend the iterative refinement to handle multiple right-hand sides
    DistMultiVec<F> u(comm);
    Zeros( u, m+n, 1 );
    auto& DLoc = D.Matrix();
    auto& uLoc = u.Matrix();
    if( commRank == 0 && time )
        timer.Start();
    for( Int j=0; j<numRHS; ++j )
    {
        auto dLoc = DLoc( ALL, IR(j) );
        Copy( dLoc, uLoc );
        reg_qsd_ldl::SolveAfter( JOrig, reg, invMap, info, JFront, u, ctrl );
        Copy( uLoc, dLoc );
    }
    if( commRank == 0 && time )
        cout << "  Solve: " << timer.Stop() << " secs" << endl;

    // Extract X from [R/alpha; X] or [X; alpha*Y] and then rescale
    // ============================================================
    if( m >= n )
        X = D( IR(m,m+n), ALL );
    else
        X = D( IR(0,n),   ALL );
}

} // namespace ls

template<typename F>
void LeastSquares
( Orientation orientation,
  const DistSparseMatrix<F>& A, const DistMultiVec<F>& B, 
        DistMultiVec<F>& X,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(CSE cse("LeastSquares"))
    typedef Base<F> Real;
    mpi::Comm comm = A.Comm();
    const int commRank = mpi::Rank(comm);

    DistSparseMatrix<F> ABar(comm);
    if( orientation == NORMAL )
        ABar = A;
    else if( orientation == TRANSPOSE )
        Transpose( A, ABar );
    else
        Adjoint( A, ABar );
    auto BBar = B;
    const Int m = ABar.Height();
    const Int n = ABar.Width();

    // Equilibrate the matrix
    // ======================
    DistMultiVec<Real> dR(comm), dC(comm);
    if( ctrl.equilibrate )
    {
        if( m >= n )
        {
            ColumnNorms( ABar, dC );
            DiagonalSolve( RIGHT, NORMAL, dC, ABar );
            Ones( dR, m, 1 );
        }
        else
        {
            RowNorms( ABar, dR );
            DiagonalSolve( LEFT, NORMAL, dR, ABar );
            Ones( dC, n, 1 );
        }
    }
    else
    {
        Ones( dR, m, 1 );
        Ones( dC, n, 1 );
    } 
    Real normScale = 1;
    if( ctrl.scaleTwoNorm )
    {
        // Scale ABar down to roughly unit two-norm
        normScale = TwoNormEstimate( ABar, ctrl.basisSize );
        if( ctrl.progress && commRank == 0 )
            cout << "Estimated || A ||_2 ~= " << normScale << endl;
        Scale( F(1)/normScale, ABar );
        Scale( normScale, dR );
    }

    // Equilibrate the RHS
    // ===================
    DiagonalSolve( LEFT, NORMAL, dR, BBar );

    // Solve the equilibrated least squares problem
    // ============================================
    ls::Equilibrated( ABar, BBar, X, ctrl.alpha, ctrl.qsdCtrl, ctrl.time );

    // Unequilibrate the solution
    // ==========================
    DiagonalSolve( LEFT, NORMAL, dC, X );
}

#define PROTO(F) \
  template void ls::Overwrite \
  ( Orientation orientation, Matrix<F>& A, const Matrix<F>& B, \
    Matrix<F>& X ); \
  template void ls::Overwrite \
  ( Orientation orientation, AbstractDistMatrix<F>& A, \
    const AbstractDistMatrix<F>& B, AbstractDistMatrix<F>& X ); \
  template void LeastSquares \
  ( Orientation orientation, const Matrix<F>& A, const Matrix<F>& B, \
    Matrix<F>& X ); \
  template void LeastSquares \
  ( Orientation orientation, const AbstractDistMatrix<F>& A, \
    const AbstractDistMatrix<F>& B, AbstractDistMatrix<F>& X ); \
  template void LeastSquares \
  ( Orientation orientation, \
    const SparseMatrix<F>& A, const Matrix<F>& B, \
    Matrix<F>& X, const LeastSquaresCtrl<Base<F>>& ctrl ); \
  template void LeastSquares \
  ( Orientation orientation, \
    const DistSparseMatrix<F>& A, const DistMultiVec<F>& B, \
    DistMultiVec<F>& X, const LeastSquaresCtrl<Base<F>>& ctrl );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
