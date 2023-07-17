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
#include "node.hpp"
#include "node_hash.hpp"

#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>
iFPGA_NAMESPACE_HEADER_START

struct latch_info
{
  std::string control = "";
  uint64_t init       = 3;
  std::string type    = "";
};

struct empty_storate_data
{ };
/**
 * @brief the storage of a network, mainly contains:
 *      nodes/ inputs/ outputs/ latches/ hash/ data
 */
template<typename Node ,typename T = empty_storate_data >
struct storage
{
  storage()
  {
    nodes.reserve(10000u);
    //hash.reserve(10000u);
    hash.assign(10000u, 0);
    num_entries = 0;

    nodes.emplace_back();     // the first node generally is a constant node
  }

  using node_type = Node;

  uint64_t abc_hash(const node_type& n, uint64_t size)
  {
    uint64_t key = 0;
    key += n.children[0].index * 7937;
    key += n.children[1].index * 2971;
    key += n.children[0].weight * 911;
    key += n.children[1].weight * 353;
    return key % size;
  }

  uint64_t hash_find(const node_type& n)
  {
    uint64_t key = abc_hash(n, hash.size());
    // here 0 means no next, not found
    for(uint64_t node = hash[key]; node != 0; node = nodes[node].next)
    {
      if(nodes[node] == n) return node;
    }
    return 0;
  }

  void hash_reserve(uint64_t size)
  {
    assert(size > 0);
    std::vector<uint64_t> new_hash(size, 0);
    for(uint64_t i = 0; i < hash.size(); ++i)
    {
      for(uint64_t pEnt = hash[i], pEnt2 = pEnt ? nodes[pEnt].next : 0;
          pEnt != 0;
          pEnt = pEnt2, pEnt2 = pEnt ? nodes[pEnt].next : 0)
          {
            uint64_t key = abc_hash(nodes[pEnt], size);
            nodes[pEnt].next = new_hash[key];
            new_hash[key] = pEnt;
          }
    }
    hash.swap(new_hash);
  }

  void hash_insert(node_type n, uint64_t index)
  {
    if(num_entries > 2 * hash.size())
    {
      hash_reserve( static_cast<uint64_t>( 3.1415f * hash.size()) );
    }
    uint64_t key = abc_hash(n, hash.size());
    nodes[index].next = hash[key];
    hash[key] = index;
    ++num_entries;
  }

  void hash_erase(node_type n)
  {
    uint64_t key = abc_hash(n, hash.size());
    uint64_t cur = hash[key], pre = hash[key];
    while(!(n == nodes[cur]) && nodes[cur].next != 0)
    {
      pre = cur;
      cur = nodes[cur].next;
    }
    assert(n == nodes[cur]);
    if(cur == hash[key])
    {
      hash[key] = nodes[cur].next;
    }
    else
    {
      nodes[pre].next = nodes[cur].next;
    }
    --num_entries;
  }

  uint64_t hash_size() { return num_entries; }

  std::vector<node_type> nodes;
  std::vector<uint64_t> inputs;
  std::vector<typename node_type::pointer_type> outputs;
  std::unordered_map<uint64_t, latch_info> latch_information;

  std::vector<uint64_t> hash; 
  uint64_t num_entries;
  T data;
};  // end struct storage

iFPGA_NAMESPACE_HEADER_END
