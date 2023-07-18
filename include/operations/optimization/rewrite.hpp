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
#include "algorithms/cleanup.hpp"
#include "algorithms/ref_deref.hpp"
#include "cut/cut_enumeration.hpp"
#include "cut/rewrite_cut_enumeration.hpp"
#include "detail/node_rewriting.hpp"
#include "utils/ifpga_namespaces.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

iFPGA_NAMESPACE_HEADER_START

struct rewrite_params
{
  rewrite_params()
  {
    cut_enumeration_ps.cut_size  = 4;
    cut_enumeration_ps.cut_limit = 10;
  }
  iFPGA_NAMESPACE::cut_enumeration_params cut_enumeration_ps{};

  bool b_use_zero_gain{ true };
  bool b_preserve_depth{ true };
};
namespace detail {

/**
 * @brief DAG-aware rewriting based on priority cut. 
*/
template <typename Ntk, typename RewritingFn>
class rewrite_impl
{
 public:
  using node_t         = typename Ntk::node;
  using signal_t       = typename Ntk::signal;
  using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, true, iFPGA_NAMESPACE::cut_enumeration_rewrite_cut>;
  using cut_t          = typename network_cuts_t::cut_t;
  using cut_set_t      = typename network_cuts_t::cut_set_t;

  rewrite_impl( Ntk&                   ntk,
                RewritingFn const&     rewriting_fn,
                rewrite_params const&  ps)
      : _ntk( ntk ),
        _rewriting_fn( rewriting_fn ),
        _ps( ps )
  {  }

  /**
   * @brief the main algorithm procedure
   * @return res network
   */
  Ntk run() {
    
    initialize();
    const auto cuts_list = cut_enumeration<Ntk, true, cut_enumeration_rewrite_cut>(_ntk, _ps.cut_enumeration_ps);

    std::map<node_t, signal_t> best_replacement;

    /// process rewrite on each nodes
    uint32_t size = _ntk.size();
    printf("perform best signal selection int ing\n");
    _ntk.foreach_gate([&](auto const& n){
      auto index = _ntk.node_to_index(n);
      if(index >= size)
        return;
      if(_ntk.fanout_size(n) > 1000)
        return;

      int best_gain = -1;
      signal_t best_signal;

      for(auto& cut : cuts_list.cuts(_ntk.node_to_index(n))) {
        if(cut->size() != 4u)
          continue;
        const auto tt = cuts_list.truth_table(*cut);

        std::vector<signal_t> children;
        std::vector<node_t> children_nodes;
        for(auto leaf : *cut) {
          children_nodes.emplace_back(_ntk.index_to_node(leaf));
          children.emplace_back(_ntk.make_signal( _ntk.index_to_node(leaf)));
        }

        int cost_before = deref_cut(n, children_nodes);
       
        const auto on_signal = [&]( auto const& s ) {
          auto ns = _ntk.get_node(s);
          int cost_tmp = ref_cut(ns, children_nodes);
          deref_cut(ns, children_nodes);

          update_arrive_depth(ns, children_nodes);
          
          int slack = _depth_require[n] - _depth_arrive[ns];
          if( !_ps.b_preserve_depth || (_ps.b_preserve_depth && slack >= 0) ) {
            if( best_gain < cost_before - cost_tmp) {
              best_gain = cost_before - cost_tmp;
              best_signal = s;
            }
          }
          return true;
        };
        
        _rewriting_fn(_ntk, tt , children.begin(), children.end(), on_signal);
        ref_cut(n, children_nodes);
      }

      if(best_gain > 0 || (_ps.b_use_zero_gain && best_gain == 0)) {
        best_replacement[n] = best_signal;
      }
    });

    // local replacement for the candidate best signal
    printf("perform local replacement ing\n");
    // uint i = 0;
    // size = best_replacement.size();
    for(auto cand : best_replacement){
      // printf("processing at [%d/%d]\n", ++i, size);
      if(cand.first == _ntk.get_node(cand.second))
        continue;
      local_replacement(cand.first, cand.second);
    }

    return cleanup_dangling(_ntk);
  }

 private:
  void initialize() {
    initialize_fanouts();
    initialize_reference();
    initialize_depth();
  }

  void initialize_fanouts() {
    _ntk.foreach_gate([&](auto const& n){
      _ntk.foreach_fanin(n, [&](auto const& sc){
        auto nc = _ntk.get_node(sc);
        _fanouts[nc].emplace_back( signal_t(n, _ntk.is_complemented(sc)) );
      });
    });

    _ntk.foreach_po([&](auto const& po){
      auto npo = _ntk.get_node(po);
      _fanouts[npo].emplace_back( po );
    });
  }

  /**
   * @brief initialize the reference count of each node
  */
  void initialize_reference() {
    _ntk.clear_values();
    _ntk.foreach_node([&](auto const& n){
      _ntk.set_value(n, _ntk.fanout_size(n) );
    });
  }

  /**
   * @brief initialize the arrive depth and require depth for each node
  */
  void initialize_depth() {
    int depth_max = 0;
    // set arrive depth first
    _ntk.foreach_node([&](auto const& n){
      _depth_arrive[n] = -1;     // init each arrive depth be -1
      if(_ntk.is_pi(n) || _ntk.is_pi(n)) {
        _depth_arrive[n] = 0u;
      } else {
        _ntk.foreach_fanin(n, [&](auto const& sc){
          _depth_arrive[n] = std::max(_depth_arrive[n], _depth_arrive[_ntk.get_node(sc)]+1);
        });
      }
      _depth_require[n] = 100000; // init each require depth be +∞
    });

    // set the require depth
    _ntk.foreach_po([&](auto const& po){
      depth_max = std::max(depth_max, _depth_arrive[_ntk.get_node(po)]);
    });

    _ntk.foreach_po([&](auto const& po){
      _depth_require[_ntk.get_node(po)] = depth_max;
      compute_require_depth_rec( _ntk.get_node(po) );
    });
  }

