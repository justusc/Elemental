/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "elemental-lite.hpp"

#define ColDist VC
#define RowDist STAR

#include "./setup.hpp"

namespace elem {

// Public section
// ##############

// Assignment and reconfiguration
// ==============================

template<typename T>
template<Dist U,Dist V>
BDM&
BDM::operator=( const DistMatrix<T,U,V>& A )
{
    DEBUG_ONLY(CallStackEntry cse("BDM[VC,STAR] = DM[U,V]"))
    BlockDistMatrix<T,U,V> ABlock(A.Grid());
    LockedView( ABlock, A );
    *this = ABlock;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MC,MR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [MC,MR]"))
    this->PartialColAllToAllFrom( A );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MC,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [MC,STAR]"))
    this->PartialColFilterFrom( A );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,MR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,MR]"))
    BlockDistMatrix<T,MC,MR> A_MC_MR( A );
    *this = A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MD,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [MD,STAR]"))
    // TODO: More efficient implementation?
    BlockDistMatrix<T,STAR,STAR> A_STAR_STAR( A );
    *this = A_STAR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,MD>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,MD]"))
    // TODO: More efficient implementation?
    BlockDistMatrix<T,STAR,STAR> A_STAR_STAR( A );
    *this = A_STAR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MR,MC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [MR,MC]"))
    BlockDistMatrix<T,VR,STAR> A_VR_STAR( A );
    *this = A_VR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MR,STAR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [MR,STAR]"))
    BlockDistMatrix<T,VR,STAR> A_VR_STAR( A );
    *this = A_VR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,MC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,MC]"))
    std::unique_ptr<BlockDistMatrix<T,MR,MC>> 
        A_MR_MC( new BlockDistMatrix<T,MR,MC>(A) );
    std::unique_ptr<BlockDistMatrix<T,VR,STAR>> 
        A_VR_STAR( new BlockDistMatrix<T,VR,STAR>(*A_MR_MC) );
    delete A_MR_MC.release(); // lowers memory highwater
    *this = *A_VR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BDM& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [VC,STAR]"))
    A.Translate( *this );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,VC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,VC]"))
    std::unique_ptr<BlockDistMatrix<T,MR,MC>> 
        A_MR_MC( new BlockDistMatrix<T,MR,MC>(A) );
    std::unique_ptr<BlockDistMatrix<T,VR,STAR>> 
        A_VR_STAR( new BlockDistMatrix<T,VR,STAR>(*A_MR_MC) );
    delete A_MR_MC.release(); // lowers memory highwater
    *this = *A_VR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,VR,STAR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [VR,STAR]"))
    LogicError("This routine is not yet written");
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,VR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,VR]"))
    BlockDistMatrix<T,MC,MR> A_MC_MR( A );
    *this = A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [STAR,STAR]"))
    this->ColFilterFrom( A );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,CIRC,CIRC>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[VC,STAR] = [CIRC,CIRC]"))
    LogicError("This routine is not yet written");
    return *this;
}

// Basic queries
// =============

template<typename T>
mpi::Comm BDM::DistComm() const { return this->grid_->VCComm(); }
template<typename T>
mpi::Comm BDM::CrossComm() const { return mpi::COMM_SELF; }
template<typename T>
mpi::Comm BDM::RedundantComm() const { return mpi::COMM_SELF; }
template<typename T>
mpi::Comm BDM::ColComm() const { return this->grid_->VCComm(); }
template<typename T>
mpi::Comm BDM::RowComm() const { return mpi::COMM_SELF; }
template<typename T>
mpi::Comm BDM::PartialColComm() const { return this->grid_->MCComm(); }
template<typename T>
mpi::Comm BDM::PartialUnionColComm() const { return this->grid_->MRComm(); }

template<typename T>
Int BDM::ColStride() const { return this->grid_->VCSize(); }
template<typename T>
Int BDM::RowStride() const { return 1; }
template<typename T>
Int BDM::PartialColStride() const { return this->grid_->MCSize(); }
template<typename T>
Int BDM::PartialUnionColStride() const { return this->grid_->MRSize(); }

// Instantiate {Int,Real,Complex<Real>} for each Real in {float,double}
// ####################################################################

#define PROTO(T) template class BlockDistMatrix<T,ColDist,RowDist>
#define COPY(T,U,V) \
  template BlockDistMatrix<T,ColDist,RowDist>::BlockDistMatrix\
  ( const BlockDistMatrix<T,U,V>& A ); \
  template BlockDistMatrix<T,ColDist,RowDist>::BlockDistMatrix\
  ( const DistMatrix<T,U,V>& A ); \
  template BlockDistMatrix<T,ColDist,RowDist>& \
           BlockDistMatrix<T,ColDist,RowDist>::operator= \
           ( const DistMatrix<T,U,V>& A )
#define FULL(T) \
  PROTO(T); \
  COPY(T,CIRC,CIRC); \
  COPY(T,MC,  MR  ); \
  COPY(T,MC,  STAR); \
  COPY(T,MD,  STAR); \
  COPY(T,MR,  MC  ); \
  COPY(T,MR,  STAR); \
  COPY(T,STAR,MC  ); \
  COPY(T,STAR,MD  ); \
  COPY(T,STAR,MR  ); \
  COPY(T,STAR,STAR); \
  COPY(T,STAR,VC  ); \
  COPY(T,STAR,VR  ); \
  COPY(T,VR,  STAR);

FULL(Int);
#ifndef DISABLE_FLOAT
FULL(float);
#endif
FULL(double);

#ifndef DISABLE_COMPLEX
#ifndef DISABLE_FLOAT
FULL(Complex<float>);
#endif
FULL(Complex<double>);
#endif 

} // namespace elem
