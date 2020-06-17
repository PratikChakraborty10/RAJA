/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA WorkGroup templates for
 *          hip execution.
 *
 *          These methods should work on any platform.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_hip_WorkGroup_HPP
#define RAJA_hip_WorkGroup_HPP

#include "RAJA/config.hpp"

#include "RAJA/policy/hip/MemUtils_HIP.hpp"
#include "RAJA/policy/hip/policy.hpp"

#include "RAJA/pattern/detail/WorkGroup.hpp"


#if defined(RAJA_ENABLE_HIP)

namespace RAJA
{

namespace detail
{

#if defined(RAJA_ENABLE_HIP_INDIRECT_FUNCTION_CALL)

template < typename T, typename ... CallArgs >
__device__ void Vtable_hip_device_call(const void* obj, CallArgs... args)
{
    const T* obj_as_T = static_cast<const T*>(obj);
    (*obj_as_T)(std::forward<CallArgs>(args)...);
}

template < typename T, typename ... CallArgs >
__global__ void get_device_Vtable_hip_device_call(
    Vtable_call_sig<CallArgs...>* ptrptr)
{
  *ptrptr = &Vtable_hip_device_call<T, CallArgs...>;
}

inline void* get_Vtable_hip_device_call_ptrptr()
{
  void* ptrptr = nullptr;
  hipErrchk(hipHostMalloc(&ptrptr, sizeof(Vtable_call_sig<>)));
  return ptrptr;
}

inline void* get_cached_Vtable_hip_device_call_ptrptr()
{
  static void* ptrptr = get_Vtable_hip_device_call_ptrptr();
  return ptrptr;
}

// TODO: make thread safe
template < typename T, typename ... CallArgs >
inline Vtable_call_sig<CallArgs...> get_Vtable_hip_device_call()
{
  Vtable_call_sig<CallArgs...>* ptrptr =
      static_cast<Vtable_call_sig<CallArgs...>*>(
        get_cached_Vtable_hip_device_call_ptrptr());
  auto func = get_device_Vtable_hip_device_call<T, CallArgs...>;
  hipLaunchKernelGGL(func,
      dim3(1), dim3(1), 0, 0, ptrptr);
  hipErrchk(hipGetLastError());
  hipErrchk(hipDeviceSynchronize());

  return *ptrptr;
}

template < typename T, typename ... CallArgs >
inline Vtable_call_sig<CallArgs...> get_cached_Vtable_hip_device_call()
{
  static Vtable_call_sig<CallArgs...> ptr =
      get_Vtable_hip_device_call<T, CallArgs...>();
  return ptr;
}

/*!
* Populate and return a Vtable object where the
* call operator is a device function
*/
template < typename T, typename ... CallArgs >
inline Vtable<CallArgs...> get_Vtable(hip_work const&)
{
  return Vtable<CallArgs...>{
        &Vtable_move_construct<T, CallArgs...>,
        get_cached_Vtable_hip_device_call<T, CallArgs...>(),
        &Vtable_destroy<T, CallArgs...>,
        sizeof(T)
      };
}

#endif


/*!
 * Runs work in a storage container in order
 * and returns any per run resources
 */
template <typename ALLOCATOR_T,
          typename INDEX_T,
          typename ... Args>
struct WorkRunner<
        RAJA::hip_work,
        RAJA::ordered,
        ALLOCATOR_T,
        INDEX_T,
        Args...>
    : WorkRunnerForallOrdered<
        RAJA::hip_exec_async<256>,
        RAJA::hip_work,
        RAJA::ordered,
        ALLOCATOR_T,
        INDEX_T,
        Args...>
{ };

/*!
 * Runs work in a storage container in reverse order
 * and returns any per run resources
 */
template <typename ALLOCATOR_T,
          typename INDEX_T,
          typename ... Args>
struct WorkRunner<
        RAJA::hip_work,
        RAJA::reverse_ordered,
        ALLOCATOR_T,
        INDEX_T,
        Args...>
    : WorkRunnerForallReverse<
        RAJA::hip_exec_async<256>,
        RAJA::hip_work,
        RAJA::reverse_ordered,
        ALLOCATOR_T,
        INDEX_T,
        Args...>
{ };

}  // namespace detail

}  // namespace RAJA

#endif

#endif  // closing endif for header file include guard
