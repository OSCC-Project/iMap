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
#include "utils/common_properties.hpp"
#include "cut/cut_enumeration.hpp"
#include "cut/general_cut_enumeration.hpp"

#include "views/mapping_view.hpp"
#include "techmap-lib/lut_cell_lib.hpp"
#include "detail/map_qor.hpp"
#include "debugger.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <memory>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <set>
#include <assert.h>
#include <type_traits>

iFPGA_NAMESPACE_HEADER_START

template<class Ntk , typename CutData = iFPGA_NAMESPACE::general_cut_data>
struct klut_mapping_storage
{
  klut_mapping_storage(uint32_t size)
    : topo_order(size, 0u),
      topo_order_reverse(size, 0u),
      arrival_times(size, -1.0f),
      require_times(size, std::numeric_limits<float>::max()),
      est_refs(size, 0.0f),
      refs(size, 0u),
      visits(size, 0u),
      visits_copy(size, 0u)
  { }

  std::vector< node<Ntk> >  topo_order;                  // only contain the and gate topo-nodes
  std::vector< node<Ntk> >  topo_order_reverse;
  std::vector<float>        arrival_times;
  std::vector<float>        require_times;
  std::vector<float>        est_refs;                    // because of the depth constraints, we using estimate fanout of a node to compute area flow
  std::vector<uint32_t>     refs;                        // the ref count of node
  std::vector<uint32_t>     visits;     
  std::vector<uint32_t>     visits_copy;

  // statistics
  float_t                   delay_current{0.0f};
  float_t                   area_current{0.0f};

  float_t                   required_glo{0.0f};          // same as delay
  float_t                   area_glo{0.0f};
  float_t                   power_glo{0.0f};
  uint32_t                  edge_size{0};                // total net size of current mapping
};


struct klut_mapping_params
{
  klut_mapping_params()
  {
    cut_enumeration_ps.cut_size = 6;
    cut_enumeration_ps.cut_limit = 10;
  }
  iFPGA_NAMESPACE::cut_enumeration_params cut_enumeration_ps{};

  bool         bPreprocess{true};     // preprocessing
  bool         bArea{false};          // area-oriented mapping
  bool         bEdge{true};           // using edge
  bool         bPower{false};         // using power
  bool         bFancy{false};
  bool         bZeroGain{true};       // allow zero gain when replace the best cut
  uint8_t      uFlowIters{1};
  uint8_t      uAreaIters{2};
  ETypeCmp     eSortMode{ETC_DELAY};  // sort mode of the cuts
  ETypeMode    eCutMode;              // mode for cut value computation
  bool         bUseLutLib{false};
  int          iDelayTraget{-1};
  float        fAndDelay{1.0f};
  float        fAndArea{1.0f};
  float        fEpsilon{0.005f};
  bool         bDebug{false};
  bool         verbose{false};
};

struct klut_mapping_stats
{
  float_t delay{0.0f};
  float_t area{0.0f};
  float_t edge{0.0f};

  void report() const
  {
    printf( "[i] Delay = %0.6f\n", delay );
    printf( "[i] Area  = %0.6f\n", area  );
    printf( "[i] Edge  = %0.6f\n", edge  );
  }
};

namespace detail
{
template<class Ntk, bool StoreFunction, typename CutData = iFPGA_NAMESPACE::general_cut_data>
class klut_mapping_impl
{
  public:
    using network_cuts_t         = iFPGA_NAMESPACE::network_cuts<Ntk, StoreFunction, CutData>;
    using cut_enumeration_impl_t = iFPGA_NAMESPACE::detail::cut_enumeration_impl<Ntk, StoreFunction, CutData>;
    using cut_t                  = typename network_cuts_t::cut_t;
    using cut_set_t              = typename network_cuts_t::cut_set_t;
    using node_t                 = typename Ntk::node;
    using klut_storage           = klut_mapping_storage<Ntk, CutData>;
    static constexpr node_t AIG_NULL = Ntk::AIG_NULL;

    klut_mapping_impl(Ntk& ntk, klut_mapping_params const& ps, klut_mapping_stats const& st)
      : _ntk(ntk),
        _storage( std::make_shared<klut_storage>(ntk.size()) ),
        _ps(std::make_shared<klut_mapping_params>(ps)),
        _st(std::make_shared<klut_mapping_stats>(st)),
        _cut_network(ntk.size())
    { }

