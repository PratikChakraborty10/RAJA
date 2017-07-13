/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file containing an implementation of a memory pool.
 *
 ******************************************************************************
 */

#ifndef RAJA_BASIC_MEMPOOL_HPP
#define RAJA_BASIC_MEMPOOL_HPP

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For additional details, please also read RAJA/LICENSE.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the disclaimer below.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of the LLNS/LLNL nor the names of its contributors may
//   be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "RAJA/util/align.hpp"

#include <cassert>
#include <list>
#include <map>
#include <cstddef>
#include <cstdlib>

namespace RAJA {

namespace basic_mempool {

namespace detail {


class memory_arena {
public:


  using free_type = std::map<void*, void*>;
  using free_value_type = typename free_type::value_type;
  using used_type = std::map<void*, void*>;
  using used_value_type = typename used_type::value_type;

  memory_arena(void* ptr, size_t size)
    : m_allocation{ ptr, static_cast<char*>(ptr)+size },
      m_free_space({ free_value_type{ptr, static_cast<char*>(ptr)+size} }),
      m_used_space()
  {
    if (m_allocation.begin == nullptr) {
      fprintf(stderr, "Attempt to create memory_arena with no memory");
      std::abort();
    }
  }

  memory_arena(memory_arena const&) = delete;
  memory_arena& operator=(memory_arena const&) = delete;

  memory_arena(memory_arena &&) = default;
  memory_arena& operator=(memory_arena &&) = default;

  ~memory_arena()
  {

  }

  size_t capacity()
  {
    return static_cast<char*>(m_allocation.end) - static_cast<char*>(m_allocation.begin);
  }

  bool unused ()
  {
    return m_used_space.empty();
  }

  void* get_allocation()
  {
    return m_allocation.begin;
  }

  void* get(size_t nbytes, size_t alignment)
  {
    void* ptr_out = nullptr;
    if (capacity() >= nbytes) {
      free_type::iterator end = m_free_space.end();
      for (free_type::iterator iter = m_free_space.begin(); iter != end; ++iter) {

        void* adj_ptr = iter->first;
        size_t cap = static_cast<char*>(iter->second) - static_cast<char*>(adj_ptr);

        if (::RAJA::align(alignment, nbytes, adj_ptr, cap)) {

          ptr_out = adj_ptr;

          remove_free_chunk(iter, adj_ptr, static_cast<char*>(adj_ptr) + nbytes);

          add_used_chunk(adj_ptr, static_cast<char*>(adj_ptr) + nbytes);
          
          break;
        }

      }
    }
    return ptr_out;
  }

  bool give(void* ptr)
  {
    if ( m_allocation.begin <= ptr && ptr < m_allocation.end ) {

      used_type::iterator found = m_used_space.find(ptr);

      if ( found != m_used_space.end() ) {

        add_free_chunk(found->first, found->second);

        m_used_space.erase(found);

      } else {
        fprintf(stderr, "Invalid free %p", ptr);
        std::abort();
      }

      return true;
    } else {
      return false;
    }
  }

private:
  struct memory_chunk {
    void* begin;
    void* end;
  };

  void add_free_chunk(void* begin, void* end)
  {
    // integrates a chunk of memory into free_space
    free_type::iterator invl = m_free_space.end();
    free_type::iterator next = m_free_space.lower_bound(begin);

    // check if prev exists
    if (next != m_free_space.begin()) {
      // check if prev can cover [begin, end)
      free_type::iterator prev = next; --prev;
      if (prev->second == begin) {
        // extend prev to cover [begin, end)
        prev->second = end;

        // check if prev can cover next too
        if (next != invl) {
          assert(next->first != begin);

          if (next->first == end) {
            // extend prev to cover next too
            prev->second = next->second;

            // remove redundant next
            m_free_space.erase(next);
          }
        }
        return;
      }
    }

    if (next != invl) {
      assert(next->first != begin);
      
      if (next->first == end) {
        // extend next to cover [begin, end)
        m_free_space.insert(next, free_value_type{begin, next->second});
        m_free_space.erase(next);

        return;
      }
    }

    // no free space adjacent to this chunk, add seperate free chunk [begin, end)
    m_free_space.insert(next, free_value_type{begin, end});
  }

