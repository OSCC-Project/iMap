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
#include "cut.hpp"

#include <vector>
#include <array>
#include <iterator>
#include <algorithm>
#include <assert.h>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief some operation of a cut_set
 *    CutType, the cut type , Cut<MaxLeaves , data_type> , eg: Cut<10 , uing32_t>
 *    MaxCuts, control the max number of cuts a cut_set could hold
 */
template<typename CutType, int MaxCuts>
class cut_set
{
public:
  /**
   * @brief constructor
   */
  cut_set()
  {
    clear();
  }

  /**
   * @brief release data and pointer the _pcuts to _cuts
   */
  void clear()
  {
    _cuts.resize(MaxCuts);
    _length = 0u;
    _pcend = _pend = _pcuts.begin();
    auto it = _pcuts.begin();
    for(auto& c : _cuts)
      *it++ = &c;
  }

  /**
   * @brief pointer point to the new memory chunk
   */
  void repointer()
  {
    _pcend = _pend = _pcuts.begin();
    auto it = _pcuts.begin();
    for(auto& c : _cuts)
      *it++ = &c;
  }

  /**
   * @brief release memory
   */
  void empty()
  {
    _cuts.clear();
    _cuts.shrink_to_fit();
    _length = 0u;
    repointer();
  }

  /**
   * @brief release the not used tail-data memory
   */
  void resize_to_fit()
  {
    _cuts[0] = best();
    _cuts.resize(size());
    _cuts.shrink_to_fit();
    _length = size();
    repointer();
  }
  
  auto begin() const  { return _pcuts.begin(); }
  auto end()   const  { return _pcend; }
  auto begin()        { return _pcuts.begin(); }
  auto end()          { return _pend; }
  auto size()  const  { return _pcend - _pcuts.begin(); }

  /**
   * @brief return the index's cut itself 
   */
  auto const& operator[]( uint32_t index) const
  {
    return *_pcuts[index];
  }

  auto const& best() const
  {
    return *_pcuts[0];
  }

  auto& best()
  {
    return *_pcuts[0];
  }

  template<typename Iterator>
  CutType& add_cut(Iterator begin, Iterator end)
  {
    assert( _pcend != _pcuts.end() );
    auto& cut = **_pend++;
    cut.set_leaves(begin, end);
    ++_pcend;
    ++_length;
    return cut;
  }

  /**
   * @brief   compute wether there is a _cuts[i] <= cut 
   * @return  true : there is a _cuts[i] <= cut
   */
  bool is_dominated(CutType const& cut) const
  {
    return std::find_if( _pcuts.begin(), _pcend, [&cut]( auto const* other ) { return other->dominates( cut ); } ) != _pcend;
  }

  /**
   * @brief insert a cut in _cuts
   *        the dominates between cut and _cuts[i]  
   *        in a sorted way
   */
  void insert(CutType const& cut)
  {
    assert(cut.size());
    _pcend = _pend = std::stable_partition( _pcuts.begin(), _pend, [&cut]( auto const* other ) { return !cut.dominates( *other ); } );
    auto ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return *a < *b; } );

    /* too many cuts, we need to remove one */
    if ( _pend == _pcuts.end() )
    {
      if ( ipos == _pend )
      {
        return;
      }
      else
      {
        --_pend;
        --_pcend;
      }
    }

    auto& icut = *_pend;
    icut->set_leaves( cut.begin(), cut.end() );
    icut->data() = cut.data();

    if ( ipos != _pend )
    {
      auto it = _pend;
      while ( it > ipos )
      {
        std::swap( *it, *( it - 1 ) );
        --it;
      }
    }

    _pcend++;
    _pend++;
    ++_length;
  }

  /**
   * @brief update the best to the begin, and still remain the sorted order
   */
  void update_best( uint32_t index)
  {
    auto* best = _pcuts[index];
    for ( auto i = index; i > 0; --i )
    {
      _pcuts[i] = _pcuts[i - 1];
    }
    _pcuts[0] = best;
  }

  /**
   * @brief limit the cuts size in a fixed size
   */
  void limit( uint32_t size)
  {
    if ( std::distance( _pcuts.begin(), _pend ) > static_cast<long>( size ) )
    {
      _pcend = _pend = _pcuts.begin() + size - 1;
      _length = size;
    }
  }

  /**
   * @brief remove the cut_index from node[index] cuts
   */
  void remove( uint32_t index)
  {
    auto* bad = _pcuts[index];
    for (auto i = index; i < size()-1; ++i )
    {
      _pcuts[i] = _pcuts[i + 1];
    }
    _pcend--;
    _pend--;
    _length--;
  }

  /**
   *  @brief only retain the top-size elements
   */
  void resize(uint8_t size)
  {
    _pcend = _pend = _pcuts.begin() + size;
    _length = size;
  }

  friend std::ostream& operator<<( std::ostream& os, cut_set const& set )
  {
    for ( auto const& c : set )
    {
      os << *c << "\n";
    }
    return os;
  }

private:
  std::vector<CutType>          _cuts;
  std::array<CutType*, MaxCuts> _pcuts;
  uint8_t                       _length{0u};
  typename std::array<CutType*, MaxCuts>::const_iterator _pcend{_pcuts.begin()};
  typename std::array<CutType*, MaxCuts>::iterator _pend{_pcuts.begin()};
};  // end class cut_set

iFPGA_NAMESPACE_HEADER_END