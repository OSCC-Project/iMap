/**
 * @file ref_deref.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief the ref and deref function of a node
 * 
 * @version 0.1
 * @date 2021-07-26
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/common_properties.hpp"
#include "utils/cost_functions.hpp"

#include <stdint.h>
#include <tuple>

iFPGA_NAMESPACE_HEADER_START

template<typename Ntk, typename NodeCostFn, typename TermConditionFn>
uint32_t deref_node_recursive_cond(Ntk const& ntk, node<Ntk> const& n, TermConditionFn const& term_fn)
{
  if ( term_fn( n ) )
    return 0u;

  uint32_t value = NodeCostFn{}( ntk, n );
  ntk.foreach_fanin( n, [&]( auto const& s ) {
    if ( ntk.decr_value( ntk.get_node( s ) ) == 0 )
    {
      value += deref_node_recursive_cond<Ntk, NodeCostFn, TermConditionFn>( ntk, ntk.get_node( s ), term_fn );
    }
  } );
  return value;
}

template<typename Ntk, typename NodeCostFn, typename TermConditionFn>
uint32_t ref_node_recursive_cond(Ntk const& ntk, node<Ntk> const& n, TermConditionFn const& term_fn)
{
  if ( term_fn( n ) )
    return 0u;

  uint32_t value = NodeCostFn{}( ntk, n );
  ntk.foreach_fanin( n, [&]( auto const& s ) {
    if ( ntk.incr_value( ntk.get_node( s ) ) == 0 )
    {
      value += ref_node_recursive_cond<Ntk, NodeCostFn, TermConditionFn>( ntk, ntk.get_node( s ), term_fn );
    }
  } );
  return value;
}

template<typename Ntk, typename NodeCostFn>
uint32_t deref_node_recursive(Ntk const& ntk, node<Ntk> const& n)
{
  const auto term_fn = [&](const auto& n){ return ntk.is_constant(n) || ntk.is_pi(n); };
  return deref_node_recursive_cond<Ntk, NodeCostFn, decltype(term_fn)>(ntk, n, term_fn);
}

template<typename Ntk, typename NodeCostFn>
uint32_t ref_node_recursive(Ntk const& ntk, node<Ntk> const& n)
{
  const auto term_fn = [&](const auto& n){ return ntk.is_constant(n) || ntk.is_pi(n); };
  return ref_node_recursive_cond<Ntk, NodeCostFn, decltype(term_fn)>(ntk, n, term_fn);
}

/**
 * @brief ref the node ,and check if the mffc contains the repalced_node
 */
template<typename Ntk>
std::tuple<int, bool> ref_node_recursive_contained(Ntk& ntk, node<Ntk> const& n, node<Ntk> const& replaced_node)
{
  if( ntk.is_constant(n) || ntk.is_pi(n) )
    return {0, false};
  
  int value = 0;
  bool contains = ( n == replaced_node);
  ntk.foreach_fanin(n, [&](auto const& s){
    contains = contains || (ntk.get_node(s) == replaced_node);
    if(ntk.decr_value( ntk.get_node(s)) == 0 )
    {
      const auto tmp_res = ref_node_recursive_contained(ntk, ntk.get_node(s), replaced_node);
      value += std::get<0>(tmp_res);
      contains = contains || std::get<1>(tmp_res);
    }
  });
  return std::make_tuple(value, contains);
}

template<typename Ntk, typename NodeCostFn = unit_cost<Ntk>>
uint32_t derefed_size(Ntk const& ntk, node<Ntk> const& n)
{
  uint32_t s1 = deref_node_recursive<Ntk, NodeCostFn>(ntk, n);
  uint32_t s2 = ref_node_recursive<Ntk, NodeCostFn>(ntk, n);
  assert(s1 == s2);
  return s1;
}

template<typename Ntk, typename NodeCostFn = unit_cost<Ntk>>
uint32_t refed_size(Ntk const& ntk, node<Ntk> const& n)
{
  uint32_t s1 = ref_node_recursive<Ntk, NodeCostFn>(ntk, n);
  uint32_t s2 = deref_node_recursive<Ntk, NodeCostFn>(ntk, n);
  assert(s1 == s2);
  return s1;
}

iFPGA_NAMESPACE_HEADER_END