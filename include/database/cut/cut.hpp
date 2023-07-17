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
#include <array>
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

#include "utils/ifpga_namespaces.hpp"
#include "detail/cut_data.hpp"

iFPGA_NAMESPACE_HEADER_START

// state a global variables
/**
 * @brief the Comparation type
 */
enum ETypeCmp {
  ETC_DEFAULT = 0,
  ETC_DELAY,
  ETC_DELAY2,
  ETC_AREA,         // area or area flow
};

/**
 * @brief the Mode type for cut value computation
 */
enum ETypeMode {
  ETM_DEFAULT = 0,
  ETM_DELAY,
  ETM_AREA,
  ETM_FLOW,         // area or area flow
};

/**
 * @brief global variable and functions
 *        
 *    numeration type of gv_etc for marking the step of mapping rounds,      
 *    ifferent mapping rounds may based on different cut_enumeration trics
 */
ETypeCmp gv_etc{ETC_DELAY};
ETypeCmp gf_get_etc() { return gv_etc; }
void gf_set_etc(ETypeCmp const& etc)  { gv_etc = etc; } 

ETypeMode gv_etm{ETM_DELAY};
ETypeMode gf_get_etm() { return gv_etm; }
void gf_set_etm(ETypeMode const& etm)  { gv_etm = etm; } 


/**
 * @brief the basic cut class for cut_enumeration 
 *  
 *    MaxLeaves, control the max number of leaves a cut can hold
 *    max_cut_size, control the max number of gates a cut can hold
 *    max_fain_size_of_a_gate , control the max number fains a gates can hold 
 *    MaxLeaves ~ max_cut_size * max_fain_size_of_a_gate
 * 
 *    T is the cut_data type ,could be a struct 、 int 、or other type, the default empty_cut_data cost zero memory
 */
template<int MaxLeaves, typename T = empty_cut_data>
class cut
{
public:
  /*! \brief Default constructor.
   */
  cut() = default;

  /*! \brief Copy constructor.
   *
   * Copies leaves, length, signature, and data.
   *
   * \param other Other cut
   */
  cut( cut const& other )
  {
    _cend = _end = std::copy( other.begin(), other.end(), _leaves.begin() );
    _length = other._length;
    _signature = other._signature;
    _data = other._data;
  }

  /*! \brief Assignment operator.
   *
   * Copies leaves, length, signature, and data.
   *
   * \param other Other cut
   */
  cut& operator=( cut const& other );

  /*! \brief Sets leaves (using iterators).
   *
   * \param begin Begin iterator to leaves
   * \param end End iterator to leaves (exclusive)
   */
  template<typename Iterator>
  void set_leaves( Iterator begin, Iterator end );

  /*! \brief Sets leaves (using container).
   *
   * Convenience function, which extracts the begin and end iterators from the
   * container.
   */
  template<typename Container>
  void set_leaves( Container const& c );

  /*! \brief Signature of the cut. */
  auto signature() const { return _signature; }

  /*! \brief Returns the size of the cut (number of leaves). */
  auto size() const { return _length; }

  /*! \brief Begin iterator (constant). */
  auto begin() const { return _leaves.begin(); }

  /*! \brief End iterator (constant). */
  auto end() const { return _cend; }

  /*! \brief Begin iterator (mutable). */
  auto begin() { return _leaves.begin(); }

  /*! \brief End iterator (mutable). */
  auto end() { return _end; }

  /*! \brief Access to data (mutable). */
  T* operator->() { return &_data; }

  /*! \brief Access to data (constant). */
  T const* operator->() const { return &_data; }

  /*! \brief Access to data (mutable). */
  T& data() { return _data; }

  /*! \brief Access to data (constant). */
  T const& data() const { return _data; }

  /*! \brief Checks whether the cut is a subset of another cut.
   *
   * If \f$L_1\f$ are the leaves of the current cut and \f$L_2\f$ are the leaves
   * of `that`, then this method returns true if and only if
   * \f$L_1 \subseteq L_2\f$.
   *
   * \param that Other cut
   */
  bool dominates( cut const& that ) const;

