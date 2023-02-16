/**
 * @file lut_aig.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief the lut cell represented by aig
 * 
 * @version 0.1
 * @date 2021-06-18
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"

#include <cstring>
#include <array>

iFPGA_NAMESPACE_HEADER_START

#define MAX_LUT_SIZE 25

/**
 * @brief LUT CELL structure
 */
struct lut_cell
{ 
  std::string                         name{};
  uint8_t                             max_size = MAX_LUT_SIZE;
  std::array<float, MAX_LUT_SIZE+1>   areas{};
  std::array<float, MAX_LUT_SIZE+1>   delays{};
  // bfsnpn::interger_truth_table        tt{};
};

iFPGA_NAMESPACE_HEADER_END