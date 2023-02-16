/**
 * @file node_hash.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The hash function of a node
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 */
#pragma once

#include "node.hpp"

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief Hash function for 64-bit word
 * ref to http://www.voidcn.com/article/p-dzlmrsvr-buu.html 
 */
inline uint64_t hash_block( uint64_t word )
{
  /* from boost::hash_detail::hash_value_unsigned */
  return word ^ ( word + ( word << 6 ) + ( word >> 2 ) );
}

/**
 * @brief Combines two hash values
 * ref to http://www.voidcn.com/article/p-dzlmrsvr-buu.html 
 */
inline void hash_combine( uint64_t& seed, uint64_t other )
{
  /* from boost::hash_detail::hash_combine_impl */
  const uint64_t m = UINT64_C( 0xc6a4a7935bd1e995 );  // not good for 64-bit hash according ref.
  const int r = 47;

  other *= m;
  other ^= other >> r;
  other *= m;
  seed ^= other;
  seed *= m;

  seed += 0xe6546b64;
}

/**
 * @brief hash function of a multi-fanins node
 */
template<typename Node>
struct node_hash
{
  uint64_t operator()( const Node& n ) const
  {
    if ( n.children.size() == 0 )
      return 0;

    auto it = std::begin( n.children );
    auto seed = hash_block( it->data );
    ++it;

    while ( it != std::end( n.children ) )
    {
      hash_combine( seed, hash_block( it->data ) );
      ++it;
    }

    return seed;
  }
};  // end struct node_hash

iFPGA_NAMESPACE_HEADER_END