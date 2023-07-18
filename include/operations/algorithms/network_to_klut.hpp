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
#include "utils/node_map.hpp"
#include "views/topo_view.hpp"

#include <optional>
#include <unordered_map>

iFPGA_NAMESPACE_HEADER_START

namespace detail
{
template<class NtkDest, class NtkSource>
class collapse_mapped_network_impl
{
  public:
  collapse_mapped_network_impl( NtkSource const& ntk )
      : ntk( ntk )
  {
  }

  void run( NtkDest& dest )
  {
    node_map<signal<NtkDest>, NtkSource> node_to_signal( ntk );

    /* special map for output drivers to perform some optimizations */
    enum class driver_type
    {
      none,
      pos,
      neg,
      mixed
    };
    node_map<driver_type, NtkSource> node_driver_type( ntk, driver_type::none );

    /* opposites are filled for nodes with mixed driver types, since they have
       two nodes in the network. */
    std::unordered_map<node<NtkSource>, signal<NtkDest>> opposites;

    /* initial driver types */
    ntk.foreach_po( [&]( auto const& f ) {
      switch ( node_driver_type[f] )
      {
      case driver_type::none:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::pos;
        break;
      case driver_type::pos:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::mixed : driver_type::pos;
        break;
      case driver_type::neg:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::mixed;
        break;
      case driver_type::mixed:
      default:
        break;
      }
    } );

    /* it could be that internal nodes also point to an output driver node */
    ntk.foreach_node( [&]( auto const n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) || !ntk.is_cell_root( n ) )
        return;

      ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
        if ( node_driver_type[fanin] == driver_type::neg )
        {
          node_driver_type[fanin] = driver_type::mixed;
        }
      } );
    } );

    /* constants */
    auto add_constant_to_map = [&]( bool value ) {
      const auto n = ntk.get_node( ntk.get_constant( value ) );
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.get_constant( value );
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.get_constant( !value );
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.get_constant( value );
        opposites[n] = dest.get_constant( !value );
        break;
      }
    };

    add_constant_to_map( false );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      add_constant_to_map( true );
    }

    /* primary inputs */
    ntk.foreach_pi( [&]( auto n ) {
      signal<NtkDest> dest_signal;
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        break;

      case driver_type::neg:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest.create_not( dest_signal );
        break;

      case driver_type::mixed:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        opposites[n] = dest.create_not( node_to_signal[n] );
        break;
      }

      if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
      {
        if ( ntk.has_name( ntk.make_signal( n ) ) )
          dest.set_name( dest_signal, ntk.get_name( ntk.make_signal( n ) ) );
      }
    } );

    /* nodes */
    topo_view topo{ntk};
    topo.foreach_node( [&]( auto n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) || !ntk.is_cell_root( n ) )
        return;

      std::vector<signal<NtkDest>> children;
      ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
        children.push_back( node_to_signal[fanin] );
      } );

      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.create_node( children, ntk.cell_function( n ) );
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.create_node( children, ~ntk.cell_function( n ) );
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.create_node( children, ntk.cell_function( n ) );
        opposites[n] = dest.create_node( children, ~ntk.cell_function( n ) );
        break;
      }
    } );

    /* outputs */
    ntk.foreach_po( [&]( auto const& f, auto index ) {
      (void)index;

      if ( ntk.is_complemented( f ) && node_driver_type[f] == driver_type::mixed )
      {
        dest.create_po( opposites[ntk.get_node( f )] );
      }
      else
      {
        dest.create_po( node_to_signal[f] );
      }

      if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
      {
        if ( ntk.has_output_name( index ) )
        {
          dest.set_output_name( index, ntk.get_output_name( index ) );
        }
      }
      });
  }

  private:
    NtkSource const& ntk;
};  // end class collapse_mapped_network_impl

template<class NtkDest, class NtkSource>
class collapse_mapped_choice_impl
{
  public:
  using node_t = typename NtkSource::node;
  collapse_mapped_choice_impl( NtkSource const& ntk )
      : ntk( ntk )
  {
  }

