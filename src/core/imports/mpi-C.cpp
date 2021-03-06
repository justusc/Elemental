/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"
#include "El.h"

extern "C" {

ElError ElMPICommSameSizeAsInteger( bool* sameSize )
{ *sameSize = El::mpi::CommSameSizeAsInteger(); return EL_SUCCESS; }

ElError ElMPIGroupSameSizeAsInteger( bool* sameSize )
{ *sameSize = El::mpi::GroupSameSizeAsInteger(); return EL_SUCCESS; }

ElError ElMPICommWorld( MPI_Comm* commWorld )
{ *commWorld = MPI_COMM_WORLD; return EL_SUCCESS; }

ElError ElMPICommSelf( MPI_Comm* commSelf )
{ *commSelf = MPI_COMM_SELF; return EL_SUCCESS; }

ElError ElMPICommRank( MPI_Comm comm, int* rank )
{ EL_TRY( *rank = El::mpi::Rank(El::mpi::Comm(comm)) ) }
ElError ElMPIGroupRank( MPI_Group group, int* rank )
{ EL_TRY( *rank = El::mpi::Rank(El::mpi::Group(group)) ) }

ElError ElMPICommSize( MPI_Comm comm, int* size )
{ EL_TRY( *size = El::mpi::Size(El::mpi::Comm(comm)) ) }
ElError ElMPIGroupSize( MPI_Group group, int* size )
{ EL_TRY( *size = El::mpi::Size(El::mpi::Group(group)) ) }

ElError ElMPICommFree( MPI_Comm* comm )
{ EL_TRY( El::mpi::Comm wrap(*comm); El::mpi::Free(wrap) ) }
ElError ElMPIGroupFree( MPI_Group* group )
{ EL_TRY( El::mpi::Group wrap(*group); El::mpi::Free(wrap) ) }

ElError ElMPIWorldRank( int* rank )
{ EL_TRY( *rank = El::mpi::WorldRank() ) }

ElError ElMPIWorldSize( int* size )
{ EL_TRY( *size = El::mpi::WorldSize() ) }

} // extern "C"
