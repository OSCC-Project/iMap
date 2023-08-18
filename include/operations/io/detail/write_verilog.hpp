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

#include <array>
#include <fstream>
#include <iostream>
#include <string>

#include <lorina/verilog.hpp>
#include <fmt/format.h>

#include "utils/traits.hpp"
#include "utils/node_map.hpp"
#include "utils/util.hpp"
#include "views/topo_view.hpp"

#include "utils/ifpga_namespaces.hpp"
iFPGA_NAMESPACE_HEADER_START

using namespace std::string_literals;

namespace detail
{

template<class Ntk>
std::vector<std::pair<bool, std::string>>
format_fanin( Ntk const& ntk, node<Ntk> const& n, node_map<std::string, Ntk>& node_names )
{
  std::vector<std::pair<bool, std::string>> children;
  ntk.foreach_fanin( n, [&]( auto const& f ) {
      children.emplace_back( std::make_pair( ntk.is_complemented( f ), node_names[f] ) );
    });
  return children;
}

} // namespace detail

struct write_verilog_params
{
  std::string module_name = "top";
  std::vector<std::pair<std::string, uint32_t>> input_names;
  std::vector<std::pair<std::string, uint32_t>> output_names;
};

/*! \brief Writes network in structural Verilog format into output stream
 *
 *  reference: mockturtle/include/io/write_verilog.hpp
    with some modification on port names printing
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `is_and`
 * - `is_or`
 * - `is_xor`
 * - `is_xor3`
 * - `is_maj`
 * - `node_to_index`
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk>
void write_verilog( Ntk const& ntk, std::ostream& os, write_verilog_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_is_or_v<Ntk>, "Ntk does not implement the is_or method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_is_xor3_v<Ntk>, "Ntk does not implement the is_xor3 method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );

  assert( ntk.is_combinational() && "Network has to be combinational" );

  std::vector<std::string> xs, inputs;
  if ( ps.input_names.empty() )
  {
    for ( auto i = 0u; i < ntk.num_pis(); ++i )
      xs.emplace_back( fmt::format( "x{}", i ) );
    inputs = xs;
  }
  else
  {
    uint32_t ctr{0u};
    for ( auto const& [name, width] : ps.input_names )
    {
      inputs.emplace_back( name );
      ctr += width;
      if(width == 0) // scalar
      {
        ctr += 1;
        xs.emplace_back(name);
      }
      else
      { // vector
        for ( auto i = 0u; i < width; ++i )
        {
          xs.emplace_back( fmt::format( "{}[{}]", name, i ) );
        }
      }
    }
    if ( ctr != ntk.num_pis() )
    {
      std::cerr << "[e] input names do not partition all inputs\n";
    }
  }

  std::vector<std::string> ys, outputs;
  if ( ps.output_names.empty() )
  {
    for ( auto i = 0u; i < ntk.num_pos(); ++i )
      ys.emplace_back( fmt::format( "y{}", i ) );
    outputs = ys;
  }
  else
  {
    uint32_t ctr{0u};
    for ( auto const& [name, width] : ps.output_names )
    {
      outputs.emplace_back( name );
      ctr += width;
      if(width == 0) // scalar
      {
        ctr += 1;
        ys.emplace_back(name);
      }
      else
      { // vector
        for ( auto i = 0u; i < width; ++i )
        {
          ys.emplace_back( fmt::format( "{}[{}]", name, i ) );
        }
      }
    }
    if ( ctr != ntk.num_pos() )
    {
      std::cerr << "[e] output names do not partition all outputs\n";
    }
  }

  std::vector<std::string> ws;
  ntk.foreach_gate( [&]( auto const& n ) {
    ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
  } );

  lorina::verilog_writer writer( os );
  writer.on_module_begin( ps.module_name, inputs, outputs );
  if ( ps.input_names.empty() )
  {
    writer.on_input( xs );
  }
  else
  {
    for ( auto const& [name, width] : ps.input_names )
    {
      if(width == 0) 
      {
        writer.on_input(name);
      }
      else
      {
        writer.on_input( width, name );
      }
    }
  }
  if ( ps.output_names.empty() )
  {
    writer.on_output( ys );
  }
  else
  {
    for ( auto const& [name, width] : ps.output_names )
    {
      if(width == 0)
      {
        writer.on_output(name);
      }
      else
      {
        writer.on_output( width, name );
      }
    }
  }
  if ( !ws.empty() )
  {
    writer.on_wire( ws );
  }

  node_map<std::string, Ntk> node_names( ntk );
  node_names[ntk.get_constant( false )] = "1'b0";
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    node_names[ntk.get_constant( true )] = "1'b1";

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_names[n] = xs[i];
  } );

  topo_view ntk_topo{ntk};

  ntk_topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return true;

    /* assign a name */
    node_names[n] = fmt::format( "n{}", ntk.node_to_index( n ) );

    if ( ntk.is_and( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "&" );
    }
    else if ( ntk.is_or( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "|" );
    }
    else if ( ntk.is_xor( n ) || ntk.is_xor3( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "^" );
    }
    else if ( ntk.is_maj( n ) )
    {
      std::array<signal<Ntk>, 3> children;
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) { children[i] = f; } );

      if ( ntk.is_constant( ntk.get_node( children[0u] ) ) )
      {
        std::vector<std::pair<bool, std::string>> vs;
        vs.emplace_back( std::make_pair( ntk.is_complemented( children[1u] ), node_names[ntk.get_node( children[1u] )] ) );
        vs.emplace_back( std::make_pair( ntk.is_complemented( children[2u] ), node_names[ntk.get_node( children[2u] )] ) );

        if ( ntk.is_complemented( children[0u] ) )
        {
          // or
          writer.on_assign( node_names[n], {vs[0u], vs[1u]}, "|" );
        }
        else
        {
          // and
          writer.on_assign( node_names[n], {vs[0u], vs[1u]}, "&" );
        }
      }
      else
      {
        writer.on_assign_maj3( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ) );
      }
    }
    else
    {
      if constexpr ( has_is_nary_and_v<Ntk> )
      {
        if ( ntk.is_nary_and( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "&" );
          return true;
        }
      }
      if constexpr ( has_is_nary_or_v<Ntk> )
      {
        if ( ntk.is_nary_or( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "|" );
          return true;
        }
      }
      if constexpr ( has_is_nary_xor_v<Ntk> )
      {
        if ( ntk.is_nary_xor( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "^" );
          return true;
        }
      }
      writer.on_assign_unknown_gate( node_names[n] );
    }

    return true;
  } );

  ntk.foreach_po( [&]( auto const& f, auto i ) {
    writer.on_assign_po( ys[i], std::make_pair( ntk.is_complemented( f ), node_names[f] ) );
  } );

  writer.on_module_end();
}

/**
 * @brief
*/
template<class Ntk>
void write_verilog( Ntk const& ntk, std::string const& filename, write_verilog_params const& ports = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_verilog( ntk, os, ports );
  os.close();
}

iFPGA_NAMESPACE_HEADER_END