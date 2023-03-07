/**
 * @file rewrite.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief technology-independent optimization for AIG network
 * @version 0.1
 * @date 2021-07-24
 * @copyright Copyright (c) 2021
 */
#pragma once
#include <map>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "algorithms/cleanup.hpp"
#include "algorithms/ref_deref.hpp"
#include "cut/cut_enumeration.hpp"
#include "cut/rewrite_cut_enumeration.hpp"
#include "detail/node_resynthesis.hpp"
#include "network/aig_network.hpp"
#include "utils/ifpga_namespaces.hpp"
#include "utils/node_map.hpp"
#include "views/depth_view.hpp"
#include "views/topo_view.hpp"

using namespace std;

iFPGA_NAMESPACE_HEADER_START

    struct rewrite_params
{
  rewrite_params()
  {
    cut_enumeration_ps.cut_size             = 4;
    cut_enumeration_ps.cut_limit            = 10;
    cut_enumeration_ps.minimize_truth_table = true;
  }
  iFPGA_NAMESPACE::cut_enumeration_params cut_enumeration_ps{};

  uint32_t min_candidate_cut_size{ 4u };  // the input of a cut min and max limits
  uint32_t max_candidate_cut_size{ 4u };

  bool use_zero_gain{ true };
  bool preserve_depth{ false };
};
namespace detail {
/**
 * @brief the rewrite calss detail complement
 *    rewrite by create a euqivelent network from the source
 *    'Ntk' we deafult set to AIG
 *    'ResynthesisFn' means the rewrite function of the node's cut
 *    'NodeCostFn' means the cost fucntion of the node's cut
 */
template <typename Ntk, typename ResynthesisFn, typename NodeCostFn>
class rewrite_impl
{
 public:
  using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::cut_enumeration_rewrite_cut>;

  rewrite_impl( Ntk const&            ntk,
                const rewrite_params& params,
                const ResynthesisFn&  resynthesis_fn,
                const NodeCostFn&     node_cost_fn )
      : _ntk( ntk ),
        _resynthesis_fn( resynthesis_fn ),
        _node_cost_fn( node_cost_fn ),
        _params( params ),
        _cut_network( cut_enumeration<Ntk, true, cut_enumeration_rewrite_cut>( _ntk, _params.cut_enumeration_ps ) )
  {
  }