  void remove_free_chunk(free_type::iterator iter, void* begin, void* end) {

    void* ptr = iter->first;
    void* ptr_end = iter->second;

    // fixup m_free_space, shrinking and adding chunks as needed
    if (ptr != begin) {

      // shrink end of current free region to [ptr, begin)
      iter->second = begin;

      if (end != ptr_end) {

        // insert free region [end, ptr_end) after current free region
        free_type::iterator next = iter; ++next;
        m_free_space.insert(next, free_value_type{end, ptr_end});
      }

    } else if (end != ptr_end) {

      // shrink beginning of current free region to [end, ptr_end)
      free_type::iterator next = iter; ++next;
      m_free_space.insert(next, free_value_type{end, ptr_end});
      m_free_space.erase(iter);

    } else {

      // can not reuse current region, erase
      m_free_space.erase(iter);
    }
  }

  void add_used_chunk(void* begin, void* end)
  {
    // simply inserts a chunk of memory into used_space
    m_used_space.insert(used_value_type{begin, end});
  }

  memory_chunk m_allocation;
  free_type m_free_space;
  used_type m_used_space;
};

} /* end namespace detail */



template < typename allocator_t >
class mempool {
public:
  using allocator_type = allocator_t;

  static inline mempool<allocator_t>& getInstance()
  {
    static mempool<allocator_t> pool{};
    return pool;
  }

  static const size_t default_default_arena_size = 32ull*1024ull*1024ull;

  mempool()
    : m_arenas(),
      m_default_arena_size(default_default_arena_size),
      m_alloc()
  {

  }

  size_t arena_size()
  {
    return m_default_arena_size;
  }

  size_t arena_size(size_t new_size)
  {
    size_t prev_size = m_default_arena_size;
    m_default_arena_size = new_size;
    return prev_size;
  }

  template <typename T>
  T* malloc(size_t nTs, size_t alignment = alignof(T))
  {
    const size_t size = nTs*sizeof(T);
    void* ptr = nullptr;
    arena_container_type::iterator end = m_arenas.end();
    for (arena_container_type::iterator iter = m_arenas.begin(); iter != end; ++iter ) {
      ptr = iter->get(size, alignment);
      if (ptr != nullptr) {
        break;
      }
    }

    if (ptr == nullptr) {
      const size_t alloc_size = std::max(size+alignment, m_default_arena_size);
      void* arena_ptr = m_alloc.malloc(alloc_size);
      if (arena_ptr != nullptr) {
        m_arenas.emplace_front(arena_ptr, alloc_size);
        ptr = m_arenas.front().get(size, alignment);
      }
    }

    return static_cast<T*>(ptr);
  }

  void free(const void* cptr)
  {
    void* ptr = const_cast<void*>(cptr);
    arena_container_type::iterator end = m_arenas.end();
    for (arena_container_type::iterator iter = m_arenas.begin(); iter != end; ++iter ) {
      if (iter->give(ptr)) {
        ptr = nullptr;
        break;
      }
    }
    if (ptr != nullptr) {
      fprintf(stderr, "Unknown pointer %p", ptr);
    }
  }

private:
  using arena_container_type = std::list<detail::memory_arena>;

  arena_container_type m_arenas;
  size_t m_default_arena_size;
  allocator_t m_alloc;
};


struct generic_allocator {

  // returns a valid pointer on success, nullptr on failure
  void* malloc(size_t nbytes)
  {
    return malloc( nbytes);
  }

  // returns true on success, false on failure
  bool free(void* ptr)
  {
    free(ptr);
    return true;
  }

};

} /* end namespace basic_mempool */

} /* end namespace RAJA */



#endif /* BASIC_MEMPOOL_HXX_ */
