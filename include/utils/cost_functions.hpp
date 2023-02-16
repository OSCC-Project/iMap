/**
 * @file cost_function.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief the cost function of a node
 * 
 * @version 0.1
 * @date 2021-07-26
 * @copyright Copyright (c) 2021
 */

/**
 * @file cost_function.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief cost functions for node in network
 * @version 0.1
 * @date 2021-07-12
 * @copyright Copyright (c) 2021
 */
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