  /**
   * @brief the main algorithm procedure
   */
  Ntk run()
  {
    init_nodes();
    Ntk dest;
    /// record the network
    node_map<typename Ntk::signal, Ntk> map_old_to_new( _ntk );

    /// create constants
    map_old_to_new[_ntk.get_constant( false )] = { 0, 0 };
    // map_old_to_new[_ntk.get_constant( true )] = {0,1};
    /// create PIs
    _ntk.foreach_pi( [&]( auto const& n ) { map_old_to_new[n] = dest.create_pi(); } );

    //////////////////////////////////////////////////////
    ///   the algorithm of DAG-aware AIG rewriting,06
    //////////////////////////////////////////////////////
    uint32_t cost_original = compute_total_cost( _ntk );
    /// create gates
    _ntk.foreach_gate( [&]( auto const& n, auto i ) {
      int32_t tmp_defered_size = derefed_size<Ntk, NodeCostFn>( _ntk, n );
      if ( tmp_defered_size == 1 )  // not have to opt
      {
        std::vector<typename Ntk::signal> children( _ntk.fanin_size( n ) );
        _ntk.foreach_fanin( n, [&]( auto const& s, auto i ) {
          children[i] = _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s];
        } );
        map_old_to_new[n] = dest.clone_node( _ntk, n, children );
      } else {
        int32_t              best_gain = -1;
        typename Ntk::signal best_signal;
        for ( auto& cut : _cut_network.cuts( _ntk.node_to_index( n ) ) ) {
          if ( cut->size() < _params.min_candidate_cut_size || cut->size() > _params.max_candidate_cut_size )
            continue;

          std::vector<typename Ntk::signal> leaves( cut->size() );
          auto                              index = 0u;
          for ( auto l : *cut ) {
            leaves[index++] = map_old_to_new[_ntk.index_to_node( l )];
          }
          /// resynthesis of the current cut for the isologue
          _resynthesis_fn.run( dest, _cut_network.truth_table( *cut ), leaves.begin(), leaves.end(), [&]( auto const& s ) {
            uint32_t tmp_defered_size2 = ref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( s ) );
            deref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( s ) );

            int32_t gain = tmp_defered_size - tmp_defered_size2;
            if ( gain > best_gain && ( gain > 0 || ( _params.use_zero_gain && gain == 0 ) ) ) {
              best_gain   = gain;
              best_signal = s;
            }
            return true;
          } );
        }
        if ( best_gain == -1 ) {
          std::vector<typename Ntk::signal> children( _ntk.fanin_size( n ) );
          _ntk.foreach_fanin( n, [&]( auto const& s, auto i ) {
            children[i] = _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s];
          } );
          map_old_to_new[n] = dest.clone_node( _ntk, n, children );
        } else {
          map_old_to_new[n] = best_signal;
        }
      }
      /// ref the current node
      ref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( map_old_to_new[n] ) );
    } );

    /// create POs
    _ntk.foreach_po( [&]( auto const& s ) {
      dest.create_po( _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s] );
    } );
    /// clean up the dangling nodes
    dest = cleanup_dangling<Ntk>( dest );

    const auto cost_res = compute_total_cost( dest );
    // printf("costs: %d -- %d\n", cost_original, cost_res);
    return cost_res > cost_original ? _ntk : dest;
  }

 private:
  /**
   * @brief initizlize the node's tmp_defered_size with fanout
   */
  void init_nodes()
  {
    _ntk.clear_values();
    _ntk.foreach_node( [&]( auto const& n ) { _ntk.set_value( n, _ntk.fanout_size( n ) ); } );
  }

  /**
   * @brief compute the total cost of the entire network
   *    we default using the unit_cost for the average node's size cost
   */
  uint32_t compute_total_cost( const Ntk& ntk )
  {
    uint32_t total_cost = 0u;
    ntk.foreach_gate( [&]( auto const& n ) { total_cost += _node_cost_fn( ntk, n ); } );
    return total_cost;
  }

 private:
  Ntk const&            _ntk;
  ResynthesisFn const&  _resynthesis_fn;
  NodeCostFn const&     _node_cost_fn;
  rewrite_params const& _params;
  network_cuts_t        _cut_network;
};  // end class rewrite_impl

template <typename Ntk, typename ResynthesisFn, typename NodeCostFn>
class rewrite_online_impl
{
 public:
  using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::cut_enumeration_rewrite_cut>;
  using cut_set_t      = typename network_cuts_t::cut_set_t;
  using cut_t          = typename network_cuts_t::cut_t;

  rewrite_online_impl( Ntk const&                    ntk,
                       network_cuts_t&               cuts,
                       const rewrite_params&         ps,
                       const ResynthesisFn&          resynthesis_fn,
                       const NodeCostFn&             node_cost_fn,
                       cut_enumeration_params const& cut_ps )
      : _ntk( ntk ),
        _resynthesis_fn( resynthesis_fn ),
        _node_cost_fn( node_cost_fn ),
        _params( ps ),
        _cut_network( cuts ),
        _cut_ps( cut_ps ),
        _fanouts( ntk.size(), -1 )
  {
  }

