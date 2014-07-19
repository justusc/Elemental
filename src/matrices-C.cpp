/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"
#include "El-C.h"
using namespace El;

extern "C" {

#define C_PROTO_BASE(SIG,SIGBASE,T) \
  /* Circulant */ \
  ElError ElCirculant_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Circulant( *Reinterpret(A), a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElCirculantDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Circulant( *Reinterpret(A), a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Diagonal */ \
  ElError ElDiagonal_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt dSize, CREFLECT(T)* dBuf ) \
  { try { \
      std::vector<T> d( Reinterpret(dBuf), Reinterpret(dBuf)+dSize ); \
      Diagonal( *Reinterpret(A), d ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElDiagonalDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt dSize, CREFLECT(T)* dBuf ) \
  { try { \
      std::vector<T> d( Reinterpret(dBuf), Reinterpret(dBuf)+dSize ); \
      Diagonal( *Reinterpret(A), d ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Forsythe */ \
  ElError ElForsythe_ ## SIG \
  ( ElMatrix_ ## SIG J, ElInt n, CREFLECT(T) alpha, CREFLECT(T) lambda ) \
  { EL_TRY( \
      Forsythe( \
        *Reinterpret(J), n, Reinterpret(alpha), Reinterpret(lambda) ) ) } \
  ElError ElForsytheDist_ ## SIG \
  ( ElDistMatrix_ ## SIG J, ElInt n, CREFLECT(T) alpha, CREFLECT(T) lambda ) \
  { EL_TRY( \
      Forsythe( \
        *Reinterpret(J), n, Reinterpret(alpha), Reinterpret(lambda) ) ) } \
  /* GCD matrix */ \
  ElError ElGCDMatrix_ ## SIG ( ElMatrix_ ## SIG G, ElInt m, ElInt n ) \
  { EL_TRY( GCDMatrix( *Reinterpret(G), m, n ) ) } \
  ElError ElGCDMatrixDist_ ## SIG ( ElDistMatrix_ ## SIG G, ElInt m, ElInt n ) \
  { EL_TRY( GCDMatrix( *Reinterpret(G), m, n ) ) } \
  /* Gear */ \
  ElError ElGear_ ## SIG \
  ( ElMatrix_ ## SIG G, ElInt n, ElInt s, ElInt t ) \
  { EL_TRY( Gear( *Reinterpret(G), n, s, t ) ) } \
  ElError ElGearDist_ ## SIG \
  ( ElDistMatrix_ ## SIG G, ElInt n, ElInt s, ElInt t ) \
  { EL_TRY( Gear( *Reinterpret(G), n, s, t ) ) } \
  /* Grcar */ \
  ElError ElGrcar_ ## SIG ( ElMatrix_ ## SIG A, ElInt n, ElInt k ) \
  { EL_TRY( Grcar( *Reinterpret(A), n, k ) ) } \
  ElError ElGrcarDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n, ElInt k ) \
  { EL_TRY( Grcar( *Reinterpret(A), n, k ) ) } \
  /* Hankel */ \
  ElError ElHankel_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt m, ElInt n, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Hankel( *Reinterpret(A), m, n, a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElHankelDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Hankel( *Reinterpret(A), m, n, a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Hanowa */ \
  ElError ElHanowa_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) mu ) \
  { EL_TRY( Hanowa( *Reinterpret(A), n, Reinterpret(mu) ) ) } \
  /* TODO: Distributed Hanowa */ \
  /* Identity */ \
  ElError ElIdentity_ ## SIG ( ElMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Identity( *Reinterpret(A), m, n ) ) } \
  ElError ElIdentityDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Identity( *Reinterpret(A), m, n ) ) } \
  /* Jordan */ \
  ElError ElJordan_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) lambda ) \
  { EL_TRY( Jordan( *Reinterpret(A), n, Reinterpret(lambda) ) ) } \
  ElError ElJordanDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, CREFLECT(T) lambda ) \
  { EL_TRY( Jordan( *Reinterpret(A), n, Reinterpret(lambda) ) ) } \
  /* KMS */ \
  ElError ElKMS_ ## SIG \
  ( ElMatrix_ ## SIG K, ElInt n, CREFLECT(T) rho ) \
  { EL_TRY( KMS( *Reinterpret(K), n, Reinterpret(rho) ) ) } \
  ElError ElKMSDist_ ## SIG \
  ( ElDistMatrix_ ## SIG K, ElInt n, CREFLECT(T) rho ) \
  { EL_TRY( KMS( *Reinterpret(K), n, Reinterpret(rho) ) ) } \
  /* Lauchli */ \
  ElError ElLauchli_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) mu ) \
  { EL_TRY( Lauchli( *Reinterpret(A), n, Reinterpret(mu) ) ) } \
  ElError ElLauchliDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, CREFLECT(T) mu ) \
  { EL_TRY( Lauchli( *Reinterpret(A), n, Reinterpret(mu) ) ) } \
  /* MinIJ */ \
  ElError ElMinIJ_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( MinIJ( *Reinterpret(A), n ) ) } \
  ElError ElMinIJDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( MinIJ( *Reinterpret(A), n ) ) } \
  /* Ones */ \
  ElError ElOnes_ ## SIG ( ElMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Ones( *Reinterpret(A), m, n ) ) } \
  ElError ElOnesDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Ones( *Reinterpret(A), m, n ) ) } \
  /* 1-2-1 */ \
  ElError ElOneTwoOne_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( OneTwoOne( *Reinterpret(A), n ) ) } \
  ElError ElOneTwoOneDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( OneTwoOne( *Reinterpret(A), n ) ) } \
  /* Redheffer */ \
  ElError ElRedheffer_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Redheffer( *Reinterpret(A), n ) ) } \
  ElError ElRedhefferDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Redheffer( *Reinterpret(A), n ) ) } \
  /* Riemann */ \
  ElError ElRiemann_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Riemann( *Reinterpret(A), n ) ) } \
  ElError ElRiemannDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Riemann( *Reinterpret(A), n ) ) } \
  /* TriW */ \
  ElError ElTriW_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt m, ElInt n, CREFLECT(T) alpha, ElInt k ) \
  { EL_TRY( TriW( *Reinterpret(A), m, n, Reinterpret(alpha), k ) ) } \
  ElError ElTriWDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n, CREFLECT(T) alpha, ElInt k ) \
  { EL_TRY( TriW( *Reinterpret(A), m, n, Reinterpret(alpha), k ) ) } \
  /* Uniform */ \
  ElError ElUniform_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt m, ElInt n, \
    CREFLECT(T) center, Base<T> radius ) \
  { EL_TRY( Uniform( *Reinterpret(A), m, n, Reinterpret(center), radius ) ) } \
  ElError ElUniformDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n, \
    CREFLECT(T) center, Base<T> radius ) \
  { EL_TRY( Uniform( *Reinterpret(A), m, n, Reinterpret(center), radius ) ) } \
  /* Walsh */ \
  ElError ElWalsh_ ## SIG ( ElMatrix_ ## SIG A, ElInt k, bool binary ) \
  { EL_TRY( Walsh( *Reinterpret(A), k, binary ) ) } \
  ElError ElWalshDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt k, bool binary ) \
  { EL_TRY( Walsh( *Reinterpret(A), k, binary ) ) } \
  /* Wilkinson */ \
  ElError ElWilkinson_ ## SIG ( ElMatrix_ ## SIG A, ElInt k ) \
  { EL_TRY( Wilkinson( *Reinterpret(A), k ) ) } \
  ElError ElWilkinsonDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt k ) \
  { EL_TRY( Wilkinson( *Reinterpret(A), k ) ) } \
  /* Zeros */ \
  ElError ElZeros_ ## SIG ( ElMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Zeros( *Reinterpret(A), m, n ) ) } \
  ElError ElZerosDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n ) \
  { EL_TRY( Zeros( *Reinterpret(A), m, n ) ) }

