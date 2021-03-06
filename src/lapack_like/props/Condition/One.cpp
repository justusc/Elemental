/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename F> 
Base<F> OneCondition( const Matrix<F>& A )
{
    DEBUG_ONLY(CSE cse("OneCondition"))
    typedef Base<F> Real;
    Matrix<F> B( A );
    const Real oneNorm = OneNorm( B );
    try { Inverse( B ); }
    catch( SingularMatrixException& e ) 
    { return std::numeric_limits<Real>::infinity(); }
    const Real oneNormInv = OneNorm( B );
    return oneNorm*oneNormInv;
}

template<typename F> 
Base<F> OneCondition( const AbstractDistMatrix<F>& A )
{
    DEBUG_ONLY(CSE cse("OneCondition"))
    typedef Base<F> Real;
    DistMatrix<F> B( A );
    const Real oneNorm = OneNorm( B );
    try { Inverse( B ); }
    catch( SingularMatrixException& e ) 
    { return std::numeric_limits<Real>::infinity(); }
    const Real oneNormInv = OneNorm( B );
    return oneNorm*oneNormInv;
}

#define PROTO(F) \
  template Base<F> OneCondition( const Matrix<F>& A ); \
  template Base<F> OneCondition( const AbstractDistMatrix<F>& A );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