  public:
    /**
     * @brief main process for klut mapping algorithm
     */
    void run()
    {
      init_parameters();
      
      // mapping body
      perform_mapping();  

      // print the initialization informations
      if(_ps->verbose)
      {
        print_network();
        print_params();
        print_storage();
      }
    }

    /**
     * @brief public function for QoR call on
     */
    float get_best_delay() const { return _storage->delay_current; }
    float get_best_area()  const { return _storage->area_current; }

  private:
    /**
     * @brief initialize the parameters to default value
     */
    void init_parameters()
    {
      _ps->bPreprocess   = true;    
      _ps->bArea         = false;  
      _ps->bEdge         = true;  
      _ps->bPower        = false;  
      _ps->bFancy        = false;
      _ps->eSortMode     = ETC_DELAY;
      _ps->eCutMode      = ETM_DELAY;
      _ps->bUseLutLib    = false;
      _ps->uFlowIters    = 1;
      _ps->uAreaIters    = 2;
      _ps->iDelayTraget  = -1;
      _ps->fEpsilon      = 0.005f;
      _ps->verbose       = false;
    }

    /**
     * @brief combinational k-LUT mapping
     * 
     */
    void perform_mapping()
    {
      uint32_t i;

      // compute trivial cut for constant and PIs
      _cut_network.add_unit_cut( 0 );
      _cut_network.set_best_cut(0, _cut_network.cuts(0).best());
      _storage->arrival_times[0] = 0.0f;
      _storage->refs[0] = 1u;
      _storage->est_refs[0] = 1.0f;

      _ntk.foreach_ci([&](auto const& n){
        _cut_network.add_unit_cut( n );
        _cut_network.set_best_cut(n, _cut_network.cuts(n).best());
        _storage->arrival_times[n] = 0.0f;
        _storage->refs[n] = 1u;
        _storage->est_refs[n] = 1.0f;
      });

      // compute the topo-order
      topologize();
      
      // combinational mapping
      if(_ps->bPreprocess && !_ps->bArea)
      {
        // mapping for delay
        perform_mapping_round(0, 1, 1 );  // delay
        
        // mapping for delay second option
        _ps->bFancy = 1;
        reset_refs();
        perform_mapping_round(0, 1, 0 ); // delay-2
        _ps->bFancy = 0;
        
        // mapping for area
        _ps->bArea  = 1;
        reset_refs();
        perform_mapping_round(0, 1, 0 );   // area
        _ps->bArea  = 0;
      }
      else
      {
        perform_mapping_round(0, 0, 1 );  // delay
      }

      // area_flow oriented mapping
      for(i = 0 ; i < _ps->uFlowIters; ++i)
      {
        perform_mapping_round(1, 0 , 0 );  // area flow
      }
      
      // area oriented mapping
      for(i = 0 ; i < _ps->uAreaIters; ++i)
      {
        perform_mapping_round(2, 0 , 0 );  // area
      }

      // get the mapped network from the flow!
      derive_final_mapping();
    }

    /**
     * @brief the mapping rounds in every step
     * 
     * @param mode        ETC_DELAY, ETC_FLOW, ETC_AREA   
     * @param preprocess  toogles for preprocess
     * @param first       mark the first mapping round
     */
    void perform_mapping_round(int mode, bool preprocess, bool first)
    {
      uint32_t i{0u};
      assert(mode >= 0 && mode <= 2);

      if(mode == 2)
      {
        _ps->eCutMode = ETM_AREA;
        gf_set_etm(ETM_AREA);

      }
      else
      {
        _ps->eCutMode = ETM_FLOW;
        gf_set_etm(ETM_FLOW);
      }
      
      if(mode || _ps->bArea)
      {
        _ps->eSortMode = ETC_AREA;
        gf_set_etc(ETC_AREA);
      }
      else if( _ps->bFancy)
      {
        _ps->eSortMode = ETC_DELAY2;
        gf_set_etc(ETC_DELAY2);
      }
      else
      {
        _ps->eSortMode = ETC_DELAY;
        gf_set_etc(ETC_DELAY);
      }

      // standard mapping steps for each node
      _ntk.clear_visited();

      for(i = 0 ; i < _storage->topo_order.size(); ++i)
      {
        auto n = _storage->topo_order[i];
        if(_ntk.is_ci(n) || _ntk.is_constant(n))
          continue;
        else{
          perform_mapping_and(n, mode, preprocess, first);
          if( _ntk.is_repr(n) )
          {
            perform_mapping_and_choice(n, mode, preprocess);
          }
        }
      }

      _ntk.clear_visited();

      compute_required_times(); // some bugs here

      if(_ps->verbose)
      {
        printf("\t[Delay] %f \t", _storage->required_glo);
        printf("[Area] %f \n", _storage->area_glo);
      }
    }

