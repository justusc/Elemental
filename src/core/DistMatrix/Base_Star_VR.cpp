/*
   Copyright (c) 2009-2011, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
#include "elemental/dist_matrix.hpp"
using namespace std;
using namespace elemental;
using namespace elemental::imports;
using namespace elemental::utilities;

// Template conventions:
//   G: general datatype
//
//   T: any ring, e.g., the (Gaussian) integers and the real/complex numbers
//   Z: representation of a real ring, e.g., the integers or real numbers
//   std::complex<Z>: representation of a complex ring, e.g. Gaussian integers
//                    or complex numbers
//
//   F: representation of real or complex number
//   R: representation of real number
//   std::complex<R>: representation of complex number

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::Print
( ostream& os, const string& msg ) const
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::Print");
#endif
    const elemental::Grid& g = this->Grid();
    if( g.VRRank() == 0 && msg != "" )
        os << msg << endl;

    const int height     = this->Height();
    const int width      = this->Width();
    const int localWidth = this->LocalWidth();
    const int p          = g.Size();
    const int rowShift   = this->RowShift();

    if( height == 0 || width == 0 )
    {
#ifndef RELEASE
        PopCallStack();
#endif
        return;
    }

    vector<T> sendBuf(height*width,0);
    const T* thisLocalBuffer = this->LockedLocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for COLLAPSE(2)
#endif
    for( int i=0; i<height; ++i )
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
            sendBuf[i+(rowShift+jLocal*p)*height] = 
                thisLocalBuffer[i+jLocal*thisLDim];

    // If we are the root, allocate a receive buffer
    vector<T> recvBuf;
    if( g.VRRank() == 0 )
        recvBuf.resize(height*width);

    // Sum the contributions and send to the root
    mpi::Reduce
    ( &sendBuf[0], &recvBuf[0], height*width, mpi::SUM, 0, g.VRComm() );

    if( g.VRRank() == 0 )
    {
        // Print the data
        for( int i=0; i<height; ++i )
        {
            for( int j=0; j<width; ++j )
                os << recvBuf[i+j*height] << " ";
            os << "\n";
        }
        os << endl;
    }
    mpi::Barrier( g.VRComm() );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::Print( const string& msg ) const
{
    Print( cout, msg );
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::Align
( int rowAlignment )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::Align");
    this->AssertFreeRowAlignment();
#endif
    this->AlignRows( rowAlignment );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRows
( int rowAlignment )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignRows");
    this->AssertFreeRowAlignment();
#endif
    const elemental::Grid& g = this->Grid();
#ifndef RELEASE
    if( rowAlignment < 0 || rowAlignment >= g.Size() )
        throw runtime_error( "Invalid row alignment for [* ,VR]" );
#endif
    this->_rowAlignment = rowAlignment;
    this->_rowShift = Shift( g.VRRank(), rowAlignment, g.Size() );
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([MC,MR])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_rowAlignment = A.RowAlignment();
    this->_rowShift = Shift( g.VRRank(), this->RowAlignment(), g.Size() );
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,MR,MC>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([MR,MC])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_rowAlignment = A.ColAlignment();
    this->_rowShift = Shift( g.VRRank(), this->RowAlignment(), g.Size() );
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,MR,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([MR,* ])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_rowAlignment = A.ColAlignment();
    this->_rowShift = Shift( g.VRRank(), this->RowAlignment(), g.Size() );
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,Star,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([* ,MR])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    const elemental::Grid& g = this->Grid();
    this->_rowAlignment = A.RowAlignment();
    this->_rowShift = Shift( g.VRRank(), this->RowAlignment(), g.Size() );
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,Star,VR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([* ,VR])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    this->_rowAlignment = A.RowAlignment();
    this->_rowShift = A.RowShift();
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignWith
( const DistMatrixBase<T,VR,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::AlignWith([VR,* ])");
    this->AssertFreeRowAlignment();
    this->AssertSameGrid( A );
#endif
    this->_rowAlignment = A.ColAlignment();
    this->_rowShift = A.ColShift();
    this->_constrainedRowAlignment = true;
    this->_height = 0;
    this->_width = 0;
    this->_localMatrix.ResizeTo( 0, 0 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,MC,MR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,MR,MC>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,MR,Star>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,Star,MR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,Star,VR>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::AlignRowsWith
( const DistMatrixBase<T,VR,Star>& A )
{ AlignWith( A ); }

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::View
( DistMatrixBase<T,Star,VR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::View");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_rowAlignment = A.RowAlignment();
    this->_rowShift = A.RowShift();
    this->_localMatrix.View( A.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::LockedView
( const DistMatrixBase<T,Star,VR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::LockedView(A)");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
#endif
    this->_height = A.Height();
    this->_width = A.Width();
    this->_rowAlignment = A.RowAlignment();
    this->_rowShift = A.RowShift();
    this->_localMatrix.LockedView( A.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::View
( DistMatrixBase<T,Star,VR>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::View");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    {
        const elemental::Grid& g = this->Grid();
        const int rowMajorRank = g.VRRank();
        const int size = g.Size();

        this->_rowAlignment = (A.RowAlignment()+j) % size;
        this->_rowShift = Shift( rowMajorRank, this->RowAlignment(), size );

        const int localWidthBefore = LocalLength( j, A.RowShift(), size );
        const int localWidth = LocalLength( width, this->RowShift(), size );

        this->_localMatrix.View
        ( A.LocalMatrix(), i, localWidthBefore, height, localWidth );
    }
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::LockedView
( const DistMatrixBase<T,Star,VR>& A,
  int i, int j, int height, int width )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::LockedView");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( A );
    this->AssertValidSubmatrix( A, i, j, height, width );
#endif
    this->_height = height;
    this->_width = width;
    {
        const elemental::Grid& g = this->Grid();
        const int rowMajorRank = g.VRRank();
        const int size = g.Size();

        this->_rowAlignment = (A.RowAlignment()+j) % size;
        this->_rowShift = Shift( rowMajorRank, this->RowAlignment(), size );

        const int localWidthBefore = LocalLength( j, A.RowShift(), size );
        const int localWidth = LocalLength( width, this->RowShift(), size );

        this->_localMatrix.LockedView
        ( A.LockedLocalMatrix(), i, localWidthBefore, height, localWidth );
    }
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::View1x2
( DistMatrixBase<T,Star,VR>& AL,
  DistMatrixBase<T,Star,VR>& AR )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::View1x2");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_rowAlignment = AL.RowAlignment();
    this->_rowShift = AL.RowShift();
    this->_localMatrix.View1x2( AL.LocalMatrix(), AR.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::LockedView1x2
( const DistMatrixBase<T,Star,VR>& AL,
  const DistMatrixBase<T,Star,VR>& AR )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::LockedView1x2");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AL );
    this->AssertSameGrid( AR );
    this->AssertConforming1x2( AL, AR );
#endif
    this->_height = AL.Height();
    this->_width = AL.Width() + AR.Width();
    this->_rowAlignment = AL.RowAlignment();
    this->_rowShift = AL.RowShift();
    this->_localMatrix.LockedView1x2
    ( AL.LockedLocalMatrix(), AR.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::View2x1
( DistMatrixBase<T,Star,VR>& AT,
  DistMatrixBase<T,Star,VR>& AB )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::View2x1");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_rowAlignment = AT.RowAlignment();
    this->_rowShift = AT.RowShift();
    this->_localMatrix.View2x1( AT.LocalMatrix(), AB.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::LockedView2x1
( const DistMatrixBase<T,Star,VR>& AT,
  const DistMatrixBase<T,Star,VR>& AB )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::LockedView2x1");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( AT );
    this->AssertSameGrid( AB );
    this->AssertConforming2x1( AT, AB );
#endif
    this->_height = AT.Height() + AB.Height();
    this->_width = AT.Width();
    this->_rowAlignment = AT.RowAlignment();
    this->_rowShift = AT.RowShift();
    this->_localMatrix.LockedView2x1
    ( AT.LockedLocalMatrix(), 
      AB.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::View2x2
( DistMatrixBase<T,Star,VR>& ATL,
  DistMatrixBase<T,Star,VR>& ATR,
  DistMatrixBase<T,Star,VR>& ABL,
  DistMatrixBase<T,Star,VR>& ABR )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::View2x2");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_rowAlignment = ATL.RowAlignment();
    this->_rowShift = ATL.RowShift();
    this->_localMatrix.View2x2
    ( ATL.LocalMatrix(), ATR.LocalMatrix(),
      ABL.LocalMatrix(), ABR.LocalMatrix() );
    this->_viewing = true;
    this->_lockedView = false;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::LockedView2x2
( const DistMatrixBase<T,Star,VR>& ATL,
  const DistMatrixBase<T,Star,VR>& ATR,
  const DistMatrixBase<T,Star,VR>& ABL,
  const DistMatrixBase<T,Star,VR>& ABR )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::LockedView2x2");
    this->AssertFreeRowAlignment();
    this->AssertNotStoringData();
    this->AssertSameGrid( ATL );
    this->AssertSameGrid( ATR );
    this->AssertSameGrid( ABL );
    this->AssertSameGrid( ABR );
    this->AssertConforming2x2( ATL, ATR, ABL, ABR );
#endif
    this->_height = ATL.Height() + ABL.Height();
    this->_width = ATL.Width() + ATR.Width();
    this->_rowAlignment = ATL.RowAlignment();
    this->_rowShift = ATL.RowShift();
    this->_localMatrix.LockedView2x2
    ( ATL.LockedLocalMatrix(), ATR.LockedLocalMatrix(),
      ABL.LockedLocalMatrix(), ABR.LockedLocalMatrix() );
    this->_viewing = true;
    this->_lockedView = true;
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::ResizeTo
( int height, int width )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::ResizeTo");
    this->AssertNotLockedView();
    if( height < 0 || width < 0 )
        throw logic_error( "Height and width must be non-negative." );
#endif
    const elemental::Grid& g = this->Grid();
    this->_height = height;
    this->_width = width;
    this->_localMatrix.ResizeTo
    ( height, LocalLength(width,this->RowShift(),g.Size()) );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
T
elemental::DistMatrixBase<T,Star,VR>::Get
( int i, int j ) const
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::Get");
    this->AssertValidEntry( i, j );
#endif
    // We will determine the owner rank of entry (i,j) and broadcast from that
    // process over the entire g
    const elemental::Grid& g = this->Grid();
    const int ownerRank = (j + this->RowAlignment()) % g.Size();

    T u;
    if( g.VRRank() == ownerRank )
    {
        const int jLoc = (j-this->RowShift()) / g.Size();
        u = this->GetLocalEntry(i,jLoc);
    }
    mpi::Broadcast( &u, 1, ownerRank, g.VRComm() );

#ifndef RELEASE
    PopCallStack();
#endif
    return u;
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::Set
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::Set");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerRank = (j + this->RowAlignment()) % g.Size();

    if( g.VRRank() == ownerRank )
    {
        const int jLoc = (j-this->RowShift()) / g.Size();
        this->SetLocalEntry(i,jLoc,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::Update
( int i, int j, T u )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::Update");
    this->AssertValidEntry( i, j );
#endif
    const elemental::Grid& g = this->Grid();
    const int ownerRank = (j + this->RowAlignment()) % g.Size();

    if( g.VRRank() == ownerRank )
    {
        const int jLoc = (j-this->RowShift()) / g.Size();
        this->UpdateLocalEntry(i,jLoc,u);
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// Utility functions, e.g., SetToIdentity and MakeTrapezoidal
//

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::MakeTrapezoidal
( Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::MakeTrapezoidal");
    this->AssertNotLockedView();
#endif
    const elemental::Grid& g = this->Grid();
    const int height = this->Height();
    const int width = this->Width();
    const int localWidth = this->LocalWidth();
    const int p = g.Size();
    const int rowShift = this->RowShift();

    if( shape == Lower )
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            int j = rowShift + jLocal*p;
            int lastZeroRow = ( side==Left ? j-offset-1
                                           : j-offset+height-width-1 );
            if( lastZeroRow >= 0 )
            {
                int boundary = min( lastZeroRow+1, height );
                T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
                memset( thisCol, 0, boundary*sizeof(T) );
            }
        }
    }
    else
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            int j = rowShift + jLocal*p;
            int firstZeroRow = ( side==Left ? max(j-offset+1,0)
                                            : max(j-offset+height-width+1,0) );
            if( firstZeroRow < height )
            {
                T* thisCol = &thisLocalBuffer[firstZeroRow+jLocal*thisLDim];
                memset( thisCol, 0, (height-firstZeroRow)*sizeof(T) );
            }
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::ScaleTrapezoidal
( T alpha, Side side, Shape shape, int offset )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::ScaleTrapezoidal");
    this->AssertNotLockedView();
#endif
    const elemental::Grid& g = this->Grid();
    const int height = this->Height();
    const int width = this->Width();
    const int localWidth = this->LocalWidth();
    const int p = g.Size();
    const int rowShift = this->RowShift();

    if( shape == Upper )
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            int j = rowShift + jLocal*p;
            int lastRow = ( side==Left ? j-offset : j-offset+height-width );
            int boundary = min( lastRow+1, height );
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            for( int i=0; i<boundary; ++i )
                thisCol[i] *= alpha;
        }
    }
    else
    {
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            int j = rowShift + jLocal*p;
            int firstRow = ( side==Left ? max(j-offset,0)
                                        : max(j-offset+height-width,0) );
            T* thisCol = &thisLocalBuffer[firstRow+jLocal*thisLDim];
            for( int i=0; i<(height-firstRow); ++i )
                thisCol[i] *= alpha;
        }
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::SetToIdentity()
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::SetToIdentity");
    this->AssertNotLockedView();
#endif
    const elemental::Grid& g = this->Grid();
    const int height = this->Height();
    const int localWidth = this->LocalWidth();
    const int p = g.Size();
    const int rowShift = this->RowShift();

    this->SetToZero();

    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const int j = rowShift + jLocal*p;
        if( j < height )
            thisLocalBuffer[j+jLocal*thisLDim] = 1;
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::SetToRandom()
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::SetToRandom");
    this->AssertNotLockedView();
#endif
    const int height = this->Height();
    const int localWidth = this->LocalWidth();
    for( int j=0; j<localWidth; ++j )
        for( int i=0; i<height; ++i )
            this->SetLocalEntry(i,j,SampleUnitBall<T>());
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::ConjugateTransposeFrom
( const DistMatrixBase<T,MR,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[*, VR]::ConjugateTransposeFrom");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.ColAlignment();
            this->_rowShift = 
                Shift( g.VRRank(), this->RowAlignment(), g.Size() );
        }
        this->ResizeTo( A.Width(), A.Height() );
    }

    if( this->RowAlignment() % g.Width() == A.ColAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int rowShift = this->RowShift();
        const int colShiftOfA = A.ColShift();
        const int rowOffset = (rowShift-colShiftOfA) / c;

        const int height = this->Height();
        const int localWidth = this->LocalWidth();

        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
            for( int i=0; i<height; ++i )
                thisLocalBuffer[i+jLocal*thisLDim] = 
                    Conj( ALocalBuffer[(rowOffset+jLocal*r)+i*ALDim] );
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [* ,VR]::ConjugateTransposeFrom" << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int row = g.MCRank();
        const int col = g.MRRank();
        const int colShiftOfA = A.ColShift();
        const int rowAlignment = this->RowAlignment();
        const int colAlignmentOfA = A.ColAlignment();

        // We will SendRecv A[*,VR] within our process row to fix alignments.
        const int sendCol = (col+c+(rowAlignment%c)-colAlignmentOfA) % c;
        const int recvCol = (col+c+colAlignmentOfA-(rowAlignment%c)) % c;
        const int sendRank = sendCol + c*row;

        const int sendRowShift = Shift( sendRank, rowAlignment, p );
        const int sendRowOffset = (sendRowShift-colShiftOfA) / c;

        const int height = this->Height();
        const int width = this->Width();
        const int localWidth = this->LocalWidth();
        const int localWidthOfSend = LocalLength(width,sendRowShift,p);

        const int sendSize = height * localWidthOfSend;
        const int recvSize = height * localWidth;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localWidthOfSend; ++jLocal )
            for( int i=0; i<height; ++i )
                sendBuffer[i+jLocal*height] = 
                    Conj( ALocalBuffer[(sendRowOffset+jLocal*r)+i*ALDim] );

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendCol, 0,
          recvBuffer, recvSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*height];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufferCol, height*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::TransposeFrom
( const DistMatrixBase<T,MR,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR]::TransposeFrom");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.ColAlignment();
            this->_rowShift = 
                Shift( g.VRRank(), this->RowAlignment(), g.Size() );
        }
        this->ResizeTo( A.Width(), A.Height() );
    }

    if( this->RowAlignment() % g.Width() == A.ColAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int rowShift = this->RowShift();
        const int colShiftOfA = A.ColShift();
        const int rowOffset = (rowShift-colShiftOfA) / c;

        const int height = this->Height();
        const int localWidth = this->LocalWidth();

        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
            for( int i=0; i<height; ++i )
                thisLocalBuffer[i+jLocal*thisLDim] = 
                    ALocalBuffer[(rowOffset+jLocal*r)+i*ALDim];
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [* ,VR]::TransposeFrom" << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int row = g.MCRank();
        const int col = g.MRRank();
        const int colShiftOfA = A.ColShift();
        const int rowAlignment = this->RowAlignment();
        const int colAlignmentOfA = A.ColAlignment();

        // We will SendRecv A[*,VR] within our process row to fix alignments.
        const int sendCol = (col+c+(rowAlignment%c)-colAlignmentOfA) % c;
        const int recvCol = (col+c+colAlignmentOfA-(rowAlignment%c)) % c;
        const int sendRank = sendCol + c*row;

        const int sendRowShift = Shift( sendRank, rowAlignment, p );
        const int sendRowOffset = (sendRowShift-colShiftOfA) / c;

        const int height = this->Height();
        const int width = this->Width();
        const int localWidth = this->LocalWidth();
        const int localWidthOfSend = LocalLength(width,sendRowShift,p);

        const int sendSize = height * localWidthOfSend;
        const int recvSize = height * localWidth;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for COLLAPSE(2)
#endif
        for( int jLocal=0; jLocal<localWidthOfSend; ++jLocal )
            for( int i=0; i<height; ++i )
                sendBuffer[i+jLocal*height] = 
                    ALocalBuffer[(sendRowOffset+jLocal*r)+i*ALDim];

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendCol, 0,
          recvBuffer, recvSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*height];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufferCol, height*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,MC,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [MC,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.RowAlignment();
            this->_rowShift = 
                Shift( g.VRRank(), this->RowAlignment(), g.Size() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->RowAlignment() % g.Width() == A.RowAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int col = g.MRRank();
        const int rowShiftOfA = A.RowShift();
        const int rowAlignment = this->RowAlignment();
        const int colAlignmentOfA = A.ColAlignment();

        const int height = this->Height();
        const int width = this->Width();
        const int localWidth = this->LocalWidth();
        const int localHeightOfA = A.LocalHeight();

        const int maxHeight = MaxLocalLength(height,r);
        const int maxWidth = MaxLocalLength(width,p);
        const int portionSize = max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( 2*r*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[r*portionSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            T* data = &sendBuffer[k*portionSize];

            const int thisRank = col+k*c;
            const int thisRowShift = RawShift(thisRank,rowAlignment,p);
            const int thisRowOffset = (thisRowShift-rowShiftOfA) / c;
            const int thisLocalWidth = RawLocalLength(width,thisRowShift,p);

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ALocalBuffer[(thisRowOffset+jLocal*r)*ALDim];
                T* dataCol = &data[jLocal*localHeightOfA];
                memcpy( dataCol, ACol, localHeightOfA*sizeof(T) );
            }
        }

        // Communicate
        mpi::AllToAll
        ( sendBuffer, portionSize,
          recvBuffer, portionSize, g.MCComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &recvBuffer[k*portionSize];

            const int thisColShift = RawShift(k,colAlignmentOfA,r);
            const int thisLocalHeight = RawLocalLength(height,thisColShift,r);

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
                for( int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    thisLocalBuffer[(thisColShift+iLocal*r)+jLocal*thisLDim] =
                        data[iLocal+jLocal*thisLocalHeight];
        }
        this->_auxMemory.Release();
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [* ,VR] <- [MC,MR]." << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int col = g.MRRank();
        const int rowShiftOfA = A.RowShift();
        const int rowAlignment = this->RowAlignment();
        const int colAlignmentOfA = A.ColAlignment();
        const int rowAlignmentOfA = A.RowAlignment();

        const int sendCol = (col+c+(rowAlignment%c)-rowAlignmentOfA) % c;
        const int recvCol = (col+c+rowAlignmentOfA-(rowAlignment%c)) % c;

        const int height = this->Height();
        const int width = this->Width();
        const int localWidth = this->LocalWidth();
        const int localHeightOfA = A.LocalHeight();

        const int maxHeight = MaxLocalLength(height,r);
        const int maxWidth = MaxLocalLength(width,p);
        const int portionSize = max(maxHeight*maxWidth,mpi::MIN_COLL_MSG);

        this->_auxMemory.Require( 2*r*portionSize );

        T* buffer = this->_auxMemory.Buffer();
        T* firstBuffer = &buffer[0];
        T* secondBuffer = &buffer[r*portionSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            T* data = &secondBuffer[k*portionSize];

            const int thisRank = sendCol+k*c;
            const int thisRowShift = RawShift(thisRank,rowAlignment,p);
            const int thisRowOffset = (thisRowShift-rowShiftOfA) / c;
            const int thisLocalWidth = RawLocalLength(width,thisRowShift,p);

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ALocalBuffer[(thisRowOffset+jLocal*r)*ALDim];
                T* dataCol = &data[jLocal*localHeightOfA];
                memcpy( dataCol, ACol, localHeightOfA*sizeof(T) );
            }
        }

        // AllToAll to gather all of the unaligned [*,VR] data into firstBuffer
        mpi::AllToAll
        ( secondBuffer, portionSize,
          firstBuffer,  portionSize, g.MCComm() );

        // SendRecv: properly align the [*,VR] via a trade in the column
        mpi::SendRecv
        ( firstBuffer,  portionSize, sendCol, 0,
          secondBuffer, portionSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            const T* data = &secondBuffer[k*portionSize];

            const int thisColShift = RawShift(k,colAlignmentOfA,r);
            const int thisLocalHeight = RawLocalLength(height,thisColShift,r);

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for COLLAPSE(2)
#endif
            for( int jLocal=0; jLocal<localWidth; ++jLocal )
                for( int iLocal=0; iLocal<thisLocalHeight; ++iLocal )
                    thisLocalBuffer[(thisColShift+iLocal*r)+jLocal*thisLDim] =
                        data[iLocal+jLocal*thisLocalHeight];
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,MC,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [MC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MC,MR> A_MC_MR(g);

    A_MC_MR = A;
    *this = A_MC_MR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,MR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,MR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.RowAlignment();
            this->_rowShift = 
                Shift( g.VRRank(), this->RowAlignment(), g.Size() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->RowAlignment() % g.Width() == A.RowAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int rowShift = this->RowShift();
        const int rowShiftOfA = A.RowShift();
        const int rowOffset = (rowShift-rowShiftOfA) / c;

        const int height = this->Height();
        const int localWidth = this->LocalWidth();

        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* ACol = &ALocalBuffer[(rowOffset+jLocal*r)*ALDim];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, ACol, height*sizeof(T) );
        }
    }
    else
    {
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [* ,VR] <- [* ,MR]." << endl;
#endif
        const int r = g.Height();
        const int c = g.Width();
        const int p = g.Size();
        const int row = g.MCRank();
        const int col = g.MRRank();
        const int rowShiftOfA = A.RowShift();
        const int rowAlignment = this->RowAlignment();
        const int rowAlignmentOfA = A.RowAlignment();

        // We will SendRecv A[*,VR] within our process row to fix alignments.
        const int sendCol = (col+c+(rowAlignment%c)-rowAlignmentOfA) % c;
        const int recvCol = (col+c+rowAlignmentOfA-(rowAlignment%c)) % c;
        const int sendRank = sendCol + c*row;

        const int sendRowShift = Shift( sendRank, rowAlignment, p );
        const int sendRowOffset = (sendRowShift-rowShiftOfA) / c;

        const int height = this->Height();
        const int width = this->Width();
        const int localWidth = this->LocalWidth();
        const int localWidthOfSend = LocalLength(width,sendRowShift,p);

        const int sendSize = height * localWidthOfSend;
        const int recvSize = height * localWidth;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidthOfSend; ++jLocal )
        {
            const T* ACol = &ALocalBuffer[(sendRowOffset+jLocal*r)*ALDim];
            T* sendBufferCol = &sendBuffer[jLocal*height];
            memcpy( sendBufferCol, ACol, height*sizeof(T) );
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendCol, 0,
          recvBuffer, recvSize, recvCol, mpi::ANY_TAG, g.MRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*height];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufferCol, height*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,MD,Star>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR] = [MD,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[* ,VR] = [MD,* ] is not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,MD>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,MD]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    throw logic_error( "[* ,VR] = [* ,MD] is not yet implemented." );
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,MR,MC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [MR,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,Star,VC> A_Star_VC(g);

    A_Star_VC = A;
    *this = A_Star_VC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,MR,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [MR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,MR,MC> > A_MR_MC
    ( new DistMatrix<T,MR,MC>(g) );
    *A_MR_MC = A;

    auto_ptr< DistMatrix<T,Star,VC> > A_Star_VC
    ( new DistMatrix<T,Star,VC>(g) );
    *A_Star_VC = *A_MR_MC;
    delete A_MR_MC.release(); // lowers memory highwater

    *this = *A_Star_VC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,MC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,MC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,Star,VC> A_Star_VC(g);

    A_Star_VC = A;
    *this = A_Star_VC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,VC,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [VC,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    DistMatrix<T,MC,MR> A_MC_MR(g);

    A_MC_MR = A;
    *this = A_MC_MR;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,VC>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,VC]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );
    
    const int height = this->Height();
    const int localWidth = this->LocalWidth();
    const int localWidthOfA = A.LocalWidth();

    const int sendSize = height * localWidthOfA;
    const int recvSize = height * localWidth;

    const int r = g.Height();
    const int c = g.Width();
    const int p = g.Size();
    const int rankCM = g.VCRank();
    const int rankRM = g.VRRank();

    const int rowShift = this->RowShift();
    const int rowShiftOfA = A.RowShift();

    // Compute which rowmajor rank has the rowShift equal to our rowShiftOfA
    const int sendRankRM = (rankRM+(p+rowShiftOfA-rowShift)) % p;

    // Compute which rowmajor rank has the A rowShift that we need
    const int recvRankCM = (rankCM+(p+rowShift-rowShiftOfA)) % p;
    const int recvRankRM = (recvRankCM/r)+c*(recvRankCM%r);

    this->_auxMemory.Require( sendSize + recvSize );

    T* buffer = this->_auxMemory.Buffer();
    T* sendBuffer = &buffer[0];
    T* recvBuffer = &buffer[sendSize];

    // Pack
    const T* ALocalBuffer = A.LockedLocalBuffer();
    const int ALDim = A.LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
    {
        const T* ACol = &ALocalBuffer[jLocal*ALDim];
        T* sendBufferCol = &sendBuffer[jLocal*height];
        memcpy( sendBufferCol, ACol, height*sizeof(T) );
    }

    // Communicate
    mpi::SendRecv
    ( sendBuffer, sendSize, sendRankRM, 0,
      recvBuffer, recvSize, recvRankRM, mpi::ANY_TAG, g.VRComm() );

    // Unpack
    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* recvBufferCol = &recvBuffer[jLocal*height];
        T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
        memcpy( thisCol, recvBufferCol, height*sizeof(T) );
    }
    this->_auxMemory.Release();
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,VR,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [VR,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    auto_ptr< DistMatrix<T,MR,MC> > A_MR_MC
    ( new DistMatrix<T,MR,MC>(g) );
    *A_MR_MC = A;

    auto_ptr< DistMatrix<T,Star,VC> > A_Star_VC
    ( new DistMatrix<T,Star,VC>(g) );
    *A_Star_VC = *A_MR_MC;
    delete A_MR_MC.release(); // lowers memory highwater

    *this = *A_Star_VC;
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,VR>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,VR]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.RowAlignment();
            this->_rowShift = A.RowShift();
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->RowAlignment() == A.RowAlignment() )
    {
        this->_localMatrix = A.LockedLocalMatrix();
    }
    else
    {
        const elemental::Grid& g = this->Grid();
#ifdef UNALIGNED_WARNINGS
        if( g.VCRank() == 0 )
            cerr << "Unaligned [* ,VR] <- [* ,VR]." << endl;
#endif
        const int rank = g.VRRank();
        const int p = g.Size();

        const int rowAlignment = this->RowAlignment();
        const int rowAlignmentOfA = A.RowAlignment();

        const int sendRank = (rank+p+rowAlignment-rowAlignmentOfA) % p;
        const int recvRank = (rank+p+rowAlignmentOfA-rowAlignment) % p;

        const int height = this->Height();
        const int localWidth = this->LocalWidth();
        const int localWidthOfA = A.LocalWidth();

        const int sendSize = height * localWidthOfA;
        const int recvSize = height * localWidth;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidthOfA; ++jLocal )
        {
            const T* ACol = &ALocalBuffer[jLocal*ALDim];
            T* sendBufferCol = &sendBuffer[jLocal*height];
            memcpy( sendBufferCol, ACol, height*sizeof(T) );
        }

        // Communicate
        mpi::SendRecv
        ( sendBuffer, sendSize, sendRank, 0,
          recvBuffer, recvSize, recvRank, mpi::ANY_TAG, g.VRComm() );

        // Unpack
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*height];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufferCol, height*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
const DistMatrixBase<T,Star,VR>&
elemental::DistMatrixBase<T,Star,VR>::operator=
( const DistMatrixBase<T,Star,Star>& A )
{ 
#ifndef RELEASE
    PushCallStack("[* ,VR] = [* ,* ]");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    if( !this->Viewing() )
        this->ResizeTo( A.Height(), A.Width() );

    const int p = this->Grid().Size();
    const int rowShift = this->RowShift();

    const int localHeight = this->LocalHeight();
    const int localWidth = this->LocalWidth();

    T* thisLocalBuffer = this->LocalBuffer();
    const int thisLDim = this->LocalLDim();
    const T* ALocalBuffer = A.LockedLocalBuffer();
    const int ALDim = A.LocalLDim();
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for( int jLocal=0; jLocal<localWidth; ++jLocal )
    {
        const T* ACol = &ALocalBuffer[(rowShift+jLocal*p)*ALDim];
        T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
        memcpy( thisCol, ACol, localHeight*sizeof(T) );
    }
#ifndef RELEASE
    PopCallStack();
#endif
    return *this;
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::SumScatterFrom
( const DistMatrixBase<T,Star,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::SumScatterFrom( [* ,MR] )");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    if( this->Viewing() )
        this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( !this->Viewing() )
    {
        if( !this->ConstrainedRowAlignment() )
        {
            this->_rowAlignment = A.RowAlignment();
            this->_rowShift = 
                Shift( g.VRRank(), this->RowAlignment(), g.Size() );
        }
        this->ResizeTo( A.Height(), A.Width() );
    }

    if( this->RowAlignment() % g.Width() == A.RowAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int p = r * c;
        const int col = g.MRRank();
        const int rowAlignment = this->RowAlignment();
        const int rowShiftOfA = A.RowShift();

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localWidth = this->LocalWidth();
        const int maxLocalWidth = MaxLocalLength( width, p );

        const int recvSize = max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
        const int sendSize = r*recvSize;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        vector<int> recvSizes(r);
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            T* data = &sendBuffer[k*recvSize];
            recvSizes[k] = recvSize;

            const int thisRank = col+k*c;
            const int thisRowShift = RawShift( thisRank, rowAlignment, p );
            const int thisRowOffset = (thisRowShift-rowShiftOfA) / c;
            const int thisLocalWidth = RawLocalLength( width, thisRowShift, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ALocalBuffer[(thisRowOffset+jLocal*r)*ALDim];
                T* dataCol = &data[jLocal*localHeight];
                memcpy( dataCol, ACol, localHeight*sizeof(T) );
            }
        }

        // Reduce-scatter over each process column
        mpi::ReduceScatter
        ( sendBuffer, recvBuffer, &recvSizes[0], mpi::SUM, g.MCComm() );

        // Unpack our received data
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            memcpy( thisCol, recvBufferCol, localHeight*sizeof(T) );
        }
        this->_auxMemory.Release();
    }
    else
    {
        throw logic_error
              ( "Unaligned [* ,VR]::ReduceScatterFrom( [* ,MR] ) is not "
                "yet implemented." );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
void
elemental::DistMatrixBase<T,Star,VR>::SumScatterUpdate
( T alpha, const DistMatrixBase<T,Star,MR>& A )
{
#ifndef RELEASE
    PushCallStack("[* ,VR]::SumScatterUpdate( [* ,MR] )");
    this->AssertNotLockedView();
    this->AssertSameGrid( A );
    this->AssertSameSize( A );
#endif
    const elemental::Grid& g = this->Grid();
    if( this->RowAlignment() % g.Width() == A.RowAlignment() )
    {
        const int r = g.Height();
        const int c = g.Width();
        const int p = r * c;
        const int col = g.MRRank();
        const int rowAlignment = this->RowAlignment();
        const int rowShiftOfA = A.RowShift();

        const int width = this->Width();
        const int localHeight = this->LocalHeight();
        const int localWidth = this->LocalWidth();
        const int maxLocalWidth = MaxLocalLength( width, p );

        const int recvSize = max(localHeight*maxLocalWidth,mpi::MIN_COLL_MSG);
        const int sendSize = r*recvSize;

        this->_auxMemory.Require( sendSize + recvSize );

        T* buffer = this->_auxMemory.Buffer();
        T* sendBuffer = &buffer[0];
        T* recvBuffer = &buffer[sendSize];

        // Pack
        vector<int> recvSizes(r);
        const T* ALocalBuffer = A.LockedLocalBuffer();
        const int ALDim = A.LocalLDim();
#if defined(_OPENMP) && !defined(PARALLELIZE_INNER_LOOPS)
        #pragma omp parallel for
#endif
        for( int k=0; k<r; ++k )
        {
            T* data = &sendBuffer[k*recvSize];
            recvSizes[k] = recvSize;

            const int thisRank = col+k*c;
            const int thisRowShift = RawShift( thisRank, rowAlignment, p );
            const int thisRowOffset = (thisRowShift-rowShiftOfA) / c;
            const int thisLocalWidth = RawLocalLength( width, thisRowShift, p );

#if defined(_OPENMP) && defined(PARALLELIZE_INNER_LOOPS)
            #pragma omp parallel for
#endif
            for( int jLocal=0; jLocal<thisLocalWidth; ++jLocal )
            {
                const T* ACol = &ALocalBuffer[(thisRowOffset+jLocal*r)*ALDim];
                T* dataCol = &data[jLocal*localHeight];
                memcpy( dataCol, ACol, localHeight*sizeof(T) );
            }
        }

        // Reduce-scatter over each process column
        mpi::ReduceScatter
        ( sendBuffer, recvBuffer, &recvSizes[0], mpi::SUM, g.MCComm() );

        // Unpack our received data
        T* thisLocalBuffer = this->LocalBuffer();
        const int thisLDim = this->LocalLDim();
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for( int jLocal=0; jLocal<localWidth; ++jLocal )
        {
            const T* recvBufferCol = &recvBuffer[jLocal*localHeight];
            T* thisCol = &thisLocalBuffer[jLocal*thisLDim];
            for( int iLocal=0; iLocal<localHeight; ++iLocal )
                thisCol[iLocal] += alpha*recvBufferCol[iLocal];
        }
        this->_auxMemory.Release();
    }
    else
    {
        throw logic_error
              ( "Unaligned [* ,VR]::ReduceScatterUpdate( [* ,MR] ) is not "
                "yet implemented." );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template class elemental::DistMatrixBase<int,   Star,VR>;
template class elemental::DistMatrixBase<float, Star,VR>;
template class elemental::DistMatrixBase<double,Star,VR>;
#ifndef WITHOUT_COMPLEX
template class elemental::DistMatrixBase<scomplex,Star,VR>;
template class elemental::DistMatrixBase<dcomplex,Star,VR>;
#endif

