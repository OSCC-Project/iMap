/**
 * @file storage.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The storage of a network
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 */

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