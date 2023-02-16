/**
 * @file mf_cut_enumeration.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The mf cut enumeration algorithm
 * 
 * @version 0.1
 * @date 2021-06-06
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"
#include "cut_enumeration.hpp"

iFPGA_NAMESPACE_HEADER_START

struct cut_enumeration_mf_cut
{
  uint32_t delay{0};
  float flow{0};
  float cost{0};
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_mf_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_mf_cut> const& c2 )
{
  constexpr auto eps{0.005f};
  if ( c1->data.flow < c2->data.flow - eps )
    return true;
  if ( c1->data.flow > c2->data.flow + eps )
    return false;
  if ( c1->data.delay < c2->data.delay )
    return true;
  if ( c1->data.delay > c2->data.delay )
    return false;
  return c1.size() < c2.size();
}

template<>
struct cut_enumeration_update_cut< cut_enumeration_mf_cut >
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    uint32_t delay{0};
    float flow = cut->data.cost = cut.size() < 2 ? 0.0f : 1.0f;

    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts.cuts( leaf )[0];
      delay = std::max( delay, best_leaf_cut->data.delay );
      flow += best_leaf_cut->data.flow;
    }

    cut->data.delay = 1 + delay;
    cut->data.flow = flow / ntk.fanout_size( n );
  }
};

template<int MaxLeaves>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_mf_cut>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.flow;
  return os;
}

iFPGA_NAMESPACE_HEADER_END