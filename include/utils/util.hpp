/**
 * @file utils.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief some useful functions
 * @version 0.1
 * @date 2021-07-12
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"

#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <string>
#include <algorithm>
#include <numeric>

iFPGA_NAMESPACE_HEADER_START

bool isFileExist(const std::string& file)
{
  struct stat buffer;
  return ( stat(file.c_str() , &buffer) == 0 );
}

bool ends_with(const std::string& str , const std::string& suffix)
{
  int len = suffix.length();
  int len_str = str.length();
  if(len_str < len)
    return false;
  else
  {
    for( ; len > 0 ; --len , --len_str)
    {
      if( suffix[len-1] != str[len_str-1] )
        return false;
    }
    return true;
  }
}

template<class Iterator, class MapFn, class JoinFn>
std::invoke_result_t<MapFn, typename Iterator::value_type> map_and_join( Iterator begin, Iterator end, MapFn&& map_fn, JoinFn&& join_fn )
{
  if constexpr ( std::is_same_v<std::decay_t<JoinFn>, std::string> )
  {
    return std::accumulate( begin + 1, end, map_fn( *begin ),
                            [&]( auto const& a, auto const& v ) {
                              return a + join_fn + map_fn( v );
                            } );
  }
  else
  {
    return std::accumulate( begin + 1, end, map_fn( *begin ),
                            [&]( auto const& a, auto const& v ) {
                              return join_fn( a, map_fn( v ) );
                            } );
  }
}

iFPGA_NAMESPACE_HEADER_END