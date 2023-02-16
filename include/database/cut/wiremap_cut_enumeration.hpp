/**
 * @file wiremap_cut_enumeration.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The wiremap cut enumeration algorithm
 * 
 * @version 0.1
 * @date 2021-06-06
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"
#include "cut_enumeration.hpp"
#include <limits>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the compare function
 *    delay-oriented compare function
 */
template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_wiremap_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_wiremap_cut> const& c2 )
{
 constexpr auto eps{0.005f};
  
  if(gf_get_etc() == ETC_DELAY)
  {
    if ( c1->data.delay < c2->data.delay )
      return true;
    if ( c1->data.delay > c2->data.delay )
      return false;
    if(c1.size() < c2.size())
      return true;
    if(c1.size() > c2.size())
      return false;
    else 
      return c1->data.area_flow < c2->data.area_flow;
  }
  else if(gf_get_etc() == ETC_DELAY2)
  {
    if ( c1->data.delay < c2->data.delay )
      return true;
    if ( c1->data.delay > c2->data.delay )
      return false;
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    return c1.size() < c2.size();
  }
  else if(gf_get_etc() == ETC_FLOW)
  {
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    //TODO: here should be fainrefs
    return c1->data.delay < c2->data.delay;
  }
  else if(gf_get_etc() == ETC_AREA)
  {
    if ( c1->data.area < c2->data.area - eps )
      return true;
    if ( c1->data.area > c2->data.area + eps )
      return false;
    //TODO: here should be fainrefs
    return c1->data.delay < c2->data.delay;
  }
  else  // default
  {
    return c1.size() < c2.size();
  }
}

template<>
struct cut_enumeration_update_cut< cut_enumeration_wiremap_cut >
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    //TODO: update the value computation methods
    float_t delay{0};
    cut->data.area = cut.size();
    float_t area_flow =  cut->data.area < 2 ? 0.0f : 1.0f;
    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts.cuts( leaf )[0];
      delay = std::max( delay, best_leaf_cut->data.delay );
      area_flow += best_leaf_cut->data.area_flow;
    }
    cut->data.delay = 1 + delay;
    cut->data.area_flow = area_flow / ntk.fanout_size( n );
  }
};

template<int MaxLeaves>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_wiremap_cut> > const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.area_flow;
  return os;
}

iFPGA_NAMESPACE_HEADER_END