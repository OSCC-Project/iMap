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
#include <unordered_map>
#include <vector>

#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

iFPGA_NAMESPACE_HEADER_START

/*! \brief Truth table cache.
 *
 * A truth table cache is used to store truth tables.  Many applications
 * require to assign truth tables to nodes in a network.  But since the truth
 * tables often repeat, it is more convenient to store an index to a cache
 * that stores the truth table.  In order to reduce space, only one entry for
 * a truth table and its complement are stored in the cache.  To distinguish
 * between both versions, the index to an entry in the truth table cache is
 * represented as literal.  The truth table whose function maps the input
 * assignment \f$0, \dots, 0\f$ to \f$0\f$ is considered the *normal* truth
 * table, while its complement is considered the complemented version.  Only
 * the normal truth table is stored at some index \f$i\f$.  A positive literal
 * \f$2i\f$ points to the normal truth table at index \f$i\f$.  A negative
 * literal \f$2i + 1\f$ points to the same truth table but returns its
 * complement.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      truth_table_cache cache;

      kitty::dynamic_truth_table maj( 3 );
      kitty::create_majority( maj );
      auto l1 = cache.insert( maj );  // index is 0

      auto tt = cache[l1 ^ 1]; // tt is ~maj
      auto l2 = cache.insert( tt ); // index is 1

      auto s = cache.size(); // size is 1
   \endverbatim
 */
template<typename TT>
class truth_table_cache
{
public:
  /*! \brief Creates a truth table cache and reserves memory. */
  truth_table_cache( uint32_t capacity = 1000u );

  /*! \brief Inserts a truth table and returns a literal.
   *
   * To save space, only normal functions are stored in the truth table cache.
   * A function is normal, if the input pattern \f$0, \dots, 0\f$ maps to
   * \f$0\f$.  If a function is not normal, its complement is inserted into the
   * cache and a negative literal is returned.
   *
   * The default convention for literals is assumed.  That is an index \f$i\f$
   * (starting) from \f$0\f$ has positive literal \f$2i\f$ and negative literal
   * \f$2i + 1\f$.
   *
   * \param tt Truth table to insert
   * \return Literal of position in cache
   */
  uint32_t insert( TT tt );

  /*! \brief Returns truth table for a given literal.
   *
   * The funtion requires that `lit` is smaller than `size()`.
   */
  TT operator[]( uint32_t lit ) const;

  /*! \brief Returns number of normalized truth tables in the cache. */
  auto size() const { return _data.size(); }

private:
  std::unordered_map<TT, uint32_t, kitty::hash<TT>> _indexes;
  std::vector<TT> _data;
};

template<typename TT>
truth_table_cache<TT>::truth_table_cache( uint32_t capacity )
{
  _indexes.reserve( capacity );
  _data.reserve( capacity );
}

template<typename TT>
uint32_t truth_table_cache<TT>::insert( TT tt )
{
  uint32_t is_compl{0};

  if ( kitty::get_bit( tt, 0 ) )
  {
    is_compl = 1;
    tt = ~tt;
  }

  /* is truth table already in cache? */
  const auto it = _indexes.find( tt );
  if ( it != _indexes.end() )
  {
    return static_cast<uint32_t>( 2 * it->second + is_compl );
  }

  /* add truth table to end of cache */
  const auto size = _data.size();
  const auto index = static_cast<uint32_t>( 2 * size + is_compl );
  _data.push_back( tt );
  _indexes[tt] = static_cast<unsigned int>( size );
  return index;
}

template<typename TT>
TT truth_table_cache<TT>::operator[]( uint32_t index ) const
{
  auto& entry = _data[index >> 1];
  return ( index & 1 ) ? ~entry : entry;
}

iFPGA_NAMESPACE_HEADER_END

