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

#include "utils/traits.hpp"

#include "utils/ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

/*! \brief Deletes all methods that can change the network.
 *
 * This view deletes all methods that can change the network structure such as
 * `create_not`, `create_and`, or `create_node`.  This view is convenient to
 * use as a base class for other views that make some computations based on the
 * structure when being constructed.  Then, changes to the structure invalidate
 * these data.
 */
template<typename Ntk>
class immutable_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /*! \brief Default constructor.
   *
   * Constructs immutable view on another network.
   */
  immutable_view( Ntk const& ntk ) : Ntk( ntk )
  {
  }

  signal create_pi( std::string const& name = {} ) = delete;
  void create_po( signal const& s, std::string const& name = {} ) = delete;
  signal create_buf( signal const& f ) = delete;
  signal create_not( signal const& f ) = delete;
  signal create_and( signal const& f, signal const& g ) = delete;
  signal create_nand( signal const& f, signal const& g ) = delete;
  signal create_or( signal const& f, signal const& g ) = delete;
  signal create_nor( signal const& f, signal const& g ) = delete;
  signal create_lt( signal const& f, signal const& g ) = delete;
  signal create_le( signal const& f, signal const& g ) = delete;
  signal create_gt( signal const& f, signal const& g ) = delete;
  signal create_ge( signal const& f, signal const& g ) = delete;
  signal create_xor( signal const& f, signal const& g ) = delete;
  signal create_xnor( signal const& f, signal const& g ) = delete;
  signal create_maj( signal const& f, signal const& g, signal const& h ) = delete;
  signal create_ite( signal const& cond, signal const& f_then, signal const& f_else ) = delete;
  signal create_node( std::vector<signal> const& fanin, kitty::dynamic_truth_table const& function ) = delete;
  signal clone_node( immutable_view<Ntk> const& other, node const& source, std::vector<signal> const& fanin ) = delete;
  void substitute_node( node const& old_node, node const& new_node ) = delete;
};

iFPGA_NAMESPACE_HEADER_END