  /**
   * @brief recursively compute the require depth
   * @param n root node for the cone
  */
  void compute_require_depth_rec(node_t n) {
    if(_ntk.is_pi(n) || _ntk.is_constant(n)) {
      return;
    }
    _ntk.foreach_fanin(n, [&](auto const& sc) {
      auto nc = _ntk.get_node(sc);
      if(_depth_require[nc] > _depth_require[n] - 1) {
        _depth_require[nc] = _depth_require[n] - 1;
        compute_require_depth_rec(nc);
      }
    });
  }

  /**
   * @brief update the arrive for the padding subgraph
   * @param n
   * @param leaves
   * @return depth for n
  */
  int update_arrive_depth(node_t const& n, std::vector<node_t> const& leaves) {
    int res = 0;
    _ntk.foreach_fanin(n, [&](auto const& sc){
      auto nc = _ntk.get_node(sc);
      if(std::find(leaves.begin(), leaves.end(), nc) != leaves.end()) {
        res = std::max(res, _depth_arrive[nc] + 1);
        _depth_arrive[n] = std::max(_depth_arrive[n], _depth_arrive[nc] + 1);
      }
      else {
        res = std::max(res, update_arrive_depth(nc, leaves) + 1);
        _depth_arrive[n] = std::max(_depth_arrive[n], update_arrive_depth(nc, leaves) + 1);
      }
    });
    return res;
  }

  /**
   * @brief deref the cone from n to leaves
   * @param n
   * @param leaves
   * @return 
  */
  int deref_cut(node_t const& n, std::vector<node_t> const& leaves) {
    if(std::find(leaves.begin(), leaves.end(), n) != leaves.end()) {
      return 0;
    }
    int res = 1;
    _ntk.foreach_fanin(n, [&](auto const& sc){
      auto nc = _ntk.get_node(sc);
      _ntk.decr_value( nc );
      if( _ntk.value(nc) == 0 ) {
        res = res + deref_cut(nc, leaves);
      }
    });
    return res;
  }

  /**
   * @brief ref the cone from n to leaves
   * @param n
   * @param leaves
   * @return 
  */
  int ref_cut(node_t const& n, std::vector<node_t> const& leaves) {
    if(std::find(leaves.begin(), leaves.end(), n) != leaves.end()) {
      return 0;
    }
    int res = 1;
    _ntk.foreach_fanin(n, [&](auto const& sc){
      auto nc = _ntk.get_node(sc);
      _ntk.incr_value( nc );
      if( _ntk.value(nc) == 1 ) {
        res = res + ref_cut(nc, leaves);
      }
    });
    return res;
  }

  /**
   * @brief perform the local placement
   * @param old_n
   * @param new_s
   * @param leaves
  */
  void local_replacement(node_t const& old_n, signal_t const& new_s) {
    // deref + ref
    auto new_n = _ntk.get_node(new_s);
    deref_node_recursive(_ntk, old_n);
    ref_node_recursive(_ntk, old_n);

    // refanouts
    // reset po signal
    bool is_po = false;
    _ntk.foreach_po([&](auto const& po, auto const& index){
      if(is_po)
        return;
      auto npo = _ntk.get_node(po);
      if(npo == old_n) {
        auto new_po = signal_t(new_n, _ntk.is_complemented(po)^_ntk.is_complemented(new_s) );
        _ntk.set_po_at(index, new_po);
        is_po = true;
      }
    });

    if( !is_po ) {
      for(auto sfo : _fanouts[old_n]) {
        auto nfo = _ntk.get_node(sfo);
        auto sc0 = _ntk.get_child0(nfo);
        auto sc1 = _ntk.get_child1(nfo);
        if(_ntk.get_node(sc0) == old_n) {
          _ntk.set_child0(nfo, signal_t(new_n, _ntk.is_complemented(new_s)^_ntk.is_complemented(sc0)));
        }
        if(_ntk.get_node(sc1) == old_n) {
          _ntk.set_child1(nfo, signal_t(new_n, _ntk.is_complemented(new_s)^_ntk.is_complemented(sc1)));
        }
      }
    }
  }

 private:
  Ntk&                  _ntk;
  RewritingFn const&    _rewriting_fn;
  rewrite_params const& _ps;

  std::unordered_map<node_t, std::vector<signal_t>> _fanouts;
  std::unordered_map<node_t, int> _depth_arrive;
  std::unordered_map<node_t, int> _depth_require;
};  // end class rewrite_impl

};  // end namespace detail

template <typename Ntk         = iFPGA_NAMESPACE::aig_network,
          typename RewritingFn = node_rewriting<Ntk> >
Ntk rewrite( Ntk& ntk, rewrite_params const& params )
{
  RewritingFn resynthesis_fn;
  const auto dest = detail::rewrite_impl<Ntk, RewritingFn>{ ntk,
                                                            resynthesis_fn,
                                                            params}.run();
  return dest;
}

iFPGA_NAMESPACE_HEADER_END