  void run( NtkDest& dest )
  {
    node_map<signal<NtkDest>, NtkSource> node_to_signal( ntk );

    /* special map for output drivers to perform some optimizations */
    enum class driver_type
    {
      none,
      pos,
      neg,
      mixed
    };
    node_map<driver_type, NtkSource> node_driver_type( ntk, driver_type::none );

    /* opposites are filled for nodes with mixed driver types, since they have
       two nodes in the network. */
    std::unordered_map<node<NtkSource>, signal<NtkDest>> opposites;

    /* initial driver types */
    ntk.foreach_po( [&]( auto const& f ) {
      switch ( node_driver_type[f] )
      {
      case driver_type::none:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::pos;
        break;
      case driver_type::pos:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::mixed : driver_type::pos;
        break;
      case driver_type::neg:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::mixed;
        break;
      case driver_type::mixed:
      default:
        break;
      }
    } );

    /* it could be that internal nodes also point to an output driver node */
    ntk.foreach_node( [&]( auto const n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) || !ntk.is_cell_root( n ) )
        return;

      ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
        if ( node_driver_type[fanin] == driver_type::neg )
        {
          node_driver_type[fanin] = driver_type::mixed;
        }
      } );
    } );

    /* constants */
    auto add_constant_to_map = [&]( bool value ) {
      const auto n = ntk.get_node( ntk.get_constant( value ) );
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.get_constant( value );
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.get_constant( !value );
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.get_constant( value );
        opposites[n] = dest.get_constant( !value );
        break;
      }
    };

    add_constant_to_map( false );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      add_constant_to_map( true );
    }

    /* primary inputs */
    ntk.foreach_pi( [&]( auto n ) {
      signal<NtkDest> dest_signal;
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        break;

      case driver_type::neg:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest.create_not( dest_signal );
        break;

      case driver_type::mixed:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        opposites[n] = dest.create_not( node_to_signal[n] );
        break;
      }

      if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
      {
        if ( ntk.has_name( ntk.make_signal( n ) ) )
          dest.set_name( dest_signal, ntk.get_name( ntk.make_signal( n ) ) );
      }
    } );

    /* nodes */

    ntk.foreach_gate([&](auto const &n){
      if (ntk.is_constant(n) || ntk.is_pi(n) || !ntk.is_cell_root(n))
        return;

      std::vector<signal<NtkDest>> children;
      ntk.foreach_cell_fanin(n, [&](auto fanin){ 
        children.push_back(node_to_signal[fanin]); 
      });

      switch (node_driver_type[n])
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.create_node(children, ntk.cell_function(n));
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.create_node(children, ~ntk.cell_function(n));
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.create_node(children, ntk.cell_function(n));
        opposites[n] = dest.create_node(children, ~ntk.cell_function(n));
        break;
      } 
    });

    /* outputs */
    ntk.foreach_po( [&]( auto const& f, auto index ) {
      (void)index;

      if ( ntk.is_complemented( f ) && node_driver_type[f] == driver_type::mixed )
      {
        dest.create_po( opposites[ntk.get_node( f )] );
      }
      else
      {
        dest.create_po( node_to_signal[f] );
      }

      if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
      {
        if ( ntk.has_output_name( index ) )
        {
          dest.set_output_name( index, ntk.get_output_name( index ) );
        }
      }
      });
  }

  private:
    NtkSource const& ntk;
};  // end class collapse_mapped_choice_impl

};  // end namespace detail

template<class NtkDest, class NtkSource>
std::optional<NtkDest> network_to_klut( NtkSource const& ntk )
{
  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return std::nullopt;
  }
  else
  {
    detail::collapse_mapped_network_impl<NtkDest, NtkSource> p( ntk );
    NtkDest dest;
    p.run( dest );
    return dest;
  }
}

template<class NtkDest, class NtkSource>
bool network_to_klut( NtkDest& dest, NtkSource const& ntk )
{
  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return false;
  }
  else
  {
    detail::collapse_mapped_network_impl<NtkDest, NtkSource> p( ntk );
    p.run( dest );
    return true;
  }
}


template<class NtkDest, class NtkSource>
std::optional<NtkDest> choice_to_klut( NtkSource const& ntk )
{
  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return std::nullopt;
  }
  else
  {
    detail::collapse_mapped_choice_impl<NtkDest, NtkSource> p( ntk );
    NtkDest dest;
    p.run( dest );
    return dest;
  }
}

template<class NtkDest, class NtkSource>
bool choice_to_klut( NtkDest& dest, NtkSource const& ntk )
{
  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return false;
  }
  else
  {
    detail::collapse_mapped_choice_impl<NtkDest, NtkSource> p( ntk );
    p.run( dest );
    return true;
  }
}
iFPGA_NAMESPACE_HEADER_END