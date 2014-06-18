/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_TOEPLITZ_HPP
#define EL_TOEPLITZ_HPP

namespace El {

template<typename S,typename T> 
inline void
Toeplitz( Matrix<S>& A, Int m, Int n, const std::vector<T>& a )
{
    DEBUG_ONLY(CallStackEntry cse("Toeplitz"))
    const Int length = m+n-1;
    if( a.size() != Unsigned(length) )
        LogicError("a was the wrong size");
    A.Resize( m, n );
    IndexDependentFill( A, [&]( Int i, Int j ) { return a[i-j+(n-1)]; } );
}

template<typename S,typename T>
inline void
Toeplitz( AbstractDistMatrix<S>& A, Int m, Int n, const std::vector<T>& a )
{
    DEBUG_ONLY(CallStackEntry cse("Toeplitz"))
    const Int length = m+n-1;
    if( a.size() != Unsigned(length) )
        LogicError("a was the wrong size");
    A.Resize( m, n );
    IndexDependentFill( A, [&]( Int i, Int j ) { return a[i-j+(n-1)]; } );
}

template<typename S,typename T>
inline void
Toeplitz( AbstractBlockDistMatrix<S>& A, Int m, Int n, const std::vector<T>& a )
{
    DEBUG_ONLY(CallStackEntry cse("Toeplitz"))
    const Int length = m+n-1;
    if( a.size() != Unsigned(length) )
        LogicError("a was the wrong size");
    A.Resize( m, n );
    IndexDependentFill( A, [&]( Int i, Int j ) { return a[i-j+(n-1)]; } );
}

} // namespace El

#endif // ifndef EL_TOEPLITZ_HPP