    /**
     * @brief perform mapping for each and gate
     *    1. finds the best cut for the given node
     * @param n the node in the network _ntk
     * @param mode 
     * @param preprocess 
     * @param first 
     */
    void perform_mapping_and(node_t const& n, int mode, bool preprocess, bool first)
    {
      // compute the estmate ref
      if(mode == 0)
      {
        _storage->est_refs[n] = (float)_storage->refs[n];
      }
      else if(mode == 1)
      {
        _storage->est_refs[n] = (float)( (_storage->refs[n] + 2*_storage->est_refs[n]) / 3.0f);
      }

      // deref the best cut
      if(mode && _storage->refs[n] > 0u)
      {
        cut_area_deref(_cut_network.get_best_cut(n));
      }

      // generate cuts
      merge_cuts(n);

      // update the best cut without increasing the delay, trival cut can not be the best cut of the node itself
      if( (_cut_network.cuts(n).best().size() < 1u) || 
          (_cut_network.cuts(n).best()->data.delay < _storage->require_times[n] + _ps->fEpsilon) || 
          (_ps->bZeroGain && _cut_network.cuts(n).best()->data.delay == _storage->require_times[n] + _ps->fEpsilon) )
      {
        _cut_network.set_best_cut(n, _cut_network.cuts(n).best());
        _storage->arrival_times[n] = _cut_network.get_best_cut(n)->data.delay;
      }

      // ref the best cut
      if(mode && _storage->refs[n] > 0u)
      {
        cut_area_ref(_cut_network.get_best_cut(n));
      }

      return;
    }

    /**
     * @brief perform mapping for the choice node
     * 
     * @param n the representation node of this equivalent class
     * @param mode 
     * @param preprocess 
     */
    void perform_mapping_and_choice(node_t const& n, int mode, bool preprocess)
    {
      // deref the best cut
      if(mode && _storage->refs[n] > 0u)
      {
        cut_area_deref(_cut_network.get_best_cut(n));
      }

      auto& cuts_repr = _cut_network.cuts(n);
      auto next_choice_node = _ntk.get_equiv_node(n);

      while(next_choice_node != AIG_NULL)
      {
        if( _cut_network.cuts(next_choice_node).size() ==0 )
        {
          next_choice_node = _ntk.get_equiv_node(next_choice_node);
          continue;
        }
        auto cut_phase = _ntk.phase(n) ^ _ntk.phase(next_choice_node);
        for( auto it = _cut_network.cuts(next_choice_node).begin(); it != _cut_network.cuts(next_choice_node).end(); ++it )
        {
          if( (**it).size() < 2 ) // skip the trival cut
            continue;
          // flip truth table according to cut phase
          if( mode != ETC_DELAY && (**it)->data.delay > _storage->require_times[next_choice_node] + _ps->fEpsilon )
            continue;
          cut_t tc = **it;
          if( cut_phase )
          {
            tc->func_id ^= 1;
          }
          cuts_repr.insert(tc);
        }

        _cut_network.cuts( n ).limit( _ps->cut_enumeration_ps.cut_limit-1);

        next_choice_node = _ntk.get_equiv_node(next_choice_node);
      }

      // update the best cut without increasing the delay
      // if( _cut_network.cuts(n).best().size() > 1 && (!preprocess ||  _cut_network.cuts(n).best()->data.delay <=  _storage->require_times[n] + _ps->fEpsilon) )
      if( (_cut_network.cuts(n).best()->data.delay < _storage->require_times[n] + _ps->fEpsilon) || 
          (_ps->bZeroGain && _cut_network.cuts(n).best()->data.delay == _storage->require_times[n] + _ps->fEpsilon)  )
      {
        _cut_network.set_best_cut(n, _cut_network.cuts(n).best());
        _storage->arrival_times[n] = _cut_network.get_best_cut(n)->data.delay;
      }

      // after insert the choice cuts, the trival cut will eliminates
      _cut_network.add_unit_cut( n );

      // ref the best cut
      if(mode && _storage->refs[n] > 0u)
      {
        cut_area_ref(_cut_network.get_best_cut(n));
      }

      return;
    }

