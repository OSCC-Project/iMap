/**
 * @file range.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief range function for iterators
 * @version 0.1
 * @date 2021-07-12
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"

#include <iterator>

iFPGA_NAMESPACE_HEADER_START

template<class Iterator, class T, class BinaryOperation>
T tree_reduce( Iterator first, Iterator last, T const& init, BinaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( *first, *( first + 1 ) );
  default:
  {
    const auto m = len / 2;
    return op( tree_reduce( first, first + m, init, op ), tree_reduce( first + m, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class T, class TernaryOperation>
T ternary_tree_reduce( Iterator first, Iterator last, T const& init, TernaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( init, *first, *( first + 1 ) );
  case 3u:
    return op( *first, *( first + 1 ), *( first + 2 ) );
  default:
  {
    const auto m1 = len / 3;
    const auto m2 = ( len - m1 ) / 2;
    return op( ternary_tree_reduce( first, first + m1, init, op ),
               ternary_tree_reduce( first + m1, first + m1 + m2, init, op ),
               ternary_tree_reduce( first + m1 + m2, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class UnaryOperation, class T>
Iterator max_element_unary( Iterator first, Iterator last, UnaryOperation&& fn, T const& init )
{
  auto best = last;
  auto max = init;
  for ( ; first != last; ++first )
  {
    if ( const auto v = fn( *first ) > max )
    {
      max = v;
      best = first;
    }
  }
  return best;
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T begin, T end )
{
  struct iterator
  {
    using value_type = T;

    value_type curr_;
    bool operator!=( iterator const& other ) const { return curr_ != other.curr_; }
    iterator& operator++() { ++curr_; return *this; }
    iterator operator++(int) { auto copy = *this; ++(*this); return copy; }
    value_type operator*() const { return curr_; }
  };
  struct iterable_wrapper
  {
    T begin_;
    T end_;
    auto begin() { return iterator{begin_}; }
    auto end() { return iterator{end_}; }
  };
  return iterable_wrapper{begin, end};
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T end )
{
  return range<T>( {}, end );
}

iFPGA_NAMESPACE_HEADER_END