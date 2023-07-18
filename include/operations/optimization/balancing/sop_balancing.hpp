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

#include <algorithm>
#include <cstdint>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/isop.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>

#include "utils/traits.hpp"
#include "utils/stopwatch.hpp"
#include "../balancing.hpp"
#include "utils.hpp"

iFPGA_NAMESPACE_HEADER_START

/*! \brief SOP rebalancing function
 *
 * This class can be used together with the generic `balancing` function.  It
 * converts each cut function into an SOP and then performs weight-oriented
 * tree balancing on the AND terms and the outer OR function.
 */
template<class Ntk>
struct sop_rebalancing
{
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, std::vector<arrival_time_pair<Ntk>> const& inputs, uint32_t best_level, uint32_t best_cost, rebalancing_function_callback_t<Ntk> const& callback ) const
  {
    auto [and_terms, num_and_gates] = create_function( dest, function, inputs );
    const auto num_gates = num_and_gates + ( and_terms.empty() ? 0u : static_cast<uint32_t>( and_terms.size() ) - 1u );
    const auto cand = balanced_tree( dest, and_terms, false );

    if ( cand.level < best_level || ( cand.level == best_level && num_gates < best_cost ) )
    {
      callback( cand, num_gates );
    }
  }

private:
  std::pair<arrival_time_queue<Ntk>, uint32_t> create_function( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times ) const
  {
    const auto sop = create_sop_form( func );

    stopwatch<> t_tree( time_tree_balancing );
    arrival_time_queue<Ntk> and_terms;
    uint32_t num_and_gates{};
    for ( auto const& cube : sop )
    {
      arrival_time_queue<Ntk> product_queue;
      for ( auto i = 0u; i < func.num_vars(); ++i )
      {
        if ( cube.get_mask( i ) )
        {
          const auto [f, l] = arrival_times[i];
          product_queue.push( {cube.get_bit( i ) ? f : dest.create_not( f ), l} );
        }
      }
      if ( !product_queue.empty() )
      {
        num_and_gates += static_cast<uint32_t>( product_queue.size() ) - 1u;
      }
      and_terms.push( balanced_tree( dest, product_queue ) );
    }
    return {and_terms, num_and_gates};
  }

  arrival_time_pair<Ntk> balanced_tree( Ntk& dest, arrival_time_queue<Ntk>& queue, bool _and = true ) const
  {
    if ( queue.empty() )
    {
      return {dest.get_constant( true ), 0u};
    }

    while ( queue.size() > 1u )
    {
      auto [s1, l1] = queue.top();
      queue.pop();
      auto [s2, l2] = queue.top();
      queue.pop();
      const auto s = _and ? dest.create_and( s1, s2 ) : dest.create_or( s1, s2 );
      const auto l = std::max( l1, l2 ) + 1;
      queue.push( {s, l} );
    }
    return queue.top();
  }

  std::vector<kitty::cube> create_sop_form( kitty::dynamic_truth_table const& func ) const
  {
    stopwatch<> t( time_sop );
    if ( auto it = sop_hash_.find( func ); it != sop_hash_.end() )
    {
      sop_cache_hits++;
      return it->second;
    }
    else
    {
      sop_cache_misses++;
      return sop_hash_[func] = kitty::isop( func ); // TODO generalize
    }
  }

private:
  mutable std::unordered_map<kitty::dynamic_truth_table, std::vector<kitty::cube>, kitty::hash<kitty::dynamic_truth_table>> sop_hash_;

public:
  mutable uint32_t sop_cache_hits{};
  mutable uint32_t sop_cache_misses{};

  mutable stopwatch<>::duration time_sop{};
  mutable stopwatch<>::duration time_tree_balancing{};
};

iFPGA_NAMESPACE_HEADER_END