// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Shanghai Anlogic Infotech Co.,Ltd.
// Copyright (c) 2023-2025 Peking University
//
// iMAP-FPGA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************

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