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

#include "utils/common_properties.hpp"
#include <vector>
#include <functional>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the events records for altering the network
 */
template<typename Ntk>
struct network_events
{
  std::vector< std::function< void( node<Ntk> const& n) > > on_add;
  std::vector< std::function< void( node<Ntk> const& n, std::vector< signal<Ntk> > const& previous_children )> > on_modified;
  std::vector< std::function< void( node<Ntk> const& n ) > > on_delete;

};  // end struct network_events

iFPGA_NAMESPACE_HEADER_END