  /**
   * @brief the main algorithm procedure
   */
  Ntk run()
  {
    init_nodes();
    Ntk dest;
    /// record the network
    node_map<typename Ntk::signal, Ntk> map_old_to_new( _ntk );
    // record depth info
    depth_view<Ntk> dp_view( _ntk );
    _ntk.foreach_node( [&]( auto n ) { _depth_map.insert( std::make_pair( n, dp_view.level( n ) ) ); } );

    /// create constants
    map_old_to_new[_ntk.get_constant( false )] = { 0, 0 };
    // map_old_to_new[_ntk.get_constant( true )] = {0,1};
    /// create PIs
    _ntk.foreach_pi( [&]( auto const& n ) { map_old_to_new[n] = dest.create_pi(); } );

    //////////////////////////////////////////////////////
    ///   the algorithm of DAG-aware AIG rewriting,06
    //////////////////////////////////////////////////////
    uint32_t cost_original = compute_total_cost( _ntk );
    topo_view<Ntk>( _ntk ).foreach_node( [&]( auto n ) {
      const auto index_node = _ntk.node_to_index( n );
      /// refer to k-feasible-cut
      _fanouts[index_node] = _ntk.fanout_size( n );
      if ( _ntk.is_constant( n ) ) {
        _cut_network.add_zero_cut( index_node );
        return;
      } else if ( _ntk.is_pi( n ) ) {
        _cut_network.add_unit_cut( index_node );
        return;
      } else {
        merge_cuts2( index_node );
        /// create gates
        typename Ntk::signal best_signal;
        if ( index_node < _ntk.size() ) {
          int32_t tmp_defered_size = derefed_size<Ntk, NodeCostFn>( _ntk, n );
          if ( tmp_defered_size == 1 )  // not have to opt
          {
            std::vector<typename Ntk::signal> children( _ntk.fanin_size( n ) );
            _ntk.foreach_fanin( n, [&]( auto const& s, auto i ) {
              children[i] = _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s];
            } );
            best_signal       = dest.clone_node( _ntk, n, children );
            map_old_to_new[n] = best_signal;
          } else {
            int32_t best_gain = -1;

            for ( auto& cut : _cut_network.cuts( _ntk.node_to_index( n ) ) ) {
              if ( cut->size() < _params.min_candidate_cut_size || cut->size() > _params.max_candidate_cut_size )
                continue;

              std::vector<typename Ntk::signal> leaves( cut->size() );
              std::unordered_set<node<Ntk>>     leaves_set;
              auto                              index = 0u;
              for ( auto l : *cut ) {
                leaves_set.insert( dest.get_node( leaves[index] ) );
                leaves[index++] = map_old_to_new[_ntk.index_to_node( l )];
              }

              // clear old depth info
              auto old_size = dest.size();
              /// resynthesis of the current cut for the isologue
              _resynthesis_fn.run(
                  dest, _cut_network.truth_table( *cut ), leaves.begin(), leaves.end(), [&]( auto const& s ) {
                    uint32_t tmp_defered_size2 = ref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( s ) );
                    deref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( s ) );
                    int32_t gain = tmp_defered_size - tmp_defered_size2;

                    if ( gain > best_gain && ( gain > 0 || ( _params.use_zero_gain && gain == 0 ) )
                         && recursive_compute_depth( dest, dest.get_node( s ), leaves_set ) <= dp_view.level( n ) ) {
                      best_gain   = gain;
                      best_signal = s;
                    }
                    return true;
                  } );
            }
            if ( best_gain == -1 ) {
              std::vector<typename Ntk::signal> children( _ntk.fanin_size( n ) );
              _ntk.foreach_fanin( n, [&]( auto const& s, auto i ) {
                children[i] = _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s];
              } );
              best_signal       = dest.clone_node( _ntk, n, children );
              map_old_to_new[n] = best_signal;
            } else {
              map_old_to_new[n] = best_signal;
            }
          }
          /// ref the current node
          ref_node_recursive<Ntk, NodeCostFn>( dest, dest.get_node( map_old_to_new[n] ) );
        }

        auto size = _ntk.size();
        // trade off of runtime and memory
        if ( ( size <= 10000 && n % ( size / 4 ) == 0 ) || ( size > 10000 && size <= 50000 && n % ( size / 5 ) == 0 )
             || ( size > 50000 && n % ( size / 6 ) == 0 ) ) {
          dest.clear_pos();
          // real po may not created yet, use the best above
          // FIXME, find real output instead of all nodes
          _ntk.foreach_gate( [&]( auto n ) {
            const auto s = map_old_to_new[n];
            dest.create_po( s );
          } );

          node_map<signal<Ntk>, Ntk> new_to_newer( dest );
          auto                       tmp = sweep( dest, new_to_newer );

          _ntk.foreach_node( [&]( auto i ) {
            uint64_t before   = map_old_to_new[i].data;
            map_old_to_new[i] = dest.is_complemented( map_old_to_new[i] ) ? tmp.create_not( new_to_newer[map_old_to_new[i]] )
                                                                          : new_to_newer[map_old_to_new[i]];
          } );
          dest = tmp;
        }

