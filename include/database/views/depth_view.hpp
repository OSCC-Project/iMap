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
#include <vector>

#include "utils/traits.hpp"
#include "utils/cost_functions.hpp"
#include "utils/node_map.hpp"
#include "immutable_view.hpp"

iFPGA_NAMESPACE_HEADER_START

struct depth_view_params
{
  /*! \brief Take complemented edges into account for depth computation. */
  bool count_complements{false};
};

/*! \brief Implements `depth` and `level` methods for networks.
 *
 * This view computes the level of each node and also the depth of
 * the network.  It implements the network interface methods
 * `level` and `depth`.  The levels are computed at construction
 * and can be recomputed by calling the `update_levels` method.
 *
 * It also automatically updates levels, and depth when creating nodes or
 * creating a PO on a depth_view, however, it does not update the information,
 * when modifying or deleting nodes, neither will the critical paths be
 * recalculated (due to efficiency reasons).  In order to recalculate levels,
 * depth, and critical paths, one can call `update_levels` instead.
 *
 * **Required network functions:**
 * - `size`
 * - `get_node`
 * - `visited`
 * - `set_visited`
 * - `foreach_fanin`
 * - `foreach_po`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow
      aig_network aig = ...;

      // create a depth view on the network
      depth_view aig_depth{aig};

      // print depth
      std::cout << "Depth: " << aig_depth.depth() << "\n";
   \endverbatim
 */
template<class Ntk, class NodeCostFn = unit_cost<Ntk>, bool has_depth_interface = has_depth_v<Ntk>&& has_level_v<Ntk>&& has_update_levels_v<Ntk>>
class depth_view
{
};

template<class Ntk, class NodeCostFn>
class depth_view<Ntk, NodeCostFn, true> : public Ntk
{
public:
  depth_view( Ntk const& ntk, depth_view_params const& ps = {} ) : Ntk( ntk )
  {
    (void)ps;
  }
};

template<class Ntk, class NodeCostFn>
class depth_view<Ntk, NodeCostFn, false> : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit depth_view( NodeCostFn const& cost_fn = {}, depth_view_params const& ps = {} )
      : Ntk(),
        _ps( ps ),
        _levels( *this ),
        _crit_path( *this ),
        _cost_fn( cost_fn )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    Ntk::events().on_add.push_back( [this]( auto const& n ) { on_add( n ); } );
  }

  /*! \brief Standard constructor.
   *
   * \param ntk Base network
   * \param count_complements Count inverters as 1
   */
  explicit depth_view( Ntk const& ntk, NodeCostFn const& cost_fn = {}, depth_view_params const& ps = {} )
      : Ntk( ntk ),
        _ps( ps ),
        _levels( ntk ),
        _crit_path( ntk ),
        _cost_fn( cost_fn )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_levels();

    Ntk::events().on_add.push_back( [this]( auto const& n ) { on_add( n ); } );
  }

  // We should add these or make sure that members are properly copied
  //depth_view( depth_view<Ntk> const& ) = delete;
  //depth_view<Ntk> operator=( depth_view<Ntk> const& ) = delete;

  uint32_t depth() const
  {
    return _depth;
  }

  uint32_t level( node const& n ) const
  {
    return _levels[n];
  }

  bool is_on_critical_path( node const& n ) const
  {
    return _crit_path[n];
  }

  void set_level( node const& n, uint32_t level )
  {
    _levels[n] = level;
  }

  void set_depth( uint32_t level )
  {
    _depth = level;
  }

  void update_levels()
  {
    _levels.reset( 0 );
    _crit_path.reset( false );

    this->incr_trav_id();
    compute_levels();
  }

  void resize_levels()
  {
    _levels.resize();
  }

  uint32_t create_po( signal const& f, std::string const& name = std::string() )
  {
    auto po = Ntk::create_po( f, name );
    _depth = std::max( _depth, _levels[f] );
    return po;
  }

private:
  uint32_t compute_levels( node const& n )
  {
    if ( this->visited( n ) == this->trav_id() )
    {
      return _levels[n];
    }
    this->set_visited( n, this->trav_id() );

    if ( this->is_constant( n ) || this->is_pi( n ) )
    {
      return _levels[n] = 0;
    }

    uint32_t level{0};
    this->foreach_fanin( n, [&]( auto const& f ) {
      auto clevel = compute_levels( this->get_node( f ) );
      if ( _ps.count_complements && this->is_complemented( f ) )
      {
        clevel++;
      }
      level = std::max( level, clevel );
    } );

    return _levels[n] = level + _cost_fn( *this, n );
  }

  void compute_levels()
  {
    _depth = 0;
    this->foreach_po( [&]( auto const& f ) {
      auto clevel = compute_levels( this->get_node( f ) );
      if ( _ps.count_complements && this->is_complemented( f ) )
      {
        clevel++;
      }
      _depth = std::max( _depth, clevel );
    } );

    // this->foreach_po( [&]( auto const& f ) {
    //   const auto n = this->get_node( f );
    //   if ( _levels[n] == _depth )
    //   {
    //     set_critical_path( n );
    //   }
    // } );
  }

  void set_critical_path( node const& n )
  {
    _crit_path[n] = true;
    if ( !this->is_constant( n ) && !this->is_pi( n ) )
    {
      const auto lvl = _levels[n];
      this->foreach_fanin( n, [&]( auto const& f ) {
        const auto cn = this->get_node( f );
        auto offset = _cost_fn( *this, n );
        if ( _ps.count_complements && this->is_complemented( f ) )
        {
          offset++;
        }
        if ( _levels[cn] + offset == lvl && !_crit_path[cn] )
        {
          set_critical_path( cn );
        }
      } );
    }
  }

  void on_add( node const& n )
  {
    _levels.resize();

    uint32_t level{0};
    this->foreach_fanin( n, [&]( auto const& f ) {
      auto clevel = _levels[f];
      if ( _ps.count_complements && this->is_complemented( f ) )
      {
        clevel++;
      }
      level = std::max( level, clevel );
    } );

    _levels[n] = level + _cost_fn( *this, n );
  }

  depth_view_params _ps;
  node_map<uint32_t, Ntk> _levels;
  node_map<uint32_t, Ntk> _crit_path;
  uint32_t _depth{};
  NodeCostFn _cost_fn;
};

template<class T>
depth_view( T const& )->depth_view<T>;

template<class T, class NodeCostFn = unit_cost<T>>
depth_view( T const&, NodeCostFn const&, depth_view_params const& )->depth_view<T, NodeCostFn>;

iFPGA_NAMESPACE_HEADER_END