#define C_PROTO_NOINT(SIG,SIGBASE,T) \
  /* Cauchy */ \
  ElError ElCauchy_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt xSize, CREFLECT(T)* xBuf, \
                        ElInt ySize, CREFLECT(T)* yBuf ) \
  { try { \
      std::vector<T> x( Reinterpret(xBuf), Reinterpret(xBuf)+xSize ); \
      std::vector<T> y( Reinterpret(yBuf), Reinterpret(yBuf)+ySize ); \
      Cauchy( *Reinterpret(A), x, y ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElCauchyDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt xSize, CREFLECT(T)* xBuf, \
                            ElInt ySize, CREFLECT(T)* yBuf ) \
  { try { \
      std::vector<T> x( Reinterpret(xBuf), Reinterpret(xBuf)+xSize ); \
      std::vector<T> y( Reinterpret(yBuf), Reinterpret(yBuf)+ySize ); \
      Cauchy( *Reinterpret(A), x, y ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Cauchy-like */ \
  ElError ElCauchyLike_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt rSize, CREFLECT(T)* rBuf, \
                        ElInt sSize, CREFLECT(T)* sBuf, \
                        ElInt xSize, CREFLECT(T)* xBuf, \
                        ElInt ySize, CREFLECT(T)* yBuf ) \
  { try { \
      std::vector<T> r( Reinterpret(rBuf), Reinterpret(rBuf)+rSize ); \
      std::vector<T> s( Reinterpret(sBuf), Reinterpret(sBuf)+sSize ); \
      std::vector<T> x( Reinterpret(xBuf), Reinterpret(xBuf)+xSize ); \
      std::vector<T> y( Reinterpret(yBuf), Reinterpret(yBuf)+ySize ); \
      CauchyLike( *Reinterpret(A), r, s, x, y ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElCauchyLikeDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt rSize, CREFLECT(T)* rBuf, \
                            ElInt sSize, CREFLECT(T)* sBuf, \
                            ElInt xSize, CREFLECT(T)* xBuf, \
                            ElInt ySize, CREFLECT(T)* yBuf ) \
  { try { \
      std::vector<T> r( Reinterpret(rBuf), Reinterpret(rBuf)+rSize ); \
      std::vector<T> s( Reinterpret(sBuf), Reinterpret(sBuf)+sSize ); \
      std::vector<T> x( Reinterpret(xBuf), Reinterpret(xBuf)+xSize ); \
      std::vector<T> y( Reinterpret(yBuf), Reinterpret(yBuf)+ySize ); \
      CauchyLike( *Reinterpret(A), r, s, x, y ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Demmel */ \
  ElError ElDemmel_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Demmel( *Reinterpret(A), n ) ) } \
  ElError ElDemmelDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Demmel( *Reinterpret(A), n ) ) } \
  /* Ehrenfest */ \
  ElError ElEhrenfest_ ## SIG ( ElMatrix_ ## SIG P, ElInt n ) \
  { EL_TRY( Ehrenfest( *Reinterpret(P), n ) ) } \
  ElError ElEhrenfestDist_ ## SIG ( ElDistMatrix_ ## SIG P, ElInt n ) \
  { EL_TRY( Ehrenfest( *Reinterpret(P), n ) ) } \
  ElError ElEhrenfestStationary_ ## SIG \
  ( ElMatrix_ ## SIG PInf, ElInt n ) \
  { EL_TRY( EhrenfestStationary( *Reinterpret(PInf), n ) ) } \
  ElError ElEhrenfestStationaryDist_ ## SIG \
  ( ElDistMatrix_ ## SIG PInf, ElInt n ) \
  { EL_TRY( EhrenfestStationary( *Reinterpret(PInf), n ) ) } \
  ElError ElEhrenfestDecay_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( EhrenfestDecay( *Reinterpret(A), n ) ) } \
  ElError ElEhrenfestDecayDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( EhrenfestDecay( *Reinterpret(A), n ) ) } \
  /* ExtendedKahan */ \
  ElError ElExtendedKahan_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt k, Base<T> phi, Base<T> mu ) \
  { EL_TRY( ExtendedKahan( *Reinterpret(A), k, phi, mu ) ) } \
  ElError ElExtendedKahanDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt k, Base<T> phi, Base<T> mu ) \
  { EL_TRY( ExtendedKahan( *Reinterpret(A), k, phi, mu ) ) } \
  /* Fiedler */ \
  ElError ElFiedler_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt cSize, CREFLECT(T)* cBuf ) \
  { try { \
      std::vector<T> c( Reinterpret(cBuf), Reinterpret(cBuf)+cSize ); \
      Fiedler( *Reinterpret(A), c ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElFiedlerDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt cSize, CREFLECT(T)* cBuf ) \
  { try { \
      std::vector<T> c( Reinterpret(cBuf), Reinterpret(cBuf)+cSize ); \
      Fiedler( *Reinterpret(A), c ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Gaussian */ \
  ElError ElGaussian_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt m, ElInt n, \
    CREFLECT(T) mean, Base<T> stddev ) \
  { EL_TRY( Gaussian( *Reinterpret(A), m, n, Reinterpret(mean), stddev ) ) } \
  ElError ElGaussianDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n, \
    CREFLECT(T) mean, Base<T> stddev ) \
  { EL_TRY( Gaussian( *Reinterpret(A), m, n, Reinterpret(mean), stddev ) ) } \
  /* Golub Klema Stewart */ \
  ElError ElGKS_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( GKS( *Reinterpret(A), n ) ) } \
  ElError ElGKSDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( GKS( *Reinterpret(A), n ) ) } \
  /* Haar */ \
  ElError ElHaar_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Haar( *Reinterpret(A), n ) ) } \
  /* TODO: Distributed Haar */ \
  ElError ElImplicitHaar_ ## SIG \
  ( ElMatrix_ ## SIG A, \
    ElMatrix_ ## SIG t, ElMatrix_ ## SIGBASE d, ElInt n ) \
  { EL_TRY( \
      ImplicitHaar( *Reinterpret(A), *Reinterpret(t), *Reinterpret(d), n ) ) } \
  /* TODO: Distributed implicit Haar */ \
  /* Hatano-Nelson */ \
  ElError ElHatanoNelson_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) center, Base<T> radius, \
    CREFLECT(T) g, bool periodic ) \
  { EL_TRY( \
      HatanoNelson( \
        *Reinterpret(A), n, Reinterpret(center), radius, Reinterpret(g), \
        periodic ) ) } \
  /* TODO: Distributed Hatano-Nelson */ \
  /* Helmholtz */ \
  ElError ElHelmholtz1D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, Reinterpret(shift) ) ) } \
  ElError ElHelmholtz1DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, Reinterpret(shift) ) ) } \
  ElError ElHelmholtz2D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, ElInt ny, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, ny, Reinterpret(shift) ) ) } \
  ElError ElHelmholtz2DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, ElInt ny, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, ny, Reinterpret(shift) ) ) } \
  ElError ElHelmholtz3D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, ElInt ny, ElInt nz, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, ny, nz, Reinterpret(shift) ) ) } \
  ElError ElHelmholtz3DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, ElInt ny, ElInt nz, CREFLECT(T) shift ) \
  { EL_TRY( Helmholtz( *Reinterpret(H), nx, ny, nz, Reinterpret(shift) ) ) } \
  /* Hermitian from EVD */ \
  ElError ElHermitianFromEVD_ ## SIG \
  ( ElUpperOrLower uplo, ElMatrix_ ## SIG A, \
    ElConstMatrix_ ## SIGBASE w, ElConstMatrix_ ## SIG Z ) \
  { EL_TRY( \
      HermitianFromEVD( \
        Reinterpret(uplo), *Reinterpret(A), \
        *Reinterpret(w), *Reinterpret(Z) ) ) } \
  /* TODO: Distributed HermitianFromEVD */ \
  /* Hermitian uniform spectrum */ \
  ElError ElHermitianUniformSpectrum_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, Base<T> lower, Base<T> upper ) \
  { EL_TRY( HermitianUniformSpectrum( *Reinterpret(A), n, lower, upper ) ) } \
  /* TODO: Distributed HermitianUniformSpectrum */ \
  /* Hilbert */ \
  ElError ElHilbert_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Hilbert( *Reinterpret(A), n ) ) } \
  ElError ElHilbertDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Hilbert( *Reinterpret(A), n ) ) } \
  /* Kahan */ \
  ElError ElKahan_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) phi ) \
  { EL_TRY( Kahan( *Reinterpret(A), n, Reinterpret(phi) ) ) } \
  ElError ElKahanDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, CREFLECT(T) phi ) \
  { EL_TRY( Kahan( *Reinterpret(A), n, Reinterpret(phi) ) ) } \
  /* Laplacian */ \
  ElError ElLaplacian1D_ ## SIG ( ElMatrix_ ## SIG L, ElInt nx ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx ) ) } \
  ElError ElLaplacian1DDist_ ## SIG ( ElDistMatrix_ ## SIG L, ElInt nx ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx ) ) } \
  ElError ElLaplacian2D_ ## SIG \
  ( ElMatrix_ ## SIG L, ElInt nx, ElInt ny ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx, ny ) ) } \
  ElError ElLaplacian2DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG L, ElInt nx, ElInt ny ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx, ny ) ) } \
  ElError ElLaplacian3D_ ## SIG \
  ( ElMatrix_ ## SIG L, ElInt nx, ElInt ny, ElInt nz ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx, ny, nz ) ) } \
  ElError ElLaplacian3DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG L, ElInt nx, ElInt ny, ElInt nz ) \
  { EL_TRY( Laplacian( *Reinterpret(L), nx, ny, nz ) ) } \
  /* Legendre */ \
  ElError ElLegendre_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Legendre( *Reinterpret(A), n ) ) } \
  ElError ElLegendreDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Legendre( *Reinterpret(A), n ) ) } \
  /* Lehmer */ \
  ElError ElLehmer_ ## SIG ( ElMatrix_ ## SIG L, ElInt n ) \
  { EL_TRY( Lehmer( *Reinterpret(L), n ) ) } \
  ElError ElLehmerDist_ ## SIG ( ElDistMatrix_ ## SIG L, ElInt n ) \
  { EL_TRY( Lehmer( *Reinterpret(L), n ) ) } \
  /* Lotkin */ \
  ElError ElLotkin_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Lotkin( *Reinterpret(A), n ) ) } \
  ElError ElLotkinDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Lotkin( *Reinterpret(A), n ) ) }  \
  /* Parter */ \
  ElError ElParter_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Parter( *Reinterpret(A), n ) ) } \
  ElError ElParterDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Parter( *Reinterpret(A), n ) ) } \
  /* Pei */ \
  ElError ElPei_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) alpha ) \
  { EL_TRY( Pei( *Reinterpret(A), n, Reinterpret(alpha) ) ) } \
  ElError ElPeiDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, CREFLECT(T) alpha ) \
  { EL_TRY( Pei( *Reinterpret(A), n, Reinterpret(alpha) ) ) } \
  /* Riffle */ \
  ElError ElRiffle_ ## SIG ( ElMatrix_ ## SIG P, ElInt n ) \
  { EL_TRY( Riffle( *Reinterpret(P), n ) ) } \
  ElError ElRiffleDist_ ## SIG ( ElDistMatrix_ ## SIG P, ElInt n ) \
  { EL_TRY( Riffle( *Reinterpret(P), n ) ) } \
  ElError ElRiffleStationary_ ## SIG \
  ( ElMatrix_ ## SIG PInf, ElInt n ) \
  { EL_TRY( RiffleStationary( *Reinterpret(PInf), n ) ) } \
  ElError ElRiffleStationaryDist_ ## SIG \
  ( ElDistMatrix_ ## SIG PInf, ElInt n ) \
  { EL_TRY( RiffleStationary( *Reinterpret(PInf), n ) ) } \
  ElError ElRiffleDecay_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( RiffleDecay( *Reinterpret(A), n ) ) } \
  ElError ElRiffleDecayDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( RiffleDecay( *Reinterpret(A), n ) ) } \
  /* Ris */ \
  ElError ElRis_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Ris( *Reinterpret(A), n ) ) } \
  ElError ElRisDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Ris( *Reinterpret(A), n ) ) } \
  /* Toeplitz */ \
  ElError ElToeplitz_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt m, ElInt n, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Toeplitz( *Reinterpret(A), m, n, a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElToeplitzDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt m, ElInt n, ElInt aSize, CREFLECT(T)* aBuf ) \
  { try { \
      std::vector<T> a( Reinterpret(aBuf), Reinterpret(aBuf)+aSize ); \
      Toeplitz( *Reinterpret(A), m, n, a ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Triangle */ \
  ElError ElTriangle_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Triangle( *Reinterpret(A), n ) ) } \
  ElError ElTriangleDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Triangle( *Reinterpret(A), n ) ) } \
  /* Wigner */ \
  ElError ElWigner_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) mean, Base<T> stddev ) \
  { EL_TRY( Wigner( *Reinterpret(A), n, Reinterpret(mean), stddev ) ) } \
  ElError ElWignerDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, CREFLECT(T) mean, Base<T> stddev ) \
  { EL_TRY( Wigner( *Reinterpret(A), n, Reinterpret(mean), stddev ) ) }

