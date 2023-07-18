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

#include "utils/ifpga_namespaces.hpp"
#include "cut/cut.hpp"
#include "cut/cut_enumeration.hpp"
#include "utils/cost_functions.hpp"
#include "algorithms/cleanup.hpp"

#include "views/depth_view.hpp"
#include "views/topo_view.hpp"
#include "utils/node_map.hpp"
#include "balancing.hpp"
#include "balancing/utils.hpp"

#include "kitty/dynamic_truth_table.hpp"

#include <cstdint>
#include <array>
#include <functional>

iFPGA_NAMESPACE_HEADER_START

struct balance_params
{
  cut_enumeration_params cut_enumeration_ps;

  bool only_on_critical_path{false};
  bool progress{false};

  bool verbose{false};
};

struct balance_stats
{
  cut_enumeration_stats cut_enumeration_st;
  void report() const
  {
    printf("balancing stats: !!! \n");
  }
};


/// callback function
template<class Ntk>
using rebalance_function_callback_t = std::function<void(arrival_time_pair<Ntk> const&, uint32_t)>;

template<class Ntk>
using rebalance_function_t = std::function<void(Ntk&, kitty::dynamic_truth_table const&, std::vector<arrival_time_pair<Ntk>> const&, uint32_t, uint32_t, rebalance_function_callback_t<Ntk> const&)>;

namespace detail
{

template<class Ntk, class CostFn>
class balance_online_impl
{
public:
  using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::empty_cut_data>;
  using cut_set_t      = typename network_cuts_t::cut_set_t;
  using cut_t          = typename network_cuts_t::cut_t;

  balance_online_impl( Ntk const& ntk,network_cuts_t& cuts ,rebalance_function_t<Ntk> const& rebalancing_fn, balance_params const& ps, balance_stats& st , cut_enumeration_params const& cut_ps)
      : _ntk( ntk ),
        _rebalancing_fn( rebalancing_fn ),
        _ps( ps ),
        _st( st ),
        _cut_network(cuts),
        _cut_ps(cut_ps),
        _fanouts( ntk.size(), -1)
  {}

  Ntk run()
  {
    Ntk dest;
    node_map<arrival_time_pair<Ntk>, Ntk> old_to_new( _ntk );
    old_to_new[_ntk.get_constant( false )] = {dest.get_constant( false ), 0u};
    if ( _ntk.get_node( _ntk.get_constant( false ) ) != _ntk.get_node( _ntk.get_constant( true ) ) )
    {
      old_to_new[_ntk.get_constant( true )] = {dest.get_constant( true ), 0u};
    }
    _ntk.foreach_pi( [&]( auto const& n ) {
      old_to_new[n] = {dest.create_pi(), 0u};
    } );

    std::shared_ptr<depth_view<Ntk, CostFn>> depth_ntk;
    if ( _ps.only_on_critical_path )
    {
      depth_ntk = std::make_shared<depth_view<Ntk, CostFn>>( _ntk );
    }
    topo_view<Ntk>(_ntk).foreach_node([&](auto n){
      
      const auto index_node = _ntk.node_to_index(n);
      /// refer to k-feasible-cut
      _fanouts[index_node] = _ntk.fanout_size( n );
      if(_ntk.is_constant(n))
      {
        _cut_network.add_zero_cut(index_node);
        return;
      }
      else if(_ntk.is_pi(n))
      {
        _cut_network.add_unit_cut(index_node);
        return;
      }
      else
      {
        merge_cuts2(index_node);

        if ( _ps.only_on_critical_path && !depth_ntk->is_on_critical_path( n ) )
        {
          std::vector< signal<Ntk> > children;
          _ntk.foreach_fanin( n, [&]( auto const& f ) {
            const auto f_best = old_to_new[f].f;
            children.push_back( _ntk.is_complemented( f ) ? dest.create_not( f_best ) : f_best );
          });
          old_to_new[n] = {dest.clone_node( _ntk, n, children ), depth_ntk->level( n )};
          return;
        }

        arrival_time_pair<Ntk> best{{}, std::numeric_limits<uint32_t>::max()};
        uint32_t best_size{};
        for ( auto& cut : _cut_network.cuts( index_node ) )
        {
          if ( cut->size() == 1u || kitty::is_const0( _cut_network.truth_table( *cut ) ) )
          {
            continue;
          }

          std::vector<arrival_time_pair<Ntk>> arrival_times( cut->size() );
          std::transform( cut->begin(), cut->end(), arrival_times.begin(), [&]( auto leaf ) { return old_to_new[_ntk.index_to_node( leaf )]; });

          _rebalancing_fn( dest, _cut_network.truth_table( *cut ), arrival_times, best.level, best_size, [&]( arrival_time_pair<Ntk> const& cand, uint32_t cand_size ) {
            if ( cand.level < best.level || ( cand.level == best.level && cand_size < best_size ) )
            {
              best = cand;
              best_size = cand_size;
            }
          });
        }
        old_to_new[n] = best;
        auto size = _ntk.size(); 
        // trade off of runtime and memory 
        if((size <= 10000 && n % (size / 4) == 0) ||
           (size > 10000 && size <= 50000 && n % (size / 5) == 0) ||
           (size > 50000 && n % (size / 6) == 0)
        )
        {
          dest.clear_pos();
          // real po may not created yet, use the best above
          // FIXME, find real output instead of all nodes 
          _ntk.foreach_gate( [&]( auto n ) {
            const auto s = old_to_new[n].f;
            dest.create_po( s );
          } );

          node_map<signal<Ntk>, Ntk> new_to_newer(dest);
          auto tmp = sweep( dest, new_to_newer );

          _ntk.foreach_node([&](auto i)
          {
            uint64_t before = old_to_new[i].f.data;
            old_to_new[i] = { dest.is_complemented( old_to_new[i].f) ? 
                              tmp.create_not( new_to_newer[old_to_new[i].f]) : 
                              new_to_newer[old_to_new[i].f], 
                                old_to_new[i].level} ;
            //if(before != old_to_new[i].f.data)
            //{
            //  std::cout << "from " << i << " update " << before << " to " << old_to_new[i].f.data << std::endl;
            //}
          });
          dest = tmp;
        }

        _ntk.foreach_fanin(n, [&](auto s, auto i){
          auto index_child = _ntk.node_to_index( _ntk.get_node( s ) );
          if( --_fanouts[index_child]  == 0)
          {
            _cut_network.cuts(index_child).empty();
          }
        });
      }
    });

    dest.clear_pos();
    _ntk.foreach_po( [&]( auto const& f ) {
      const auto s = old_to_new[f].f;
      dest.create_po( _ntk.is_complemented( f ) ? dest.create_not( s ) : s );
    } );

    return cleanup_dangling( dest );
  }

private:

  uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
  {
    std::vector<kitty::dynamic_truth_table> tt( vcuts.size() );
    auto i = 0;
    for ( auto const& cut : vcuts )
    {
      tt[i] = kitty::extend_to( _cut_network.at((*cut)->func_id), res.size() );
      const auto supp = _cut_network.compute_truth_table_support( *cut, res );
      kitty::expand_inplace( tt[i], supp );
      ++i;
    }

    auto tt_res = _ntk.compute( _ntk.index_to_node( index ), tt.begin(), tt.end() );

    if ( _cut_ps.minimize_truth_table )
    {
      const auto support = kitty::min_base_inplace( tt_res );
      if ( support.size() != res.size() )
      {
        auto tt_res_shrink = kitty::shrink_to( tt_res, static_cast<unsigned>( support.size() ) );
        std::vector<uint32_t> leaves_before( res.begin(), res.end() );
        std::vector<uint32_t> leaves_after( support.size() );

        auto it_support = support.begin();
        auto it_leaves = leaves_after.begin();
        while ( it_support != support.end() )
        {
          *it_leaves++ = leaves_before[*it_support++];
        }
        res.set_leaves( leaves_after.begin(), leaves_after.end() );
        return _cut_network.insert_truth_table( tt_res_shrink );
      }
    }

    return _cut_network.insert_truth_table( tt_res );
  }

  void merge_cuts2( uint32_t index )
  {
    array<cut_set_t*, Ntk::max_fanin_size + 1> lcuts; // child's cut
    const auto fanin = 2;
    uint32_t pairs{1};
    _ntk.foreach_fanin( _ntk.index_to_node( index ), [&]( auto child, auto i ) {
      lcuts[i] = &_cut_network.cuts( _ntk.node_to_index( _ntk.get_node( child ) ) );
      pairs *= static_cast<uint32_t>( lcuts[i]->size() );
    } );
    lcuts[2] = &_cut_network.cuts( index );
    auto& rcuts = *lcuts[fanin];
    rcuts.clear();

    cut_t new_cut;

    std::vector<cut_t const*> vcuts( fanin );

    _cut_network.incre_total_tuples(pairs);

    for ( auto const& c1 : *lcuts[0] )
    {
      for ( auto const& c2 : *lcuts[1] )
      {
        if ( !c1->merge( *c2, new_cut, _cut_ps.cut_size ) )
        {
          continue;
        }

        if ( rcuts.is_dominated( new_cut ) )
        {
          continue;
        }
        // compute boolean function
        vcuts[0] = c1;
        vcuts[1] = c2;
        new_cut->func_id = compute_truth_table( index, vcuts, new_cut );

        if(new_cut.size() == 0)
          continue;
        rcuts.insert( new_cut );
      }
    }
    /* limit the maximum number of cuts */
    rcuts.limit( _cut_ps.cut_limit - 1 );

    _cut_network.incre_total_cuts(rcuts.size());

    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
    {
      _cut_network.add_unit_cut( index );
    }
  }
private:
  Ntk const& _ntk;
  rebalance_function_t<Ntk> const& _rebalancing_fn;
  balance_params const& _ps;
  balance_stats& _st;

  network_cuts_t&             _cut_network;
  cut_enumeration_params      _cut_ps;
  vector<int16_t>             _fanouts;
};  // end class 
} // end namespace detail

template<class Ntk, class CostFn = unit_cost<Ntk>>
Ntk balance_online( Ntk const& ntk, rebalance_function_t<Ntk> const& rebalancing_fn = {}, balance_params const& ps = {}, balance_stats* pst = nullptr )
{
  balance_stats st;
  iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::empty_cut_data> cut_network(ntk.size());
  detail::balance_online_impl<Ntk, CostFn> p(ntk, cut_network, rebalancing_fn, ps, st, ps.cut_enumeration_ps);
  const auto dest = p.run();
  if ( pst )
  {
    *pst = st;
  }
  if ( ps.verbose )
  {
    st.report();
  }

  return dest;
}




iFPGA_NAMESPACE_HEADER_END