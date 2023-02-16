/**
 * @file common_properties.hpp
 * @author liwei ni (nilw@pcl.ac.cn)  
 * @brief The manager of common properties
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

#include "ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief we think all the networks should have the following properties
 */
template<typename Ntk>
using signal = typename Ntk::signal;

template<typename Ntk>
using node = typename Ntk::node;

iFPGA_NAMESPACE_HEADER_END