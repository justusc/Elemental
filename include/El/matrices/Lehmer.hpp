/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_LEHMER_HPP
#define EL_LEHMER_HPP

namespace El {

template<typename F> 
inline void
Lehmer( Matrix<F>& L, Int n )
{
    DEBUG_ONLY(CallStackEntry cse("Lehmer"))
    L.Resize( n, n );
    IndexDependentFill
    ( L, []( Int i, Int j ) 
         { if( i < j ) { return F(i+1)/F(j+1); }
           else        { return F(j+1)/F(i+1); } } );
}

template<typename F>
inline void
Lehmer( AbstractDistMatrix<F>& L, Int n )
{
    DEBUG_ONLY(CallStackEntry cse("Lehmer"))
    L.Resize( n, n );
    IndexDependentFill
    ( L, []( Int i, Int j ) 
         { if( i < j ) { return F(i+1)/F(j+1); }
           else        { return F(j+1)/F(i+1); } } );
}

template<typename F>
inline void
Lehmer( AbstractBlockDistMatrix<F>& L, Int n )
{
    DEBUG_ONLY(CallStackEntry cse("Lehmer"))
    L.Resize( n, n );
    IndexDependentFill
    ( L, []( Int i, Int j ) 
         { if( i < j ) { return F(i+1)/F(j+1); }
           else        { return F(j+1)/F(i+1); } } );
}

} // namespace El

#endif // ifndef EL_LEHMER_HPP
