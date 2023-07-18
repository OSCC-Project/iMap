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
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include "utils/traits.hpp"
#include "views/depth_view.hpp"

iFPGA_NAMESPACE_HEADER_START

template<class Ntk>
class default_dot_drawer
{
public:
  virtual ~default_dot_drawer()
  {
  }

public: /* callbacks */
  virtual std::string node_label( Ntk const& ntk, node<Ntk> const& n ) const
  {
    return std::to_string( ntk.node_to_index( n ) );
  }

  virtual std::string node_shape( Ntk const& ntk, node<Ntk> const& n ) const
  {
    if ( ntk.is_constant( n ) )
    {
      return "box";
    }
    else if ( ntk.is_pi( n ) )
    {
      return "triangle";
    }
    else
    {
      return "ellipse";
    }
  }

  virtual uint32_t node_level( Ntk const& ntk, node<Ntk> const& n ) const
  {
    if ( !_depth_ntk )
    {
      _depth_ntk = std::make_shared<depth_view<Ntk>>( ntk );
    }

    return _depth_ntk->level( n );
  }

  virtual std::string po_shape( Ntk const& ntk, uint32_t i ) const
  {
    (void)ntk;
    (void)i;
    return "invtriangle";
  }

  virtual std::string node_fillcolor( Ntk const& ntk, node<Ntk> const& n ) const
  {
    return ( ntk.is_constant( n ) || ntk.is_pi( n ) ) ? "snow2" : "white";
  }

  virtual std::string po_fillcolor( Ntk const& ntk, uint32_t i ) const
  {
    (void)ntk;
    (void)i;
    return "snow2";
  }

  virtual bool draw_signal( Ntk const& ntk, node<Ntk> const& n, signal<Ntk> const& f ) const
  {
    (void)ntk;
    (void)n;
    (void)f;
    return true;
  }

  virtual std::string signal_style( Ntk const& ntk, signal<Ntk> const& f ) const
  {
    return ntk.is_complemented( f ) ? "dashed" : "solid";
  }

private:
  mutable std::shared_ptr<depth_view<Ntk>> _depth_ntk;
};

template<class Ntk>
class gate_dot_drawer : public default_dot_drawer<Ntk>
{
public:
  virtual std::string node_label( Ntk const& ntk, node<Ntk> const& n ) const override
  {
    if constexpr ( has_is_and_v<Ntk> )
    {
      if ( ntk.is_and( n ) )
      {
        return "AND";
      }
    }

    if constexpr ( has_is_or_v<Ntk> )
    {
      if ( ntk.is_or( n ) )
      {
        return "OR";
      }
    }

    if constexpr ( has_is_xor_v<Ntk> )
    {
      if ( ntk.is_xor( n ) )
      {
        return "XOR";
      }
    }

    if constexpr ( has_is_maj_v<Ntk> )
    {
      if ( ntk.is_maj( n ) )
      {
        std::string label{"MAJ"};
        ntk.foreach_fanin( n, [&]( auto const& f ) {
          if ( ntk.is_constant( ntk.get_node( f ) ) )
          {
            const auto v = ntk.constant_value( ntk.get_node( f ) ) != ntk.is_complemented( f );
            label = v ? "OR" : "AND";
            return false;
          }
          return true;
        } );
        return label;
      }
    }

    if constexpr ( has_is_xor3_v<Ntk> )
    {
      if ( ntk.is_xor3( n ) )
      {
        return "XOR";
      }
    }

    if constexpr ( has_is_nary_and_v<Ntk> )
    {
      if ( ntk.is_nary_and( n ) )
      {
        return "AND";
      }
    }

    if constexpr ( has_is_nary_or_v<Ntk> )
    {
      if ( ntk.is_nary_or( n ) )
      {
        return "OR";
      }
    }

    if constexpr ( has_is_nary_xor_v<Ntk> )
    {
      if ( ntk.is_nary_xor( n ) )
      {
        return "XOR";
      }
    }

    return default_dot_drawer<Ntk>::node_label( ntk, n );
  }