    int visit_initialize()
    {
      int nCutSize = 0, nCutMax = 0;
      node_t child;
      for(auto n : _storage->topo_order)
      {
        if(_ntk.is_ci(n) || _ntk.is_constant(n))  continue;
        
        if(nCutMax < ++nCutSize)  nCutMax = nCutSize;
        
        if(_storage->visits[n] == 0)  --nCutSize;

        child = _ntk.get_node(_ntk.get_child0(n));
        if( !_ntk.is_ci(child) &&  --_storage->visits[child] == 0)  --nCutSize;
        child = _ntk.get_node(_ntk.get_child1(n));
        if( !_ntk.is_ci(child) &&  --_storage->visits[child] == 0)  --nCutSize;

        auto next_choice_node = _ntk.get_equiv_node(n);

        while(next_choice_node != AIG_NULL)
        {
          if( !_ntk.is_ci(next_choice_node) &&  --_storage->visits[child] == 0)  --nCutSize;
          next_choice_node = _ntk.get_equiv_node(next_choice_node);
        }
      }

      for(auto n : _storage->topo_order)
      {
        assert( _ntk.is_ci(n) || _ntk.is_constant(n) || _storage->visits[n] == 0u);
        _storage->visits_copy[n] = _storage->visits[n];
      }

      assert(nCutSize == 0);
      return nCutMax;
    }

    /**
     * @brief compute require_times of current network
     */
    void compute_required_times()
    {
      // step1 computes area, references and nodes used in the mapping!
      std::fill(_storage->require_times.begin(), _storage->require_times.end(), std::numeric_limits<float>::max());
      clear_refs();
      _storage->area_glo = 0.0f;
      _storage->required_glo = 0.0f;
      _storage->power_glo = 0.0f;
      _storage->edge_size = 0u;

      // compute the global area and global required time
      _ntk.foreach_co( [&]( auto s){
        auto n = _ntk.get_node(s);
        _storage->area_glo += mark_current_mapping_rec( n );

        if( _storage->required_glo < _storage->arrival_times[n] )
          _storage->required_glo = _storage->arrival_times[n];
      });

      // update the required time for POs by the global required time
      _ntk.foreach_co([&]( auto s){
        _storage->require_times[  _ntk.get_node(s) ] = _storage->required_glo;
      });

      if( _ps->bArea )
        return;
      
      // propagate required times from POs to PIs
      for(auto it = _storage->topo_order.rbegin(); it != _storage->topo_order.rend(); ++it)
      {
        auto an = *it;
        if(_ntk.is_ci(an) || _ntk.is_constant(an))
        {
          continue;
        }
        else
        {
          if( _storage->refs[an] == 0u )
            continue;
          auto required = _storage->require_times[ an ];          
          for( auto leaf : _cut_network.get_best_cut(an) )
          {
            _storage->require_times[ leaf ] = std::min( _storage->require_times[ leaf ],  required - 1.0f);
          }
          
          // assign the choice node the same require time
          auto next_choice_node = _ntk.get_equiv_node(an);
          while(next_choice_node != AIG_NULL)
          {
            _storage->require_times[ next_choice_node ] = _storage->require_times[ an ];
            next_choice_node = _ntk.get_equiv_node(next_choice_node);
          }
        }
      }
      return;
    }

    /**
     * @brief compute the 
     * 
     * @param n 
     * @return float 
     */ 
    float mark_current_mapping_rec(node_t const& n)
    {
      float area{0.0f};
      _storage->edge_size = 0u;
      if( _storage->refs[n]++ || _ntk.is_ci(n) || _ntk.is_constant(n) )
      {
        return 0.0f;
      }

      auto& best_cut = _cut_network.get_best_cut(n);
      
      area = lut_area(best_cut);
      _storage->edge_size += best_cut.size();
      for(auto& leaf : best_cut)
      {
        area += mark_current_mapping_rec( leaf );
      }
      return area;
    }

    /**
     * @brief 
     * 
     * @param n 
     */
    void mark_ref_rec( node_t const& n)
    {
      if( _storage->refs[n]++ || _ntk.is_ci(n) || _ntk.is_constant(n) )
      {
        return;
      }
      ++_storage->refs[n];
      for(auto& leaf : _cut_network.get_best_cut(n))
      {
        mark_ref_rec(leaf);
      }
    }

