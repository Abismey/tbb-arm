/*
   Copyright 2012 ARM Limited  All Rights Reserved.

   This file is part of Threading Building Blocks.

   Threading Building Blocks is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   Threading Building Blocks is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Threading Building Blocks; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   As a special exception, you may use this file as part of a free software
   library without restriction.  Specifically, if other files instantiate
   templates or use macros or inline functions from this file, or you compile
   this file and link it with other files to produce an executable, this
   file does not by itself cause the resulting executable to be covered by
   the GNU General Public License.  This exception does not however
   invalidate any other reasons why the executable file might be covered by
   the GNU General Public License.
*/

/*
   This is the TBB implementation for the ARMv7-a architecture.
*/ 

#ifndef __TBB_machine_H
#error Do not include this file directly; include tbb_machine.h instead
#endif

#if !(__ARM_ARCH_7A__)
#error Threading Building Blocks ARM port requires an ARMv7-a architecture.
#endif

#include <sys/param.h>
#include <unistd.h>

#define __TBB_WORDSIZE 4

#ifndef __BYTE_ORDER__
  // Hopefully endianness can be validly determined at runtime.
   // This may silently fail in some embedded systems with page-specific endianness.
#elif __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
   #define __TBB_BIG_ENDIAN 1
#elif __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
   #define __TBB_BIG_ENDIAN 0
#else
   #define __TBB_BIG_ENDIAN -1 // not currently supported
#endif
                

#define __TBB_compiler_fence() __asm__ __volatile__("": : :"memory")
#define __TBB_control_consistency_helper() __TBB_compiler_fence()

#define __TBB_armv7_inner_shareable_barrier() __asm__ __volatile__("dmb ish": : :"memory")
#define __TBB_acquire_consistency_helper() __TBB_armv7_inner_shareable_barrier()
#define __TBB_release_consistency_helper() __TBB_armv7_inner_shareable_barrier()
#define __TBB_full_memory_fence() __TBB_armv7_inner_shareable_barrier()


//--------------------------------------------------
// Compare and swap
//--------------------------------------------------

/**
* Atomic CAS for 32 bit values, if *ptr==comparand, then *ptr=value, returns *ptr
 * @param ptr pointer to value in memory to be swapped with value if *ptr==comparand
 * @param value value to assign *ptr to if *ptr==comparand
 * @param comparand value to compare with *ptr
 * @return value originally in memory at ptr, regardless of success
*/
static inline int32_t __TBB_machine_cmpswp4(volatile void *ptr, int32_t value, int32_t comparand )
{
  int32_t oldval, res;

  __TBB_full_memory_fence();
  
  do {
   __asm__ __volatile__(
       "@ atomic_cmpxchg\n"
       "ldrex      %1, [%3]\n"
       "mov        %0, #0\n"
       "teq        %1, %4\n"
       "strexeq    %0, %5, [%3]\n"
       : "=&r" (res), "=&r" (oldval), "+Qo" (*(volatile int32_t*)ptr)
       : "r" ((int32_t *)ptr), "Ir" (comparand), "r" (value)
       : "cc");
   } while (res);

   __TBB_full_memory_fence();

  return oldval;
}

/**
 * Atomic CAS for 64 bit values, if *ptr==comparand, then *ptr=value, returns *ptr
 * @param ptr pointer to value in memory to be swapped with value if *ptr==comparand
 * @param value value to assign *ptr to if *ptr==comparand
 * @param comparand value to compare with *ptr
 * @return value originally in memory at ptr, regardless of success
 */
static inline int64_t __TBB_machine_cmpswp8(volatile void *ptr, int64_t value, int64_t comparand )
{
   int64_t oldval;
   int32_t res;

   __TBB_full_memory_fence();

   do {
       __asm__ __volatile__(
           "@ atomic64_cmpxchg\n"
           "mov        %0, #0\n"
           "ldrexd     %1, %H1, [%3]\n"
           "teq        %1, %4\n"
           "teqeq      %H1, %H4\n"
           "strexdeq   %0, %5, %H5, [%3]"
       : "=&r" (res), "=&r" (oldval), "+Qo" (*(volatile int64_t*)ptr)
       : "r" ((int64_t *)ptr), "r" (comparand), "r" (value)
       : "cc");
   } while (res);

   __TBB_full_memory_fence();

   return oldval;
}

inline void __TBB_machine_pause (int32_t delay )
{
   while(delay>0)
   {
	__TBB_compiler_fence();
       delay--;
   }
}

namespace tbb {
namespace internal {
   template <typename T, size_t S>
   struct machine_load_store_relaxed {
       static inline T load ( const volatile T& location ) {
           const T value = location;

           /*
           * An extra memory barrier is required for errata #761319
           * Please see http://infocenter.arm.com/help/topic/com.arm.doc.uan0004a
           */
           __TBB_armv7_inner_shareable_barrier();
           return value;
       }

       static inline void store ( volatile T& location, T value ) {
           location = value;
       }
   };
}} // namespaces internal, tbb

// Machine specific atomic operations

#define __TBB_CompareAndSwap4(P,V,C) __TBB_machine_cmpswp4(P,V,C)
#define __TBB_CompareAndSwap8(P,V,C) __TBB_machine_cmpswp8(P,V,C)
#define __TBB_CompareAndSwapW(P,V,C) __TBB_machine_cmpswp4(P,V,C)
#define __TBB_Pause(V) __TBB_machine_pause(V)

// Use generics for some things
#define __TBB_USE_GENERIC_PART_WORD_CAS				1
#define __TBB_USE_GENERIC_PART_WORD_FETCH_ADD			1
#define __TBB_USE_GENERIC_PART_WORD_FETCH_STORE			1
#define __TBB_USE_GENERIC_FETCH_ADD				1
#define __TBB_USE_GENERIC_FETCH_STORE				1
#define __TBB_USE_GENERIC_HALF_FENCED_LOAD_STORE		1
#define __TBB_USE_GENERIC_DWORD_LOAD_STORE			1
#define __TBB_USE_GENERIC_SEQUENTIAL_CONSISTENCY_LOAD_STORE	1

