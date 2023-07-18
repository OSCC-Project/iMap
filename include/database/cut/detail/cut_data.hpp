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
#include "utils/ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief empty cut data cost zero memory
 * 
 */
struct empty_cut_data 
{
};

/**
 * @brief a general cut data for cut-enumeration to calculate 
 */
struct general_cut_data
{
  float     delay{0.0f};
  float     area{0.0f};                 
  float     edge{0.0f};
  float     power{0.0f};
  uint32_t  useless{0};
};

/**
 * @brief the cut data for wiremapping
 */
struct cut_enumeration_wiremap_cut
{
  float    delay{0.0f};
  float    area{0.0f};     // postpone to calc maybe more exact
  float    area_flow{0.0f};
  float    edge_flow{0.0f};     // postpone to calc maybe more exact
  float    edge{0.0f};
};

iFPGA_NAMESPACE_HEADER_END