    /**
     * @brief perform best cut selection for mapped node
     *  select best cut according the _map_ref vector
     */
    void derive_final_mapping()
    {
      float tmp_area  = 0.0f;
      float tmp_delay = 0.0f;

      clear_refs();
      _ntk.foreach_po( [&]( auto s){
        const auto n = _ntk.get_node(s);
        tmp_delay = std::max( tmp_delay, _storage->arrival_times[n]);   // compute current delay
        mark_ref_rec(n);
      });

      for(auto it = _storage->topo_order.rbegin(); it != _storage->topo_order.rend(); ++it)
      {
        auto n = *it;

        if( _storage->refs[n] == 0u || _ntk.is_ci(n) || _ntk.is_constant(n) )
          continue;

        assert( _cut_network.get_best_cut(n).size() > 1);

        std::vector< node_t > nodes;
        for( auto leaf : _cut_network.get_best_cut(n) )
        {
          nodes.emplace_back( leaf );
        }
        ++tmp_area;                                              // compute current area
        _ntk.add_to_mapping( n, nodes.begin(), nodes.end() );

        if constexpr ( StoreFunction)
        {
          _ntk.set_cell_function( n, _cut_network.truth_table( _cut_network.get_best_cut(n) ));
        }
      }
      _storage->delay_current = tmp_delay;
      _storage->area_current  = tmp_area;
      return;
    }

#pragma region cut data

  float lut_area(cut_t const& cut)  
  { 
    return 1.0f; 
  }

  float lut_delay(cut_t const& cut)  
  { 
    return 1.0f;
  }

  float cut_delay(cut_t const& cut)
  {
    float delay = -1.0f;
    for(auto leaf : cut)
    {
      const auto& best_leaf_cut = _cut_network.get_best_cut(leaf);
      delay = std::max(delay, best_leaf_cut->data.delay);
    }
    return delay + 1.0f;
  }


  float cut_area_deref(cut_t const& cut)
  { 
    float area = 1.0f;
    for(auto leaf : cut)
    {
      assert(_storage->refs[leaf] > 0u);
      if( --_storage->refs[leaf] > 0u || _ntk.is_pi(leaf) || _ntk.is_constant(leaf) )
        continue;
      area += cut_area_deref( _cut_network.get_best_cut(leaf));
    }
    return area;
  }

  float cut_area_ref(cut_t const& cut)
  { 
    float area = 1.0f ;
    for(auto leaf : cut)
    {
      assert(_storage->refs[leaf] >= 0u);
      if( _storage->refs[leaf]++ > 0u || _ntk.is_pi(leaf) || _ntk.is_constant(leaf) )
        continue;
      area += cut_area_ref(_cut_network.get_best_cut(leaf));
    }
    return area;
  }

  float cut_area_derefed(cut_t const& cut)
  {
    if(cut.size() < 2)  
      return 0;
    [[maybe_unused]] float res1 = cut_area_ref(cut);
    float res2 = cut_area_deref(cut);
    assert(std::fabs(res1 - res2) < 0.005f);
    return res2;
  }

  float cut_area_refed(cut_t const& cut)
  {
    if(cut.size() < 2)  
      return 0;
    [[maybe_unused]] float res1 = cut_area_deref(cut);
    float res2 = cut_area_ref(cut);

    assert(std::fabs(res1 - res2) < 0.005f);
    return res2;
  }

  float cut_edge_deref(cut_t const& cut)
  {
    float edge = (float)cut.size();
    for(auto leaf : cut)
    {
      assert(_storage->refs[leaf] > 0u);
      if( --_storage->refs[leaf] > 0u || _ntk.is_pi(leaf) || _ntk.is_constant(leaf) )
        continue;
      edge += cut_edge_deref(_cut_network.get_best_cut(leaf) );
    }
    return edge;
  }

  float cut_edge_ref(cut_t const& cut)
  {
    float edge = (float)cut.size();
    for(auto leaf : cut)
    {
      assert(_storage->refs[leaf] >= 0u);
      if( _storage->refs[leaf]++ > 0u || _ntk.is_pi(leaf) || _ntk.is_constant(leaf) )
        continue;
      edge += cut_edge_ref(_cut_network.get_best_cut(leaf));
    }
    return edge;
  }

