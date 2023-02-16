/**
 * @file map_qor.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief the QoR of mapping result
 * @version 0.1
 * @date 2021-06-29
 * @copyright Copyright (c) 2021
 */
#pragma once
#include <cstdlib>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief QoR storagement
 */
struct mapping_qor_storage
{
  float_t delay;
  float_t area;
};

iFPGA_NAMESPACE_HEADER_END