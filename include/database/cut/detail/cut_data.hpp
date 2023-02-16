/**
 * @file cut_data.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief 
 * @version 0.1
 * @date 2022-03-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
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