  float cut_edge_derefed(cut_t const& cut)
  {
    if(cut.size() < 2)  
      return 0;
    [[maybe_unused]] float res1 = cut_edge_ref(cut);
    float res2 = cut_edge_deref(cut);

    assert(std::fabs(res1 - res2) < 0.005f);

    return res2;
  }

  float cut_edge_refed(cut_t const& cut)
  {
    float res1, res2;
    if(cut.size() < 2)  
      return 0;
    res1 = cut_edge_deref(cut);
    res2 = cut_edge_ref(cut);

    assert(std::fabs(res1 - res2) < 0.005f);

    return res2;
  }

  float cut_area_flow(cut_t const& cut)
  {
    float area_flow = 1.0f, addon_area_flow;
    for(auto leaf : cut)
    {
      const auto& best_leaf_cut = _cut_network.get_best_cut(leaf);
      if( _storage->refs[leaf] == 0u || _ntk.is_constant(leaf) )
        addon_area_flow = best_leaf_cut->data.area;
      else
      {
        assert(_storage->est_refs[leaf] > _ps->fEpsilon);
        addon_area_flow = best_leaf_cut->data.area / _storage->est_refs[leaf];
      }

      if(area_flow >= (float)1e32 || addon_area_flow >= (float)1e32)
      {
        area_flow = (float)1e32;
      }
      else
      {
        area_flow += addon_area_flow;
        if(area_flow >= (float)1e32)
          area_flow = (float)1e32;
      }
    }
    return area_flow;
  }

  float cut_edge_flow(cut_t& cut)
  {
    float edge_flow = (float)cut.size(), addon_edge_flow;
    for(auto leaf : cut)
    {
      const auto& best_leaf_cut = _cut_network.get_best_cut(leaf);
      if( _storage->refs[leaf] == 0u || _ntk.is_constant(leaf) )
        addon_edge_flow = best_leaf_cut->data.edge;
      else
      {
        assert(_storage->est_refs[leaf] > _ps->fEpsilon);
        addon_edge_flow = best_leaf_cut->data.edge / _storage->est_refs[leaf];
      }

      if(edge_flow >= (float)1e32 || addon_edge_flow >= (float)1e32)
      {
        edge_flow = (float)1e32;
      }
      else
      {
        edge_flow += addon_edge_flow;
        if(edge_flow >= (float)1e32)
          edge_flow = (float)1e32;
      }
    }
    return edge_flow;
  }

#pragma endregion cut data

#pragma region Cut Enumeration

    void merge_cuts(uint32_t index)
    {
      auto n = _ntk.index_to_node( index );
      const auto fanin = 2;
      uint32_t pairs{1};
      std::array<uint32_t, 2> children_id;      // store the children's index

      auto child0_index = _ntk.get_node( _ntk.get_child0(n) );
      auto child1_index = _ntk.get_node( _ntk.get_child1(n) );
      
      children_id[0] = child0_index;
      children_id[1] = child1_index;
      
      _lcuts[0] = &_cut_network.cuts( child0_index );
      _lcuts[1] = &_cut_network.cuts( child1_index );

      pairs *= static_cast<uint32_t>( _lcuts[0]->size() );
      pairs *= static_cast<uint32_t>( _lcuts[1]->size() );
      
      _lcuts[2] = &_cut_network.cuts( index );

      auto& rcuts = *_lcuts[fanin];

      rcuts.clear();                            // update current n's cutset by fanins

      cut_t new_cut;

      std::vector<cut_t const*> vcuts( fanin );

      _cut_network.incre_total_tuples(pairs);

      // insert best cut for cut generation
      _lcuts[0]->insert( _cut_network.get_best_cut(child0_index) );
      _lcuts[1]->insert( _cut_network.get_best_cut(child1_index) );

      for ( auto const& c1 : *_lcuts[0] )
      {
        for ( auto const& c2 : *_lcuts[1] )
        {
          if ( !c1->merge( *c2, new_cut, _ps->cut_enumeration_ps.cut_size ) )
          {
            continue;
          }

          if ( rcuts.is_dominated( new_cut ) )
          {
            continue;
          }

          if constexpr ( StoreFunction )
          {
            vcuts[0] = c1;
            vcuts[1] = c2;
            new_cut->func_id = compute_truth_table( index, vcuts, new_cut );
          }
          // compute cut data for new_cut
          if(gf_get_etm() ==  ETM_AREA)
          {
            new_cut->data.area  = cut_area_derefed(new_cut);
            new_cut->data.edge  = cut_edge_derefed(new_cut);
            new_cut->data.delay = cut_delay(new_cut);
          }
          else
          {
            new_cut->data.area  = cut_area_flow(new_cut);
            new_cut->data.edge  = cut_edge_flow(new_cut);
            new_cut->data.delay = cut_delay(new_cut);
          }

          rcuts.insert( new_cut );
        }
      }
      
      /* limit the maximum number of _cut_network, and reserve one position for trival cut */
      rcuts.limit( _ps->cut_enumeration_ps.cut_limit - 1 );

      _cut_network.incre_total_cuts(rcuts.size());
      /* add trival cut ,and it directlt add at the end of _cut_network */ 
      if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
      {
        _cut_network.add_unit_cut( index );
      }
    }

    uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
    {
      std::vector<kitty::dynamic_truth_table> tt( vcuts.size() );
      auto i = 0;

      for ( auto const& cut : vcuts )
      {
        assert( ( ( *cut )->func_id >> 1 ) <= _cut_network.truth_cache_size() );
        tt[i] = kitty::extend_to( _cut_network.at(( *cut )->func_id), res.size() );
        const auto supp = _cut_network.compute_truth_table_support( *cut, res );
        kitty::expand_inplace( tt[i], supp );
        ++i;
      }

      auto tt_res = _ntk.compute( _ntk.index_to_node( index ), tt.begin(), tt.end() );

      if ( _ps->cut_enumeration_ps.minimize_truth_table )
      {
        const auto support = kitty::min_base_inplace( tt_res );
        if ( support.size() != res.size() )
        {
          auto tt_res_shrink = shrink_to( tt_res, static_cast<unsigned>( support.size() ) );
          std::vector<uint32_t> leaves_before( res.begin(), res.end() );
          std::vector<uint32_t> leaves_after( support.size() );

          auto it_support = support.begin();
          auto it_leaves = leaves_after.begin();
          while ( it_support != support.end() )
          {
            *it_leaves++ = leaves_before[*it_support++];
          }
          res.set_leaves( leaves_after.begin(), leaves_after.end() );
          return _cut_network.insert_truth_table( tt_res_shrink );
        }
      }

      return _cut_network.insert_truth_table( tt_res );
    }


#pragma endregion Cut Enumeration

    ////////////////////////////////////////////////////
    //////         misc functions
    ////////////////////////////////////////////////////
    /**
     * @brief compute the topological order vector
     *  only comtain the gates, without constant, pis, pos
    */
    void topologize()
    {
      uint32_t i = 0 ;
      _ntk.clear_visited();
      _ntk.set_visited(0, 1u);
      _ntk.foreach_co([&](auto const& s){
        auto n = _ntk.get_node(s);
        // if( _ntk.is_ci(n) || _ntk.is_constant(n) )  return; // only contain the and nodes
        dfs_topo_sort_rec(n, i);
      });
      _ntk.clear_visited();
      _storage->topo_order_reverse.assign(_storage->topo_order.rbegin(), _storage->topo_order.rend());
    }

    /**
     * @brief the dfs-topo method
     */
    void dfs_topo_sort_rec(node_t n, uint32_t& index)
    {
      // if( _ntk.visited(n) || _ntk.is_ci(n) || _ntk.is_constant(n) )  return;
      if( _ntk.visited(n) )  return;
      
      if(_ntk.get_equiv_node(n) != AIG_NULL)
      {
        dfs_topo_sort_rec(_ntk.get_equiv_node(n), index);
      }

      _ntk.set_visited(n, 1u);
      auto child0 = _ntk.get_child0(n);
      auto child1 = _ntk.get_child1(n);
      dfs_topo_sort_rec(_ntk.get_node( child0 ), index);
      dfs_topo_sort_rec(_ntk.get_node( child1 ), index);

      _storage->topo_order[index++] = n;
    }

    void clear_refs()
    {
      std::fill(_storage->refs.begin(), _storage->refs.end(), 0u);
    }