#define C_PROTO_INT(SIG,SIGBASE,T) \
  C_PROTO_BASE(SIG,SIGBASE,T)

#define C_PROTO_REAL(SIG,SIGBASE,T) \
  C_PROTO_BASE(SIG,SIGBASE,T) \
  C_PROTO_NOINT(SIG,SIGBASE,T)

#define C_PROTO_COMPLEX(SIG,SIGBASE,T) \
  C_PROTO_BASE(SIG,SIGBASE,T) \
  C_PROTO_NOINT(SIG,SIGBASE,T) \
  /* Bull's head */ \
  ElError ElBullsHead_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( BullsHead( *Reinterpret(A), n ) ) } \
  ElError ElBullsHeadDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( BullsHead( *Reinterpret(A), n ) ) } \
  /* Egorov */ \
  ElError ElEgorov_ ## SIG \
  ( ElMatrix_ ## SIG A, Base<T> (*phase)(ElInt,ElInt), ElInt n ) \
  { try { \
      std::function<Base<T>(Int,Int)> phaseFunc(phase); \
      Egorov( *Reinterpret(A), phaseFunc, n ); \
    } EL_CATCH; return EL_SUCCESS; } \
  ElError ElEgorovDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, Base<T> (*phase)(ElInt,ElInt), ElInt n ) \
  { try { \
      std::function<Base<T>(Int,Int)> phaseFunc(phase); \
      Egorov( *Reinterpret(A), phaseFunc, n ); \
    } EL_CATCH; return EL_SUCCESS; } \
  /* Fox-Li */ \
  ElError ElFoxLi_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, Base<T> omega ) \
  { EL_TRY( FoxLi( *Reinterpret(A), n, omega ) ) } \
  /* Fourier */ \
  ElError ElFourier_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Fourier( *Reinterpret(A), n ) ) } \
  ElError ElFourierDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Fourier( *Reinterpret(A), n ) ) } \
  /* Helmholtz with PML */ \
  ElError ElHelmholtzPML1D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  ElError ElHelmholtzPML1DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  ElError ElHelmholtzPML2D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, ElInt ny, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, ny, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  ElError ElHelmholtzPML2DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, ElInt ny, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, ny, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  ElError ElHelmholtzPML3D_ ## SIG \
  ( ElMatrix_ ## SIG H, ElInt nx, ElInt ny, ElInt nz, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, ny, nz, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  ElError ElHelmholtzPML3DDist_ ## SIG \
  ( ElDistMatrix_ ## SIG H, ElInt nx, ElInt ny, ElInt nz, CREFLECT(T) omega, \
    ElInt numPmlPoints, Base<T> sigma, Base<T> pmlExp ) \
  { EL_TRY( \
      HelmholtzPML( \
        *Reinterpret(H), nx, ny, nz, Reinterpret(omega), \
        numPmlPoints, sigma, pmlExp ) ) } \
  /* NormalFromEVD */ \
  ElError ElNormalFromEVD_ ## SIG \
  ( ElMatrix_ ## SIG A, ElConstMatrix_ ## SIG w, ElConstMatrix_ ## SIG Z ) \
  { EL_TRY( \
      NormalFromEVD( *Reinterpret(A), *Reinterpret(w), *Reinterpret(Z) ) ) } \
  /* TODO: Distributed NormalFromEVD */ \
  /* NormalUniformSpectrum */ \
  ElError ElNormalUniformSpectrum_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, CREFLECT(T) center, Base<T> radius ) \
  { EL_TRY( \
      NormalUniformSpectrum( \
        *Reinterpret(A), n, Reinterpret(center), radius ) ) } \
  /* TODO: Distributed NormalUniformSpectrum */ \
  /* Trefethen */ \
  ElError ElTrefethen_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Trefethen( *Reinterpret(A), n ) ) } \
  ElError ElTrefethenDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Trefethen( *Reinterpret(A), n ) ) } \
  /* Uniform Helmholtz Green's */ \
  ElError ElUniformHelmholtzGreens_ ## SIG \
  ( ElMatrix_ ## SIG A, ElInt n, Base<T> lambda ) \
  { EL_TRY( UniformHelmholtzGreens( *Reinterpret(A), n, lambda ) ) } \
  ElError ElUniformHelmholtzGreensDist_ ## SIG \
  ( ElDistMatrix_ ## SIG A, ElInt n, Base<T> lambda ) \
  { EL_TRY( UniformHelmholtzGreens( *Reinterpret(A), n, lambda ) ) } \
  /* Whale */ \
  ElError ElWhale_ ## SIG ( ElMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Whale( *Reinterpret(A), n ) ) } \
  ElError ElWhaleDist_ ## SIG ( ElDistMatrix_ ## SIG A, ElInt n ) \
  { EL_TRY( Whale( *Reinterpret(A), n ) ) }

#include "El/macros/CInstantiate.h"

} // extern "C"
