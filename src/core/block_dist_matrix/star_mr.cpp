/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "elemental-lite.hpp"

#define ColDist STAR
#define RowDist MR

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
    DEBUG_ONLY(CallStackEntry cse("BDM[STAR,MR] = DM[U,V]"))
    BlockDistMatrix<T,U,V> ABlock(A.Grid());
    LockedView( ABlock, A );
    *this = ABlock;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MC,MR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [MC,MR]"))
    A.ColAllGather( *this );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MC,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [MC,STAR]"))
    BlockDistMatrix<T,MC,MR> A_MC_MR(this->Grid());
    A_MC_MR.AlignRowsWith(*this);
    A_MC_MR = A;
    *this = A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BDM& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,MR]"))
    A.Translate( *this );
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MD,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [MD,STAR]"))
    // TODO: More efficient implementation?
    BlockDistMatrix<T,STAR,STAR> A_STAR_STAR( A );
    *this = A_STAR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,MD>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,MD]"))
    // TODO: More efficient implementation?
    BlockDistMatrix<T,STAR,STAR> A_STAR_STAR( A );
    *this = A_STAR_STAR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MR,MC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [MR,MC]"))
    std::unique_ptr<BlockDistMatrix<T,STAR,VC>> A_STAR_VC
    ( new BlockDistMatrix<T,STAR,VC>(A) );

    std::unique_ptr<BlockDistMatrix<T,STAR,VR>> A_STAR_VR
    ( new BlockDistMatrix<T,STAR,VR>(this->Grid()) );
    A_STAR_VR->AlignRowsWith(*this);
    *A_STAR_VR = *A_STAR_VC;
    delete A_STAR_VC.release(); // lowers memory highwater

    *this = *A_STAR_VR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,MR,STAR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [MR,STAR]"))
    std::unique_ptr<BlockDistMatrix<T,VR,STAR>> A_VR_STAR
    ( new BlockDistMatrix<T,VR,STAR>(A) );

    std::unique_ptr<BlockDistMatrix<T,VC,STAR>> A_VC_STAR
    ( new BlockDistMatrix<T,VC,STAR>(*A_VR_STAR) );
    delete A_VR_STAR.release(); // lowers memory highwater

    std::unique_ptr<BlockDistMatrix<T,MC,MR>> A_MC_MR
    ( new BlockDistMatrix<T,MC,MR>(this->Grid()) );
    A_MC_MR->AlignRowsWith(*this);
    *A_MC_MR = *A_VC_STAR;
    delete A_VC_STAR.release(); // lowers memory highwater

    *this = *A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,MC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,MC]"))
    std::unique_ptr<BlockDistMatrix<T,STAR,VC>> A_STAR_VC
    ( new BlockDistMatrix<T,STAR,VC>(A) );

    std::unique_ptr<BlockDistMatrix<T,STAR,VR>> A_STAR_VR
    ( new BlockDistMatrix<T,STAR,VR>(this->Grid()) );
    A_STAR_VR->AlignRowsWith(*this);
    *A_STAR_VR = *A_STAR_VC;
    delete A_STAR_VC.release(); // lowers memory highwater

    std::unique_ptr<BlockDistMatrix<T,MC,MR>> A_MC_MR
    ( new BlockDistMatrix<T,MC,MR>(*A_STAR_VR) );
    delete A_STAR_VR.release(); // lowers memory highwater

    *this = *A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,VC,STAR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [VC,STAR]"))
    BlockDistMatrix<T,MC,MR> A_MC_MR(this->Grid());
    A_MC_MR.AlignRowsWith(*this);
    A_MC_MR = A;
    *this = A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,VC>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,VC]"))
    BlockDistMatrix<T,STAR,VR> A_STAR_VR(this->Grid());
    A_STAR_VR.AlignRowsWith(*this);
    A_STAR_VR = A;
    *this = A_STAR_VR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,VR,STAR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [VR,STAR]"))
    std::unique_ptr<BlockDistMatrix<T,VC,STAR>> A_VC_STAR
    ( new BlockDistMatrix<T,VC,STAR>(A) );

    std::unique_ptr<BlockDistMatrix<T,MC,MR>> A_MC_MR
    ( new BlockDistMatrix<T,MC,MR>(this->Grid()) );
    A_MC_MR->AlignRowsWith(*this);
    *A_MC_MR = *A_VC_STAR;
    delete A_VC_STAR.release(); // lowers memory highwater

    *this = *A_MC_MR;
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,VR>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,VR]"))
    A.PartialRowAllGather(*this);
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,STAR,STAR>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [STAR,STAR]"))
    this->RowFilterFrom(A);
    return *this;
}

template<typename T>
BDM&
BDM::operator=( const BlockDistMatrix<T,CIRC,CIRC>& A )
{
    DEBUG_ONLY(CallStackEntry cse("[STAR,MR] = [CIRC,CIRC]"))
    BlockDistMatrix<T,MC,MR> A_MC_MR( A );
    A_MC_MR.AlignWith( *this );
    A_MC_MR = A;
    *this = A_MC_MR;
    return *this;
}

// Basic queries
// =============

template<typename T>
mpi::Comm BDM::DistComm() const { return this->grid_->MRComm(); }
template<typename T>
mpi::Comm BDM::CrossComm() const { return mpi::COMM_SELF; }
template<typename T>
mpi::Comm BDM::RedundantComm() const { return this->grid_->MCComm(); }
template<typename T>
mpi::Comm BDM::ColComm() const { return mpi::COMM_SELF; }
template<typename T>
mpi::Comm BDM::RowComm() const { return this->grid_->MRComm(); }

template<typename T>
Int BDM::ColStride() const { return 1; }
template<typename T>
Int BDM::RowStride() const { return this->grid_->MRSize(); }

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
  COPY(T,STAR,STAR); \
  COPY(T,STAR,VC  ); \
  COPY(T,STAR,VR  ); \
  COPY(T,VC,  STAR); \
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