    void reset_refs()
    {
      clear_refs();

      _ntk.foreach_node([&](auto const& n){
        auto cn0 = _ntk.get_node( _ntk.get_child0(n) );
        auto cn1 = _ntk.get_node( _ntk.get_child1(n) );
        ++_storage->refs[cn0];
        ++_storage->refs[cn1];
      });

      _ntk.foreach_po([&](auto const& s){
        auto ns = _ntk.get_node( s );
        ++_storage->refs[ns];
      });
    }

    /**
     * @brief equivalence checking for mapping
     * 
     * @return true 
     * @return false 
     */
    bool checking()
    {
      printf("Checking by mapped graph ing:\n");
      if (debug_mapped_aig_by_origin(_ntk))
      {
        std::cout << "\tequivalent" << std::endl;
        return true;
      }
      else
      {
        std::cout << "\tunequivalent" << std::endl;
        return false;
      }
    }

    void print_network()
    {
      printf("\033[0;32;40m Network-Information: \033[0m \n");
      printf("constant zero   : %d\n", 1);
      printf("primary inputs  : %d\n", _ntk.num_cis() );
      printf("and gates       : %d\n", _ntk.num_gates() );
      printf("primary outputs : %d\n", _ntk.num_cos() );
    }

    void print_params()
    {
      printf("\033[0;32;40m KLUT-Mapper-Params: \033[0m \n");
      printf("cut-size   : %d, default 4\n", _ps->cut_enumeration_ps.cut_size);
      printf("cut-limit  : %d, default 8\n", _ps->cut_enumeration_ps.cut_limit);
      printf("preprocess : %d, default 1\n", _ps->bPreprocess);
      printf("area       : %d, default 0\n", _ps->bArea);
      printf("edge       : %d, default 1\n", _ps->bEdge);
      printf("power      : %d, default 0\n", _ps->bPower);
      printf("fancy      : %d, default 0\n", _ps->bFancy);
      printf("flow-iters : %d, default 1\n", _ps->uFlowIters);
      printf("area-iters : %d, default 2\n", _ps->uAreaIters);
    }

    void print_storage()
    {
      printf("\033[0;32;40m KLUT-Mapper-Storage: \033[0m \n");
      printf("network size : %u\n", _ntk.size() );
      printf("current delay: %f\n", _storage->delay_current);
      printf("current area : %f\n",  _storage->area_current);
      printf("topo-vector size  : %lu\n", _storage->topo_order.size());
      printf("arrival_times size: %lu\n", _storage->arrival_times.size());
      printf("require_times size: %lu\n", _storage->require_times.size());
    }

    void print_choice()
    {
      printf("choice node : \n");
      uint i = 0;
      for(i = 0 ; i < _storage->topo_order.size(); ++i)
      {
        if( _ntk.get_equiv_node(_storage->topo_order[i]) != AIG_NULL && _ntk.is_repr(_storage->topo_order[i]) )
        {
          printf("the choice node of node %d: ", _ntk.node_to_index(_storage->topo_order[i]) );
          auto next_node = _ntk.get_equiv_node(_storage->topo_order[i]);
          while(next_node != AIG_NULL)
          {
            printf("%d(%d) \t", _ntk.phase(next_node) ^ _ntk.phase(_storage->topo_order[i]), _ntk.node_to_index(next_node));
            next_node = _ntk.get_equiv_node( next_node );
          }
          printf("\n");  
        }
      }
      printf("\n");
    }
  
  private:
    Ntk&                                  _ntk;
    std::shared_ptr<klut_storage>         _storage;
    std::shared_ptr<klut_mapping_params>  _ps;
    std::shared_ptr<klut_mapping_stats>   _st;

    network_cuts_t                        _cut_network;
    std::array<cut_set_t*, Ntk::max_fanin_size + 1> _lcuts; // tmp cuts for merge
};  // end class klut_mapping_impl

};  // end namespace detail

template<class Ntk, bool StoreFunction = false, typename CutData = iFPGA_NAMESPACE::general_cut_data>
mapping_qor_storage klut_mapping(Ntk& ntk, klut_mapping_params const& ps = {}, klut_mapping_stats* pst = nullptr )
{
  klut_mapping_stats st;
  iFPGA_NAMESPACE::detail::klut_mapping_impl<Ntk, StoreFunction, CutData> p(ntk, ps, st);
  p.run();
  if ( pst )
    *pst = st;
  return {p.get_best_delay(), p.get_best_area()};
}

iFPGA_NAMESPACE_HEADER_END