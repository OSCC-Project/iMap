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
#include "node.hpp"
#include <cstdlib>

iFPGA_NAMESPACE_HEADER_START

using node_data_type = node_data;

struct signal
{
public:
  signal() = default;
  signal(uint64_t index , uint64_t complement) : complement(complement), index(index)  {}
  explicit signal(uint64_t data) : data(data) {}
  signal(node_data_type const& p) : complement(p.weight), index(p.index) {}
  union 
  {
    struct
    {
      uint64_t complement : 1;
      uint64_t index      : 63;
    };
    uint64_t data;
  };
  signal operator!() const  { return signal(data^1); }
  signal operator+() const  { return signal(index, 0); }
  signal operator-() const  { return signal(index, 1); }
  signal operator^(bool complement) const  { return signal(data^(complement ? 1 : 0)); }
  bool operator==(signal const& other) const { return data == other.data; }
  bool operator!=(signal const& other) const { return data != other.data; }
  bool operator<(signal const& other) const { return data < other.data; }
  bool operator==(node_data_type const& other) const { return data == other.data; }
  operator node_data_type() const   { return node_data_type(index, complement); }

};  // end struct signal

iFPGA_NAMESPACE_HEADER_END