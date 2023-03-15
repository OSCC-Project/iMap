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
    if ( c1->data.useless < c2->data.useless )
      return true;
    if ( c1->data.useless > c2->data.useless )
      return false;
    return false;
  }
  else if(gf_get_etc() == ETC_DELAY2)
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
  else if(gf_get_etc() == ETC_AREA)
  {
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
    if ( c1->data.delay < c2->data.delay - gv_eps )
      return true;
    if ( c1->data.delay > c2->data.delay + gv_eps )
      return false;
    if(c1.size() < c2.size())
      return true;
    if(c1.size() > c2.size())
      return false;
    if ( c1->data.useless < c2->data.useless )
      return true;
    if ( c1->data.useless > c2->data.useless )
      return false;
    return false;
  }
  else  // default
  {
    if ( c1->data.delay < c2->data.delay - gv_eps)
      return true;
    if ( c1->data.delay > c2->data.delay + gv_eps)
      return false;
    else
      return c1.size() < c2.size();
  }
}


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