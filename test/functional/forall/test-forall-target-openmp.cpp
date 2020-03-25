//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "tests/test-forall-segment.hpp"

#if defined(RAJA_ENABLE_TARGET_OPENMP)

// OpenMP target execution policy types
using OMPTargetTypes = list< RAJA::omp_target_parallel_for_exec<8>,
                             RAJA::omp_target_parallel_for_exec_nt >;

// OpenMP target tests index, resource, and execution policy types
using OMPTargetForallTypes =
    Test<cartesian_product<IdxTypes, ListHost, OMPTargetTypes>>::Types;

INSTANTIATE_TYPED_TEST_SUITE_P(TargetOmp,
                               ForallSegmentTest,
                               OMPTargetForallTypes);
#endif