        /// TODO: add remove children's function
        ///  remove some bad cuts foreach_fanin
        _ntk.foreach_fanin( n, [&]( auto s, auto i ) {
          auto index_child = _ntk.node_to_index( _ntk.get_node( s ) );
          if ( --_fanouts[index_child] == 0 ) {
            _cut_network.cuts( index_child ).empty();
          }
        } );
      }
    } );
    dest.clear_pos();
    /// create POs
    _ntk.foreach_po( [&]( auto const& s ) {
      dest.create_po( _ntk.is_complemented( s ) ? dest.create_not( map_old_to_new[s] ) : map_old_to_new[s] );
    } );
    /// clean up the dangling nodes
    dest = cleanup_dangling<Ntk>( dest );

    const auto cost_res = compute_total_cost( dest );
    // printf("costs: %d -- %d\n", cost_original, cost_res);
    return cost_res > cost_original ? _ntk : dest;
  }

 private:
  /**
   * @brief initizlize the node's tmp_defered_size with fanout
   */
  void init_nodes()
  {
    _ntk.clear_values();
    _ntk.foreach_node( [&]( auto const& n ) { _ntk.set_value( n, _ntk.fanout_size( n ) ); } );
  }

  /**
   * @brief compute the total cost of the entire network
   *    we default using the unit_cost for the average node's size cost
   */
  uint32_t compute_total_cost( const Ntk& ntk )
  {
    uint32_t total_cost = 0u;
    ntk.foreach_gate( [&]( auto const& n ) { total_cost += _node_cost_fn( ntk, n ); } );
    return total_cost;
  }

  uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
  {
    std::vector<kitty::dynamic_truth_table> tt( vcuts.size() );
    auto                                    i = 0;
    for ( auto const& cut : vcuts ) {
      // tt[i] = kitty::extend_to( _cut_network._truth_tables[( *cut )->func_id], res.size() );
      // tt[i] = extend_truth_table_at( (*cut)->func_id, res);
      tt[i]           = kitty::extend_to( _cut_network.at( ( *cut )->func_id ), res.size() );
      const auto supp = _cut_network.compute_truth_table_support( *cut, res );
      kitty::expand_inplace( tt[i], supp );
      ++i;
    }

    auto tt_res = _ntk.compute( _ntk.index_to_node( index ), tt.begin(), tt.end() );

    if ( _cut_ps.minimize_truth_table ) {
      const auto support = kitty::min_base_inplace( tt_res );
      if ( support.size() != res.size() ) {
        auto                  tt_res_shrink = kitty::shrink_to( tt_res, static_cast<unsigned>( support.size() ) );
        std::vector<uint32_t> leaves_before( res.begin(), res.end() );
        std::vector<uint32_t> leaves_after( support.size() );

        auto it_support = support.begin();
        auto it_leaves  = leaves_after.begin();
        while ( it_support != support.end() ) {
          *it_leaves++ = leaves_before[*it_support++];
        }
        res.set_leaves( leaves_after.begin(), leaves_after.end() );
        // return _cut_network._truth_tables.insert( tt_res_shrink );
        return _cut_network.insert_truth_table( tt_res_shrink );
      }
    }

    // return _cut_network._truth_tables.insert( tt_res );
    return _cut_network.insert_truth_table( tt_res );
  }

  void merge_cuts2( uint32_t index )
  {
    array<cut_set_t*, Ntk::max_fanin_size + 1> lcuts;  // child's cut
    const auto                                 fanin = 2;
    uint32_t                                   pairs{ 1 };
    _ntk.foreach_fanin( _ntk.index_to_node( index ), [&]( auto child, auto i ) {
      lcuts[i] = &_cut_network.cuts( _ntk.node_to_index( _ntk.get_node( child ) ) );
      pairs *= static_cast<uint32_t>( lcuts[i]->size() );
    } );
    lcuts[2]    = &_cut_network.cuts( index );
    auto& rcuts = *lcuts[fanin];
    rcuts.clear();

    cut_t new_cut;

    std::vector<cut_t const*> vcuts( fanin );

    _cut_network.incre_total_tuples( pairs );

    for ( auto const& c1 : *lcuts[0] ) {
      for ( auto const& c2 : *lcuts[1] ) {
        if ( !c1->merge( *c2, new_cut, _cut_ps.cut_size ) ) {
          continue;
        }

        if ( rcuts.is_dominated( new_cut ) ) {
          continue;
        }
        // compute boolean function
        vcuts[0]         = c1;
        vcuts[1]         = c2;
        new_cut->func_id = compute_truth_table( index, vcuts, new_cut );

        if ( new_cut.size() == 0 )
          continue;
        rcuts.insert( new_cut );
      }
    }
    /* limit the maximum number of cuts */
    rcuts.limit( _cut_ps.cut_limit - 1 );

    _cut_network.incre_total_cuts( rcuts.size() );

    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 ) {
      _cut_network.add_unit_cut( index );
    }
  }

  uint32_t recursive_compute_depth( Ntk& dest, node<Ntk> root, std::unordered_set<node<Ntk>>& leaves_set )
  {
    if ( leaves_set.find( root ) != leaves_set.end() ) {
      return _depth_map.find( root )->second;
    }

    uint32_t max_child_dp = 0;
    dest.foreach_fanin( root, [&]( auto s, auto i ) {
      max_child_dp = std::max( max_child_dp, recursive_compute_depth( dest, dest.get_node( s ), leaves_set ) );
    } );

    // update depth info
    auto iter = _depth_map.find( root );
    if ( iter == _depth_map.end() ) {  // newly node
      _depth_map.insert( std::make_pair( root, 1 + max_child_dp ) );
    } else {
      iter->second = std::max( 1 + max_child_dp, iter->second );
    }

    return 1 + max_child_dp;
  }

 private:
  Ntk const&            _ntk;
  ResynthesisFn const&  _resynthesis_fn;
  NodeCostFn const&     _node_cost_fn;
  rewrite_params const& _params;

  network_cuts_t&        _cut_network;
  cut_enumeration_params _cut_ps;
  vector<int16_t>        _fanouts;

  std::map<node<Ntk>, uint32_t> _depth_map;
};  // end class rewrite_online_impl

};  // end namespace detail

