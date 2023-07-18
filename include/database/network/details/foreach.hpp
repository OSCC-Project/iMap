// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Shanghai Anlogic Infotech Co.,Ltd.
// Copyright (c) 2023-2025 Peking University
//
// iMAP-FPGA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************

#pragma once

#include <cstdint>
#include <type_traits>
#include "utils/ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

namespace detail
{

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_with_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType, uint32_t>;

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_without_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType>;

template<class Iterator, class ElementType = typename Iterator::value_type, class Fn>
Iterator foreach_element( Iterator begin, Iterator end, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !fn( *begin++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      if ( !fn( *begin++, index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      fn( *begin++ );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      fn( *begin++, index++ );
    }
    return begin;
  }
}

template<class Iterator, class ElementType = typename Iterator::value_type, class Pred, class Fn>
Iterator foreach_element_if( Iterator begin, Iterator end, Pred&& pred, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      if ( !fn( *begin++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      if ( !fn( *begin++, index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      fn( *begin++ );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      fn( *begin++, index++ );
    }
    return begin;
  }
}

template<class Iterator, class ElementType, class Transform, class Fn>
Iterator foreach_element_transform( Iterator begin, Iterator end, Transform&& transform, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !fn( transform( *begin++ ) ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      if ( !fn( transform( *begin++ ), index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      fn( transform( *begin++ ) );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{counter_offset};
    while ( begin != end )
    {
      fn( transform( *begin++ ), index++ );
    }
    return begin;
  }
}

} /* namespace detail */
iFPGA_NAMESPACE_HEADER_END