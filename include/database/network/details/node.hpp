/**
 * @file node.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The types of node
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 */

#pragma once

#include <stdint.h>
#include <array>
#include <vector>

#include "utils/ifpga_namespaces.hpp"
iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the pointer of a node with a weight on the first bit
 */
template<int PointerFieldSize = 0>
struct node_pointer
{
private:
  static constexpr auto _len = sizeof( uint64_t ) * 8;

public:
  node_pointer() = default;
  node_pointer( uint64_t index, uint64_t weight ) : weight( weight ), index( index ) {}

  union {
    struct
    {
      uint64_t weight : PointerFieldSize;
      uint64_t index : _len - PointerFieldSize;
    };
    uint64_t data;
  };

  bool operator==( node_pointer<PointerFieldSize> const& other ) const
  {
    return data == other.data;
  }
};

template<>
struct node_pointer<0>
{
public:
  node_pointer<0>() = default;
  node_pointer<0>( uint64_t index ) : index( index ) {}

  union {
    uint64_t index;
    uint64_t data;
  };

  bool operator==( node_pointer<0> const& other ) const
  {
    return data == other.data;
  }
};


/**
 * @brief mainly record the node state
 *  (fanout size, visited index, value ...)
 */
union node_state {
  struct
  { 
    uint64_t h1 : 32;
    uint64_t h2 : 32;
  };
  uint64_t n{0};
};

/**
 * @brief the node's fanin and size are fixed
 */
template<int Fanin ,int StateSize , int PointerFieldSize = 1, typename T=node_state>
struct fixed_node
{
  using pointer_type = node_pointer<PointerFieldSize>;
  std::array< pointer_type, Fanin >       children;
  std::array< T, StateSize >     data;         // the state of the node
  uint64_t next{0};                        // the linked list in hash table

  bool operator==(fixed_node<Fanin, StateSize, PointerFieldSize, T> const& other) const
  {
    return children == other.children;
  }               
};

/**
 * @brief the unfixed fanins type of a node
 */
template<int StateSize, int PointerFieldSize = 0, typename T=node_state>
struct mixed_node
{
  using pointer_type = node_pointer<PointerFieldSize>;
  std::vector< pointer_type >           children;
  std::array< T, StateSize >            data;
  bool operator==(mixed_node<StateSize, PointerFieldSize, T>const& other)
  {
    return children == other.children;
  }
};

iFPGA_NAMESPACE_HEADER_END

