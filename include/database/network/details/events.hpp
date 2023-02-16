/**
 * @file aig_network.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The events of a network
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 */

#pragma once

#include "utils/common_properties.hpp"
#include <vector>
#include <functional>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the events records for altering the network
 */
template<typename Ntk>
struct network_events
{
  std::vector< std::function< void( node<Ntk> const& n) > > on_add;
  std::vector< std::function< void( node<Ntk> const& n, std::vector< signal<Ntk> > const& previous_children )> > on_modified;
  std::vector< std::function< void( node<Ntk> const& n ) > > on_delete;

};  // end struct network_events

iFPGA_NAMESPACE_HEADER_END