  /*! \brief Merges two cuts.
   *
   * This method merges two cuts and stores the result in `res`.  The merge of
   * two cuts is the union \f$L_1 \cup L_2\f$ of the two leaf sets \f$L_1\f$ of
   * the cut and \f$L_2\f$ of `that`.  The merge is only successful if the
   * union has not more than `cut_size` elements.  In that case, the function
   * returns `false`, otherwise `true`.
   *
   * \param that Other cut
   * \param res Resulting cut
   * \param cut_size Maximum cut size
   * \return True, if resulting cut is small enough
   */
  bool merge( cut const& that, cut& res, uint32_t cut_size ) const; 

private:
  std::array<uint32_t, MaxLeaves> _leaves;
  uint32_t                        _length{0};
  uint64_t                        _signature{0};
  typename std::array<uint32_t, MaxLeaves>::const_iterator _cend;
  typename std::array<uint32_t, MaxLeaves>::iterator       _end;
  T                               _data;
};

/*! \brief Compare two cuts.
 *
 * Default comparison function for two cuts.  A cut is smaller than another
 * cut, if it has fewer leaves.
 *
 * This function should be specialized for custom cuts, if additional data
 * changes the cost of a cut.
 */
template<int MaxLeaves, typename T>
bool operator<( cut<MaxLeaves, T> const& c1, cut<MaxLeaves, T> const& c2 )
{
  // printf("7O7 \t");
  return c1.size() < c2.size();
}

/*! \brief Prints a cut.
 */
template<int MaxLeaves, typename T>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, T> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}";
  return os;
}

template<int MaxLeaves, typename T>
cut<MaxLeaves, T>& cut<MaxLeaves, T>::operator=( cut<MaxLeaves, T> const& other )
{
  if ( &other != this )
  {
    _cend = _end = std::copy( other.begin(), other.end(), _leaves.begin() );
    _length = other._length;
    _signature = other._signature;
    _data = other._data;
  }
  return *this;
}

template<int MaxLeaves, typename T>
template<typename Iterator>
void cut<MaxLeaves, T>::set_leaves( Iterator begin, Iterator end )
{
  _cend = _end = std::copy( begin, end, _leaves.begin() );
  _length = static_cast<uint32_t>( std::distance( begin, end ) );
  _signature = 0;

  while ( begin != end )
  {
    _signature |= UINT64_C( 1 ) << ( *begin++ & 0x3f );
  }
}

template<int MaxLeaves, typename T>
template<typename Container>
void cut<MaxLeaves, T>::set_leaves( Container const& c )
{
  set_leaves( std::begin( c ), std::end( c ) );
}

template<int MaxLeaves, typename T>
bool cut<MaxLeaves, T>::dominates( cut const& that ) const
{
  /* quick check for counter example */
  if ( _length > that._length || ( _signature & that._signature ) != _signature )
  {
    return false;
  }

  if ( _length == that._length )
  {
    return std::equal( begin(), end(), that.begin() );
  }

  if ( _length == 0 )
  {
    return true;
  }

  // this is basically
      // return std::includes( that.begin(), that.end(), begin(), end() )
  // but it turns out that this code is faster compared to the standard
  // implementation.
  for ( auto it2 = that.begin(), it1 = begin(); it2 != that.end(); ++it2 )
  {
    if ( *it2 > *it1 )
    {
      return false;
    }
    if ( ( *it2 == *it1 ) && ( ++it1 == end() ) )
    {
      return true;
    }
  }

  return false;
}

template<int MaxLeaves, typename T>
bool cut<MaxLeaves, T>::merge( cut const& that, cut& res, uint32_t cut_size ) const
{
  if ( _length + that._length > cut_size )
  {
    const auto sign = _signature + that._signature;
    if ( uint32_t( __builtin_popcount( static_cast<uint32_t>( sign & 0xffffffff ) ) ) + uint32_t( __builtin_popcount( static_cast<uint32_t>( sign >> 32 ) ) ) > cut_size )
    {
      return false;
    }
  }

  auto it = std::set_union( begin(), end(), that.begin(), that.end(), res.begin() );
  if ( auto length = std::distance( res.begin(), it ); length <= cut_size )
  {
    res._cend = res._end = it;
    res._length = static_cast<uint32_t>( length );
    res._signature = _signature | that._signature;
    return true;
  }
  return false;
}


iFPGA_NAMESPACE_HEADER_END
