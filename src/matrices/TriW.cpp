/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename T> 
void TriW( Matrix<T>& A, Int n, T alpha, Int k )
{
    DEBUG_ONLY(CSE cse("TriW"))
    if( k < 0 )
        LogicError("Number of superdiagonals must be non-negative");
    Zeros( A, n, n );
    FillDiagonal( A, T(1) );
    for( Int j=0; j<Min(n-1,k); ++j ) 
        FillDiagonal( A, alpha, j+1 );
}

template<typename T>
void TriW( AbstractDistMatrix<T>& A, Int n, T alpha, Int k )
{
    DEBUG_ONLY(CSE cse("TriW"))
    if( k < 0 )
        LogicError("Number of superdiagonals must be non-negative");
    Zeros( A, n, n );
    FillDiagonal( A, T(1) );
    for( Int j=0; j<Min(n-1,k); ++j )
        FillDiagonal( A, alpha, j+1 );
}

template<typename T>
void TriW( AbstractBlockDistMatrix<T>& A, Int n, T alpha, Int k )
{
    DEBUG_ONLY(CSE cse("TriW"))
    if( k < 0 )
        LogicError("Number of superdiagonals must be non-negative");
    Zeros( A, n, n );
    FillDiagonal( A, T(1) );
    for( Int j=0; j<Min(n-1,k); ++j )
        FillDiagonal( A, alpha, j+1 );
}

#define PROTO(T) \
  template void TriW( Matrix<T>& A, Int n, T alpha, Int k ); \
  template void TriW( AbstractDistMatrix<T>& A, Int n, T alpha, Int k ); \
  template void TriW( AbstractBlockDistMatrix<T>& A, Int n, T alpha, Int k );

#define EL_ENABLE_QUAD
#include "El/macros/Instantiate.h"

} // namespace El
