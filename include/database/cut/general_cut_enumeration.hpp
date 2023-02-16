/**
 * @file general_cut_enumeration.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The gengeral template algorithm to generate cuts
 * 
 * @version 0.1
 * @date 2021-06-06
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"
#include "cut_enumeration.hpp"

iFPGA_NAMESPACE_HEADER_START
static const float_t   gv_eps = 0.005f;       // abc-epsilon

template< bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, empty_cut_data> const& c1, cut_type<ComputeTruth, empty_cut_data> const& c2 )
{
  return c1.size() < c2.size();
}

// template< bool ComputeTruth, ETypeCmp etc, class cut_enum_data = general_cut_data>
template< bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, general_cut_data> const& c1, cut_type<ComputeTruth, general_cut_data> const& c2 )
{  
  
  if(gf_get_etc() == ETC_DELAY)
  {
    if ( c1->data.delay < c2->data.delay - gv_eps )
      return true;
    if ( c1->data.delay > c2->data.delay + gv_eps )
      return false;
    if(c1.size() < c2.size())
      return true;
    if(c1.size() > c2.size())
      return false;
    if ( c1->data.area < c2->data.area - gv_eps )
      return true;
    if ( c1->data.area > c2->data.area + gv_eps )
      return false;
    if ( c1->data.edge < c2->data.edge - gv_eps )
      return true;
    if ( c1->data.edge > c2->data.edge + gv_eps )
      return false;
    if ( c1->data.power < c2->data.power - gv_eps )
      return true;
    if ( c1->data.power > c2->data.power + gv_eps )
      return false;
    return ( c1->data.useless < c2->data.useless );
  }
  else if(gf_get_etc() == ETC_DELAY2)
  {
    // {/// could achieve QoR: area 93+%, delay 94+%
    //   if ( c1->data.delay < c2->data.delay )
    //     return true;
    //   if ( c1->data.delay > c2->data.delay )
    //     return false;
    //   if ( c1->data.area < c2->data.area - gv_eps )
    //     return true;
    //   if ( c1->data.area > c2->data.area + gv_eps )
    //     return false;
    //   return c1.size() < c2.size();
    // }

    {
      if ( c1->data.delay < c2->data.delay - gv_eps )
        return true;
      if ( c1->data.delay > c2->data.delay + gv_eps )
        return false;
      if ( c1->data.useless < c2->data.useless )
        return true;
      if ( c1->data.useless > c2->data.useless )
        return false;
      if ( c1->data.area < c2->data.area - gv_eps )
        return true;
      if ( c1->data.area > c2->data.area + gv_eps )
        return false;
      if ( c1->data.edge < c2->data.edge - gv_eps )
        return true;
      if ( c1->data.edge > c2->data.edge + gv_eps )
        return false;
      if ( c1->data.power < c2->data.power - gv_eps )
        return true;
      if ( c1->data.power > c2->data.power + gv_eps )
        return false;
      return (c1.size() < c2.size());      
    }
  }
  else if(gf_get_etc() == ETC_FLOW)
  {
    if ( c1->data.delay < c2->data.delay - gv_eps )
      return true;
    if ( c1->data.delay > c2->data.delay + gv_eps )
      return false;
    if ( c1->data.area < c2->data.area - gv_eps )
      return true;
    if ( c1->data.area > c2->data.area + gv_eps )
      return false;
    if ( c1->data.power < c2->data.power - gv_eps )
      return true;
    if ( c1->data.power > c2->data.power + gv_eps )
      return false;
    if ( c1->data.edge < c2->data.edge - gv_eps )
      return true;
    if ( c1->data.edge > c2->data.edge + gv_eps )
      return false;
    return c1.size() < c2.size();
  }
  else if(gf_get_etc() == ETC_AREA)
  {
    if ( c1->data.delay < c2->data.delay - gv_eps )
      return true;
    if ( c1->data.delay > c2->data.delay + gv_eps )
      return false;
    if ( c1->data.power < c2->data.power - gv_eps )
      return true;
    if ( c1->data.power > c2->data.power + gv_eps )
      return false;
    if ( c1->data.edge < c2->data.edge - gv_eps )
      return true;
    if ( c1->data.edge > c2->data.edge + gv_eps )
      return false;
    if ( c1->data.area < c2->data.area - gv_eps )
      return true;
    if ( c1->data.area > c2->data.area + gv_eps )
      return false;
    return (c1.size() < c2.size());
  }
  else  // default
  {
    return c1.size() < c2.size();
  }
}

template<>
struct cut_enumeration_update_cut<general_cut_data>
{
  /// apply to update the cut data
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply(Cut& cut, NetworkCuts& cuts, Ntk const& ntk, node<Ntk> const& node )
  {
    float delay = 0.0f;
    float area = 1.0f, addon_area;
    float edge = cut.size(), addon_edge;
    if(gf_get_etc() ==  ETC_FLOW)
    {
      for(auto& leaf : cut)
      {
        const auto& best_leaf_cut = cuts.cuts( leaf ).best();
        delay = std::max(delay, best_leaf_cut->data.delay);
        if( ntk.fanout_size(leaf) == 0 || ntk.is_constant(leaf) )
        {
          addon_area = best_leaf_cut->data.area;
          addon_edge = best_leaf_cut->data.edge;
        }
        else
        {
          addon_area = best_leaf_cut->data.area / ntk.fanout_size(leaf);
          addon_edge = best_leaf_cut->data.edge / ntk.fanout_size(leaf);
        }

        if(area >= (float)1e32 || addon_area >= (float)1e32)
        {
          area = (float)1e32;
        }
        else
        {
          area += addon_area;
          if(area >= (float)1e32)
            area = (float)1e32;
        }

        if(edge >= (float)1e32 || addon_edge >= (float)1e32)
        {
          edge = (float)1e32;
        }
        else
        {
          edge += addon_edge;
          if(edge >= (float)1e32)
            edge = (float)1e32;
        }
      }
      cut->data.area  = area;
      cut->data.edge  = edge;
      cut->data.delay = delay+1.0f;
    }
    else
    {
      cut->data.area = cuts.cut_area_derefed(cut, ntk, node);
      cut->data.edge = cuts.cut_edge_derefed(cut, ntk, node);
       for(auto leaf : cut)
      {
        const auto& best_leaf_cut = cuts.cuts( leaf ).best();
        delay = std::max(delay, best_leaf_cut->data.delay);
      }
      cut->data.delay = delay+1.0f;
    }
  }
};

template<int MaxLeaves, class cut_enum_data = general_cut_data>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enum_data>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.area << " E = " << c->data.edge ;
  return os;
}

template<int MaxLeaves, class cut_enum_data = general_cut_data>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<true, cut_enum_data>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.area << " E = " << c->data.edge ;
  return os;
}

iFPGA_NAMESPACE_HEADER_END