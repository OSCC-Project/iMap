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
#include <optional>

#include "utils/traits.hpp"
#include "cleanup.hpp"

iFPGA_NAMESPACE_HEADER_START

/*! \brief Creates a combinational miter from two networks.
 *
 * This method combines two networks that have the same number of primary
 * inputs and the same number of primary outputs into a miter.  The miter
 * has the same number of inputs and one primary output.  This output is the
 * OR of XORs of all primary output pairs.  In other words, the miter outputs
 * 1 for all input assignments in which the two input networks differ.
 *
 * All networks may have different types.  The method returns an optional, which
 * is `nullopt`, whenever the two input networks don't match in their number of
 * primary inputs and primary outputs.
 */
template<class NtkDest, class NtkSource1, class NtkSource2>
std::optional<NtkDest> miter( NtkSource1 const& ntk1, NtkSource2 const& ntk2 )
{
  static_assert( is_network_type_v<NtkSource1>, "NtkSource1 is not a network type" );
  static_assert( is_network_type_v<NtkSource2>, "NtkSource2 is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_num_pis_v<NtkSource1>, "NtkSource1 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource1>, "NtkSource1 does not implement the num_pos method" );
  static_assert( has_num_pis_v<NtkSource2>, "NtkSource2 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource2>, "NtkSource2 does not implement the num_pos method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_xor_v<NtkDest>, "NtkDest does not implement the create_xor method" );
  static_assert( has_create_nary_or_v<NtkDest>, "NtkDest does not implement the create_nary_or method" );

  /* both networks must have same number of inputs and outputs */
  if ( ( ntk1.num_pis() != ntk2.num_pis() ) || ( ntk1.num_pos() != ntk2.num_pos() ) )
  {
    return std::nullopt;
  }

  /* create primary inputs */
  NtkDest dest;
  std::vector<signal<NtkDest>> pis;
  for ( auto i = 0u; i < ntk1.num_pis(); ++i )
  {
    pis.push_back( dest.create_pi() );
  }

  /* copy networks */
  const auto pos1 = cleanup_dangling( ntk1, dest, pis.begin(), pis.end() );
  const auto pos2 = cleanup_dangling( ntk2, dest, pis.begin(), pis.end() );

  /* create XOR of output pairs */
  std::vector<signal<NtkDest>> xor_outputs;
  std::transform( pos1.begin(), pos1.end(), pos2.begin(), std::back_inserter( xor_outputs ),
                  [&]( auto const& o1, auto const& o2 ) { return dest.create_xor( o1, o2 ); } );

  /* create big OR of XOR gates */
  dest.create_po( dest.create_nary_or( xor_outputs ) );

  return dest;
}

iFPGA_NAMESPACE_HEADER_END