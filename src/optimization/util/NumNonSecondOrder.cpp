/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

// Members of second-order cones are stored contiguously within the column
// vector x, with the corresponding order of the cone each member belongs to
// stored in the same index of 'order', and the first index of the cone 
// being listed in the same index of 'firstInd'.
template<typename Real>
Int NumNonSecondOrder
( const Matrix<Real>& x, 
  const Matrix<Int>& orders, const Matrix<Int>& firstInds )
{
    DEBUG_ONLY(CallStackEntry cse("NumNonSecondOrder"))
    const Int height = x.Height();
    if( x.Width() != 1 || orders.Width() != 1 || firstInds.Width() != 1 ) 
        LogicError("x, orders, and firstInds should be column vectors");
    if( orders.Height() != height || firstInds.Height() != height )
        LogicError("orders and firstInds should be of the same height as x");

    Int numNonSO = 0;
    Int i = 0;
    while( i < height )
    {
        // This should be the root of a second-order cone
        if( i != firstInds.Get(i,0) )
            LogicError("Inconsistency in orders and firstInds");
        const Int order = orders.Get(i,0);
        const Real root = x.Get(i,0);
        const Real vecNrm = Nrm2( x(IR(i+1,i+order),IR(0,1)) );
        if( root < vecNrm )
            ++numNonSO;
        i += order;
    }
    return numNonSO;
}

template<typename Real>
Int NumNonSecondOrder
( const AbstractDistMatrix<Real>& xPre, 
  const AbstractDistMatrix<Int>& ordersPre, 
  const AbstractDistMatrix<Int>& firstIndsPre,
  Int cutoff )
{
    DEBUG_ONLY(CallStackEntry cse("NumNonSecondOrder"))
    AssertSameGrids( xPre, ordersPre, firstIndsPre );

    ProxyCtrl ctrl;
    ctrl.colConstrain = true;
    ctrl.colAlign = 0;

    auto xPtr = ReadProxy<Real,VC,STAR>(&xPre,ctrl); 
    auto& x = *xPtr;

    auto ordersPtr = ReadProxy<Int,VC,STAR>(&ordersPre,ctrl); 
    auto& orders = *ordersPtr;

    auto firstIndsPtr = ReadProxy<Int,VC,STAR>(&firstIndsPre,ctrl);
    auto& firstInds = *firstIndsPtr;

    const Int height = x.Height();
    if( x.Width() != 1 || orders.Width() != 1 || firstInds.Width() != 1 ) 
        LogicError("x, orders, and firstInds should be column vectors");
    if( orders.Height() != height || firstInds.Height() != height )
        LogicError("orders and firstInds should be of the same height as x");

    const Int localHeight = x.LocalHeight();
    mpi::Comm comm = x.DistComm();
    int commSize = mpi::Size(comm);

    // Perform an mpi::AllToAll to collect all of the second-order cones of
    // order less than or equal to the cutoff at the root locations and 
    // individually handle the remainder 
    // TODO: Find a better strategy

    // Handle all second-order cones with order <= cutoff
    // ==================================================
    Int numLocalNonSO = 0;
    // Compute the send and recv counts and offsets
    // --------------------------------------------
    std::vector<int> sendCounts(commSize,0), recvCounts(commSize,0);
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i == firstInd )
        {
            for( Int k=1; k<order; ++k )
                ++recvCounts[x.RowOwner(i+k)];
        }
        else
        {
            ++sendCounts[x.RowOwner(firstInd)];
        }
    }
    std::vector<int> sendOffsets, recvOffsets;
    int totalSend = Scan( sendCounts, sendOffsets );
    int totalRecv = Scan( recvCounts, recvOffsets );
    // Pack the entries 
    // ----------------
    std::vector<Real> sendBuf(totalSend);
    auto offsets = sendOffsets;
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i != firstInd )
            sendBuf[offsets[x.RowOwner(firstInd)]++];
    }
    // Exchange entries
    // ----------------
    std::vector<Real> recvBuf(totalRecv); 
    mpi::AllToAll 
    ( sendBuf.data(), sendCounts.data(), sendOffsets.data(),
      recvBuf.data(), recvCounts.data(), recvOffsets.data(), comm );
    // Check the cone constraints
    // --------------------------
    offsets = recvOffsets;
    std::vector<Real> socBuf(cutoff-1);
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i == firstInd )
        {
            const Real t = x.GetLocal(iLoc,0);
            for( Int k=1; k<order; ++k )
                socBuf[k-1] = recvBuf[offsets[x.RowOwner(i+k)]++];
            const Real botNrm = blas::Nrm2( order-1, socBuf.data(), 1 );
            if( t < botNrm )
                ++numLocalNonSO;
        }
    }
    Int numNonSO = mpi::AllReduce( numLocalNonSO, comm );

    // Handle all of the second-order cones with order > cutoff
    // ========================================================
    // Allgather the list of cones with sufficiently large order
    // ---------------------------------------------------------
    std::vector<Real> sendCaps;
    std::vector<Int> sendCones;
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( order > cutoff && i == firstInd )
        {
            sendCaps.push_back(x.GetLocal(iLoc,0));
            sendCones.push_back(i);
        }
    }
    int numSendCones = sendCones.size();
    std::vector<int> numRecvCones(commSize);
    mpi::AllToAll( &numSendCones, 1, numRecvCones.data(), 1, comm );
    totalRecv = Scan( numRecvCones, recvOffsets );
    std::vector<Real> recvCaps(totalRecv);
    std::vector<Int> recvCones(totalRecv);
    mpi::AllGather
    ( sendCaps.data(), numSendCones,
      recvCaps.data(), numRecvCones.data(), recvOffsets.data(), comm );
    mpi::AllGather
    ( sendCones.data(), numSendCones,
      recvCones.data(), numRecvCones.data(), recvOffsets.data(), comm );
    // TODO: Perform a sequence of mpi::AllReduce calls to simultaneously
    //       safely compute the bottom vector norms
    LogicError("This routine is not yet finished");

    return numNonSO;
}

