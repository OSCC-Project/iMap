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
#include "ifpga_namespaces.hpp"

#include <stdint.h>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief every node cost 1u
 *  unit-load-module
 */
template<typename Ntk>
struct unit_cost
{
  uint32_t operator() (Ntk const& ntk, node<Ntk> const& node) const
  {
    return 1u;
  }
};

/**
 * @brief every node cost 0u
 *  zero-load-module
 */
template<typename Ntk>
struct zero_cost
{
  uint32_t operator() (Ntk const& ntk, node<Ntk> const& node) const
  {
    return 0u;
  }
};

iFPGA_NAMESPACE_HEADER_END