  virtual std::string node_fillcolor( Ntk const& ntk, node<Ntk> const& n ) const override
  {
    if constexpr ( has_is_and_v<Ntk> )
    {
      if ( ntk.is_and( n ) )
      {
        return "lightcoral";
      }
    }

    if constexpr ( has_is_or_v<Ntk> )
    {
      if ( ntk.is_or( n ) )
      {
        return "palegreen2";
      }
    }

    if constexpr ( has_is_xor_v<Ntk> )
    {
      if ( ntk.is_xor( n ) )
      {
        return "lightskyblue";
      }
    }

    if constexpr ( has_is_maj_v<Ntk> )
    {
      if ( ntk.is_maj( n ) )
      {
        std::string color{"lightsalmon"};
        ntk.foreach_fanin( n, [&]( auto const& f ) {
          if ( ntk.is_constant( ntk.get_node( f ) ) )
          {
            const auto v = ntk.constant_value( ntk.get_node( f ) ) != ntk.is_complemented( f );
            color = v ? "palegreen2" : "lightcoral";
            return false;
          }
          return true;
        } );
        return color;
      }
    }

    if constexpr ( has_is_xor3_v<Ntk> )
    {
      if ( ntk.is_xor3( n ) )
      {
        return "lightskyblue";
      }
    }

    if constexpr ( has_is_nary_and_v<Ntk> )
    {
      if ( ntk.is_nary_and( n ) )
      {
        return "lightcoral";
      }
    }

    if constexpr ( has_is_nary_or_v<Ntk> )
    {
      if ( ntk.is_nary_or( n ) )
      {
        return "palegreen2";
      }
    }

    if constexpr ( has_is_nary_xor_v<Ntk> )
    {
      if ( ntk.is_nary_xor( n ) )
      {
        return "lightskyblue";
      }
    }

    return default_dot_drawer<Ntk>::node_fillcolor( ntk, n );
  }

  virtual bool draw_signal( Ntk const& ntk, node<Ntk> const& n, signal<Ntk> const& f ) const override
  {
    if constexpr ( has_is_maj_v<Ntk> )
    {
      if ( ntk.is_maj( n ) )
      {
        return !ntk.is_constant( ntk.get_node( f ) );
      }
    }

    return default_dot_drawer<Ntk>::draw_signal( ntk, n, f );
  }
};

/*! \brief Writes network in DOT format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required network functions:**
 * - is_constant
 * - is_pi
 * - foreach_node
 * - foreach_fanin
 * - foreach_po
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk, class Drawer = default_dot_drawer<Ntk>>
void write_dot( Ntk const& ntk, std::ostream& os, Drawer const& drawer = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );

  std::stringstream nodes, edges, levels;

  std::vector<std::vector<uint32_t>> level_to_node_indexes;

  ntk.foreach_node( [&]( auto const& n ) {
    nodes << fmt::format( "{} [label=\"{}\",shape={},style=filled,fillcolor={}]\n",
                          ntk.node_to_index( n ),
                          drawer.node_label( ntk, n ),
                          drawer.node_shape( ntk, n ),
                          drawer.node_fillcolor( ntk, n ) );
    if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
    {
      ntk.foreach_fanin( n, [&]( auto const& f ) {
        if ( !drawer.draw_signal( ntk, n, f ) )
          return true;
        edges << fmt::format( "{} -> {} [style={}]\n",
                              ntk.node_to_index( ntk.get_node( f ) ),
                              ntk.node_to_index( n ),
                              drawer.signal_style( ntk, f ) );
        return true;
      } );
    }

    const auto lvl = drawer.node_level( ntk, n );
    if ( level_to_node_indexes.size() <= lvl )
    {
      level_to_node_indexes.resize( lvl + 1 );
    }
    level_to_node_indexes[lvl].push_back( ntk.node_to_index( n ) );
  } );

  for ( auto const& indexes : level_to_node_indexes )
  {
    levels << "{rank = same; ";
    std::copy( indexes.begin(), indexes.end(), std::ostream_iterator<uint32_t>( levels, "; " ) );
    levels << "}\n";
  }

  levels << "{rank = same; ";
  ntk.foreach_po( [&]( auto const& f, auto i ) {
    nodes << fmt::format( "po{} [shape={},style=filled,fillcolor={}]\n", i, drawer.po_shape( ntk, i ), drawer.po_fillcolor( ntk, i ) );
    edges << fmt::format( "{} -> po{} [style={}]\n",
                          ntk.node_to_index( ntk.get_node( f ) ),
                          i,
                          drawer.signal_style( ntk, f ) );
    levels << fmt::format( "po{}; ", i );
  } );
  levels << "}\n";

  os << "digraph {\n"
     << "rankdir=BT;\n"
     << nodes.str() << edges.str() << levels.str() << "}\n";
}

/*! \brief Writes network in DOT format into a file
 *
 * **Required network functions:**
 * - is_constant
 * - is_pi
 * - foreach_node
 * - foreach_fanin
 * - foreach_po
 *
 * \param ntk Network
 * \param filename Filename
 */
template<class Ntk, class Drawer = default_dot_drawer<Ntk>>
void write_dot( Ntk const& ntk, std::string const& filename, Drawer const& drawer = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_dot( ntk, os, drawer );
  os.close();
}

iFPGA_NAMESPACE_HEADER_END