template<typename Real>
Int NumNonSecondOrder
( const DistMultiVec<Real>& x, 
  const DistMultiVec<Int>& orders, 
  const DistMultiVec<Int>& firstInds, Int cutoff )
{
    DEBUG_ONLY(CallStackEntry cse("NumNonSecondOrder"))

    // TODO: Check that the communicators are congruent
    mpi::Comm comm = x.Comm();
    const int commSize = mpi::Size(comm);
    const int localHeight = x.LocalHeight();

    const Int height = x.Height();
    if( x.Width() != 1 || orders.Width() != 1 || firstInds.Width() != 1 ) 
        LogicError("x, orders, and firstInds should be column vectors");
    if( orders.Height() != height || firstInds.Height() != height )
        LogicError("orders and firstInds should be of the same height as x");

    // Perform an mpi::AllToAll to collect all of the second-order cones of
    // order less than or equal to the cutoff at the root locations and 
    // individually handle the remainder 
    // TODO: Find a better strategy

    // Handle all second-order cones with order <= cutoff
    // ==================================================
    Int numLocalNonSO = 0;
    // Compute the send and recv counts and offsets
    // --------------------------------------------
    std::vector<int> sendCounts(commSize,0), recvCounts(commSize,0);
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i == firstInd )
        {
            for( Int k=1; k<order; ++k )
                ++recvCounts[x.RowOwner(i+k)];
        }
        else
        {
            ++sendCounts[x.RowOwner(firstInd)];
        }
    }
    std::vector<int> sendOffsets, recvOffsets;
    int totalSend = Scan( sendCounts, sendOffsets );
    int totalRecv = Scan( recvCounts, recvOffsets );
    // Pack the entries 
    // ----------------
    std::vector<Real> sendBuf(totalSend);
    auto offsets = sendOffsets;
    for( Int iLoc=0; iLoc<x.LocalHeight(); ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i != firstInd )
            sendBuf[offsets[x.RowOwner(firstInd)]++];
    }
    // Exchange entries
    // ----------------
    std::vector<Real> recvBuf(totalRecv); 
    mpi::AllToAll 
    ( sendBuf.data(), sendCounts.data(), sendOffsets.data(),
      recvBuf.data(), recvCounts.data(), recvOffsets.data(), comm );
    // Check the cone constraints
    // --------------------------
    offsets = recvOffsets;
    std::vector<Real> socBuf(cutoff-1);
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        if( order > cutoff )
            continue;

        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( i == firstInd )
        {
            const Real t = x.GetLocal(iLoc,0);
            for( Int k=1; k<order; ++k )
                socBuf[k-1] = recvBuf[offsets[x.RowOwner(i+k)]++];
            const Real botNrm = blas::Nrm2( order-1, socBuf.data(), 1 );
            if( t < botNrm )
                ++numLocalNonSO;
        }
    }
    Int numNonSO = mpi::AllReduce( numLocalNonSO, comm );

    // Handle all of the second-order cones with order > cutoff
    // ========================================================
    // Allgather the list of cones with sufficiently large order
    // ---------------------------------------------------------
    std::vector<Real> sendCaps;
    std::vector<Int> sendCones;
    for( Int iLoc=0; iLoc<localHeight; ++iLoc )
    {
        const Int i = x.GlobalRow(iLoc);
        const Int order = orders.GetLocal(iLoc,0);
        const Int firstInd = firstInds.GetLocal(iLoc,0);
        if( order > cutoff && i == firstInd )
        {
            sendCaps.push_back(x.GetLocal(iLoc,0));
            sendCones.push_back(i);
        }
    }
    int numSendCones = sendCones.size();
    std::vector<int> numRecvCones(commSize);
    mpi::AllToAll( &numSendCones, 1, numRecvCones.data(), 1, comm );
    totalRecv = Scan( numRecvCones, recvOffsets ); 
    std::vector<Real> recvCaps(totalRecv);
    std::vector<Int> recvCones(totalRecv);
    mpi::AllGather
    ( sendCaps.data(), numSendCones,
      recvCaps.data(), numRecvCones.data(), recvOffsets.data(), comm );
    mpi::AllGather
    ( sendCones.data(), numSendCones,
      recvCones.data(), numRecvCones.data(), recvOffsets.data(), comm );
    // TODO: Perform a sequence of mpi::AllReduce calls to simultaneously
    //       safely compute the bottom vector norms
    LogicError("This routine is not yet finished");

    return numNonSO;
}

#define PROTO(Real) \
  template Int NumNonSecondOrder \
  ( const Matrix<Real>& x, \
    const Matrix<Int>& orders, \
    const Matrix<Int>& firstInds ); \
  template Int NumNonSecondOrder \
  ( const AbstractDistMatrix<Real>& x, \
    const AbstractDistMatrix<Int>& orders, \
    const AbstractDistMatrix<Int>& firstInds, Int cutoff ); \
  template Int NumNonSecondOrder \
  ( const DistMultiVec<Real>& x, \
    const DistMultiVec<Int>& orders, \
    const DistMultiVec<Int>& firstInds, Int cutoff );

#define EL_NO_INT_PROTO
#define EL_NO_COMPLEX_PROTO
#include "El/macros/Instantiate.h"

} // namespace El