template <typename Ntk           = iFPGA_NAMESPACE::aig_network,
          typename ResynthesisFn = node_resynthesis<Ntk>,
          typename NodeCostFn    = unit_cost<Ntk>>
Ntk rewrite( Ntk const& ntk, rewrite_params const& params )
{
  // printf("rewrite start!\n");
  ResynthesisFn resynthesis_fn;
  NodeCostFn    cost_fn;
  const auto    dest = detail::rewrite_impl<Ntk, ResynthesisFn, NodeCostFn>{ ntk, params, resynthesis_fn, cost_fn }.run();
  return dest;
}

template <typename Ntk           = iFPGA_NAMESPACE::aig_network,
          typename ResynthesisFn = node_resynthesis<Ntk>,
          typename NodeCostFn    = unit_cost<Ntk>>
Ntk rewrite_online( Ntk const& ntk, rewrite_params const& params )
{
  ResynthesisFn                                                                          resynthesis_fn;
  NodeCostFn                                                                             cost_fn;
  iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::cut_enumeration_rewrite_cut> cut_network( ntk.size() );
  const auto dest = detail::rewrite_online_impl<Ntk, ResynthesisFn, NodeCostFn>{ ntk,
                                                                                 cut_network,
                                                                                 params,
                                                                                 resynthesis_fn,
                                                                                 cost_fn,
                                                                                 params.cut_enumeration_ps }
                        .run();
  return dest;
}

iFPGA_NAMESPACE_HEADER_END
