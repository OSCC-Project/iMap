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

#include <queue>

#include "utils/traits.hpp"

iFPGA_NAMESPACE_HEADER_START

template<class Ntk>
struct arrival_time_compare
{
  bool operator()( arrival_time_pair<Ntk> const& p1, arrival_time_pair<Ntk> const& p2 ) const
  {
    return p1.level > p2.level;
  }
};

template<class Ntk>
using arrival_time_queue = std::priority_queue<arrival_time_pair<Ntk>, std::vector<arrival_time_pair<Ntk>>, arrival_time_compare<Ntk>>;

iFPGA_NAMESPACE_HEADER_END