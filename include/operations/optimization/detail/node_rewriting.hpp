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
#include "network/aig_network.hpp"
#include "algorithms/ref_deref.hpp"
#include "utils/cost_functions.hpp"

#include "database_npn4_aig.hpp"

#include "kitty/kitty.hpp"
#include "kitty/npn.hpp"

#include "algorithms/simulation.hpp"

#include <stdint.h>
#include <vector>
#include <string>
#include <tuple>
#include <bitset>
#include <unordered_map>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief resynthesis the node for 4-input graph
 */
template<typename Ntk>
class node_rewriting
{
public:
  node_rewriting()
    : _vec_canonical_repre(1u << 16u)
  {
    _umap_repre_to_signal.reserve(222);
    generate_classes();

    generate_db(database_subgraphs_aig);
  }

public:
  /**
   * @brief override the operator
   */
  template<typename LeavesIterator, typename CheckGainFn>
  void operator()(Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, CheckGainFn&& fn) const
  {
    kitty::static_truth_table<4> dtt = kitty::extend_to<4u>( function );

    /// get the transformations between the truth_table input variables
    const auto [repre_tt, phase, perm] = _vec_canonical_repre[*dtt.cbegin()];

    const auto it = _umap_repre_to_signal.find( repre_tt );
    /// only do rewrite for thr pre-computed-structures
    if( it == _umap_repre_to_signal.end())
      return;
    
    std::vector<typename Ntk::signal> vec_PIs(4, ntk.get_constant(false) ); // store the cut's input variables

    std::copy(begin, end, vec_PIs.begin());

    std::unordered_map< node<Ntk>, typename Ntk::signal > db_to_aig;
    db_to_aig.insert( {0u, ntk.get_constant(false) } );
    
    for(uint8_t i = 0 ; i < 4u ; ++i)
    {
      db_to_aig.insert( { i+1, (phase >> perm[i] & 1) ? ntk.create_not(vec_PIs[perm[i]]) : vec_PIs[perm[i]]} );   // see the kitty package
    }

    /// process the cut's isologues
    for(auto const& cand : it->second)
    {
      const auto s = padding_network_by_signal(ntk, _ntk_subgraph_db.get_node(cand), db_to_aig);
      /// check the gain of the replacement ( original structure with new structure)
      if( !fn( (_ntk_subgraph_db.is_complemented(cand) != (phase >> 4 & 1))  ? ntk.create_not(s) : s ) )  // output polarity
        return;
    }
  }

private:
  /**
   * @brief generate the 4-input npn-class to the array
   */
  void generate_classes()
  {
    /// we have to using kitty package for the permutation of the input
    kitty::static_truth_table<4u> stt;
    do{
      _vec_canonical_repre[*stt.cbegin()] = kitty::exact_npn_canonization( stt );
      kitty::next_inplace(stt);
    }while( !kitty::is_const0(stt) );
  }
  
  /**
   * @brief generate the database
   *   each node's pattern in the subgraph database
   *           node
   *          /    \
   *        c1      c2
   *       /  \    /  \
   *     c11 c12  c21  c22
   */
  void generate_db(std::vector<uint32_t> const& data)
  {
    /// convert the AIG sugbraph database into network
    database_npn4_aig dna(data);
    decode_data_rewriting(_ntk_subgraph_db, dna);
    /// compute the truth table for every 4-input
    const auto sim_res = simulate_nodes< kitty::static_truth_table<4u> >(_ntk_subgraph_db);
    /// construct the isomorphic class of one node
    _ntk_subgraph_db.foreach_node( [&](auto n){
      if( std::get<0>(_vec_canonical_repre[*sim_res[n].cbegin()]) == sim_res[n] )
      {
        if( _umap_repre_to_signal.count(sim_res[n]) == 0 )
        {
          _umap_repre_to_signal.insert( {sim_res[n], {_ntk_subgraph_db.make_signal(n)}} );
        }
        else
        {
          _umap_repre_to_signal[ sim_res[n] ].push_back(_ntk_subgraph_db.make_signal(n) );
        }
      }
      else  // compute the complement's cond
      {
        const auto tmp_tt = ~sim_res[n];
        if( std::get<0>(_vec_canonical_repre[*tmp_tt.cbegin()]) == tmp_tt)
        {
          if( _umap_repre_to_signal.count(tmp_tt) == 0 )
          {
            _umap_repre_to_signal.insert( {tmp_tt, {!_ntk_subgraph_db.make_signal(n)}} );
          }
          else
          {
            _umap_repre_to_signal[tmp_tt].push_back( !_ntk_subgraph_db.make_signal(n) );
          }
        } 
      }
    });
  }

  /**
   * @brief create the subgraph-network for the signal in the database
   *    we rewrite the network by generate a new network
   *    so we uing the padding function for the new network
   */
  typename Ntk::signal padding_network_by_signal( Ntk& ntk, 
                                                  node<Ntk> const& n, 
                                                  std::unordered_map<node<Ntk>, typename Ntk::signal>& db_map) const
  {
    if( auto it = db_map.find(n); it != db_map.end() )
    {
      return it->second;
    }

    std::array<typename Ntk::signal, 2> children{};
    _ntk_subgraph_db.foreach_fanin(n, [&](auto const& s, auto i){
      const auto tmp_s = padding_network_by_signal(ntk, _ntk_subgraph_db.get_node(s), db_map);
      children[i] = _ntk_subgraph_db.is_complemented(s) ? ntk.create_not(tmp_s) : tmp_s;
    });

    const auto s = ntk.create_and(children[0], children[1]);
    db_map.insert( {n, s} );
    return s;
  }
  
public:
  int db_size() const
  {
    return _ntk_subgraph_db.size();
  }
  int npn_class() const
  {
    return _umap_repre_to_signal.size();
  }

private:
  std::vector< std::tuple< kitty::static_truth_table<4u>, 
               uint32_t, 
               std::vector<uint8_t>> 
             >         _vec_canonical_repre;                       // store the representation of each truth-table
  std::unordered_map< kitty::static_truth_table<4u>, 
                      std::vector<typename Ntk::signal>, 
                      kitty::hash<kitty::static_truth_table<4u>>
                    >  _umap_repre_to_signal;                 //truth table map to the signal in the network
  Ntk                  _ntk_subgraph_db;                           // store the subgraph database
};  // end class node_rewriting

iFPGA_NAMESPACE_HEADER_END
