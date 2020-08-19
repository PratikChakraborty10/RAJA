//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//
// Execution policy lists used throughout teams tests
//

#ifndef __RAJA_test_teams_execpol_HPP__
#define __RAJA_test_teams_execpol_HPP__

#include "RAJA/RAJA.hpp"
#include "camp/list.hpp"

//Launch policies
#if defined(RAJA_ENABLE_CUDA)
using seq_cuda_policies = camp::list<
  RAJA::expt::LaunchPolicy<RAJA::expt::seq_launch_t,RAJA::expt::cuda_launch_t<false>>,
  RAJA::expt::LoopPolicy<RAJA::loop_exec, RAJA::cuda_block_x_direct>,
  RAJA::expt::LoopPolicy<RAJA::loop_exec,RAJA::cuda_thread_x_loop>>;

using Sequential_launch_policies = camp::list<
        seq_cuda_policies
         >;

#elif defined(RAJA_ENABLE_HIP)
using Sequential_launch_policies = camp::list<
        camp::list<
         RAJA::expt::LaunchPolicy<RAJA::expt::seq_launch_t,RAJA::expt::hip_launch_t<false>>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec, RAJA::hip_block_x_direct>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec,RAJA::hip_thread_x_loop>>>;
#else
using Sequential_launch_policies = camp::list<
        camp::list<
         RAJA::expt::LaunchPolicy<RAJA::expt::seq_launch_t>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec>>>;
#endif


#if defined(RAJA_ENABLE_OPENMP)

#if defined(RAJA_ENABLE_CUDA)

using omp_cuda_policies = camp::list<
         RAJA::expt::LaunchPolicy<RAJA::expt::omp_launch_t,RAJA::expt::cuda_launch_t<false>>,
         RAJA::expt::LoopPolicy<RAJA::omp_parallel_for_exec, RAJA::cuda_block_x_direct>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec,RAJA::cuda_thread_x_loop>
  >;

using OpenMP_launch_policies = camp::list<
         omp_cuda_policies
         >;

#elif defined(RAJA_ENABLE_HIP)
using OpenMP_launch_policies = camp::list<
        camp::list<
         RAJA::expt::LaunchPolicy<RAJA::expt::omp_launch_t,RAJA::expt::hip_launch_t<false>>,
         RAJA::expt::LoopPolicy<RAJA::omp_parallel_for_exec, RAJA::hip_block_x_direct>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec,RAJA::hip_thread_x_loop>>>;
#else
using OpenMP_launch_policies = camp::list<
        camp::list<
         RAJA::expt::LaunchPolicy<RAJA::expt::omp_launch_t>,
         RAJA::expt::LoopPolicy<RAJA::omp_parallel_for_exec>,
         RAJA::expt::LoopPolicy<RAJA::loop_exec>>>;
#endif

#endif  // RAJA_ENABLE_OPENMP

#if defined(RAJA_ENABLE_CUDA)
using Cuda_launch_policies = camp::list<
         seq_cuda_policies
#if defined(RAJA_ENABLE_OPENMP)
         , omp_cuda_policies
#endif
        >;
#endif


#endif  // __RAJA_test_teams_execpol_HPP__
