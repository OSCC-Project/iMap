/**
 * @file lut_mapping.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The FPGA klut-mapping implementation
 *        // mainly refer to wiremap
 * @version 0.1
 * @date 2021-06-15
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"
#include "utils/common_properties.hpp"
#include "cut/cut_enumeration.hpp"
#include "cut/general_cut_enumeration.hpp"
#include "cut/wiremap_cut_enumeration.hpp"

#include "views/mapping_view.hpp"
#include "techmap-lib/lut_cell_lib.hpp"
#include "detail/map_qor.hpp"
#include "utils/tic_toc.hpp"
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

struct klut_mapping_params
{
  klut_mapping_params()
  {
    cut_enumeration_ps.cut_size = 4;
    cut_enumeration_ps.cut_limit = 8;
  }
  iFPGA_NAMESPACE::cut_enumeration_params cut_enumeration_ps{};
  bool     bPreprocess{true};     // preprocessing
  bool     bArea{false};      // area-oriented mapping
  bool     bEdge{true};       // using edge
  bool     bPower{false};     // using power
  bool     bFancy{false};
  bool     bExpRed{true};         // expand or reduce of the best cuts
  bool     bTimeDriven{false};
  ETypeCmp sort_mode{ETC_DELAY};  // sort mode of the cuts
  bool     bUseLutLib{false};
  uint8_t  uGlobal_round{1};
  uint8_t  uLocal_round{2};
  int      iDelayTraget{-1};
  float    fAndDelay{1.0f};
  float    fAndArea{1.0f};
  float    fEpsilon{0.005f};
  bool     bDebug{false};
  bool     verbose{false};
  bool     very_verbose{false};
};

struct klut_mapping_stats
{
  float_t delay{0.0f};
  float_t area{0.0f};
  float_t edge_flow{0.0f};

  std::vector<std::string> round_stats{}; // the iteration stats
  void report() const
  {
    for(auto rs : round_stats)
    {
      std::cout << rs;
    }
    printf( "[i] Delay = %0.6f\n", delay );
    printf( "[i] Area  = %0.6f\n", area );
    printf( "[i] Edge  = %0.6f\n", edge_flow );
  }
};

/**
 * @brief the storegement of klut_mapping
 * 
 * @tparam Ntk 
 * @tparam  StoreFunction
 * @tparam CutData 
 */
template<class Ntk, bool StoreFunction , typename CutData = iFPGA_NAMESPACE::general_cut_data>
struct klut_mapping_storage
{
  klut_mapping_storage(uint32_t size)
    : topo_order(size, 0u),
      arrival_times(size, -1.0f),
      require_times(size, std::numeric_limits<float>::max())
  {
  }

  ///
  std::vector< node<Ntk> >    topo_order;                  // only contain the and gate topo-nodes

  ///
  std::vector<float>        arrival_times;
  std::vector<float>        require_times;
  // std::vector<float>        est_refs;

  // statistics
  float_t                   delay_final{0.0f};
  float_t                   area_final{0.0f};
  float_t                   delay_current{0.0f};           // current delay for the mapping
  float_t                   area_current{0.0f};

  float_t                   required_glo{0.0f};
  float_t                   area_glo{0.0f};
  float_t                   power_glo{0.0f};
  uint32_t                  net_size{0};                   // total net size of current mapping
};

template<typename CutData>
struct klut_mapping_update_cuts
{
  template<typename NetworkCuts, typename Ntk>
  static void apply( NetworkCuts const& cuts, Ntk const& _ntk )
  {
    (void)cuts;
    (void)_ntk;
  }
};

namespace detail
{

template<class Ntk, bool StoreFunction , typename CutData>
class klut_mapping_impl
{
  public:
    using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, StoreFunction, CutData>;
    using cut_enumeration_impl_t = iFPGA::detail::cut_enumeration_impl<Ntk, StoreFunction, CutData>;
    using cut_t = typename network_cuts_t::cut_t;

    klut_mapping_impl(Ntk& ntk, klut_mapping_params const& ps, klut_mapping_stats& st)
      : _ntk(ntk), _ps(ps), _st(st),
        _ntk_best(ntk),
        _fanins(_ntk.size(), 0u), 
        _fanouts(_ntk.size(), 1u),
        _map_refs(_ntk.size(), 0u),
        _max_cut_sizes(_ntk.size(), 0u),
        _delays(_ntk.size(), 0.0f),
        _levels(_ntk.size(), 0u),
        _edges(_ntk.size(), 0.0f),
        _areas(_ntk.size(), 0.0f),
        _area_flows(_ntk.size(), 0.0f),
        _edge_flows(_ntk.size(), 0.0f),
        _cut_network( ntk.size() ),
        _cut_enumeration_singleton(cut_enumeration_impl_t(ntk, ps.cut_enumeration_ps, _st_cut_enum, _cut_network))
    {
      _cut_enumeration_singleton.set_cmp_type(ETC_DELAY2);
      _cut_enumeration_singleton.run();
      _topo_order.reserve(_ntk.size());
    }

  public:
    /**
     * @brief the main process in mapping flow
     */
    void run()
    {
      initialize();
      topologize();
      mapping_rounds();
    }

    /**
     * @brief for QoR comparationS
     */
    float get_best_delay() const { return _delay_best; }
    float get_best_area()  const { return _area_best; }

  private:
    /**
     * @brief the main mapping rounds
     */
    void mapping_rounds()
    {
      tic_toc tt;
      /// delay oriented mapping
      if( !_ps.bArea )
      {
        // _cut_enumeration_singleton.set_cmp_type(ETC_DELAY);
        mapping_depth_oriented<true>(0);    //  delay

        //TODO: only little effect ≈ 0.01%
        // _cut_enumeration_singleton.set_cmp_type(ETC_DELAY2);
        mapping_depth_oriented<false>(1);   // delay2

        // _cut_enumeration_singleton.set_cmp_type(ETC_AREA);
        mapping_depth_oriented<false>(2);   // area

      }
      else
      {
        // _cut_enumeration_singleton.set_cmp_type(ETC_DELAY);
        mapping_depth_oriented<true>(0);
      }

      //TODO: no effect
      /// area_flow edge_flow oriented mapping
      for(uint32_t i = 0 ; i < _ps.uGlobal_round; ++i)
      {
        // _cut_enumeration_singleton.set_cmp_type(ETC_FLOW);
        global_area_edge_recovery();
      }

      /// local_area, local_edge oriented mapping
      for(uint32_t i = 0 ; i < _ps.uLocal_round; ++i)
      {
        // _cut_enumeration_singleton.set_cmp_type(ETC_AREA);
        local_area_edge_recovery();
      }

      std::cout << "Mapping time: " << tt.toc() << " secs"<< std::endl;
    }
    /**
     * @brief compute the member various for the start stage
     */
    void initialize()
    {
      if( _ps.bUseLutLib )
        init_lut_lib();
      
      _ntk.foreach_node( [this]( auto n ) {
        const auto index = _ntk.node_to_index( n );

        for(int cut_index = 0; cut_index < _cut_network.cuts(index).size(); ++cut_index)
        {
          _max_cut_sizes[index] = std::max(_max_cut_sizes[index], _cut_network.cuts(index)[cut_index].size());
        }
        _fanouts[index] = _ntk.fanout_size( n );
        _fanins[index]  = _ntk.fanin_size( n );
        update_node_stats(index);
      } );
    }

    /**
     * @brief compute the topological order vector
     */
    void topologize()
    {
      _ntk.clear_visited();
      _ntk.foreach_co([&](auto const& s){
        dfs_topo_sort(s);
      });
      _ntk.clear_visited();
    }

    /**
     * @brief the dfs-topo method
     */
    void dfs_topo_sort(typename Ntk::signal s)
    {
      auto n = _ntk.get_node(s);
      if(_fanins[n] == 0)
        return;
      if(!_ntk.visited(n))
      {
        _ntk.set_visited(n, 1u);
        _ntk.foreach_fanin(n, [&](auto const& s2){
          dfs_topo_sort(s2);  
        });
        _topo_order.emplace_back(n);
      }
      else
        return;
    }

    /**
     * @brief calculate the level of each node
     *  constant delay model
     *        constant node set to 0
     *        ci node set to 1
     *        level(node) = max(level(node), level(node's fanins) + 1)
     */
    void calc_levels()
    {
      _ntk.foreach_node( [this]( auto n ) {
        auto index = _ntk.node_to_index(n);
        if(_ntk.is_constant(n))
        {
          _levels[index] = 0u;
        }
        else if(_ntk.is_ci(n))
        {
          _levels[index] = 1u;
        }
        else
        {
          _ntk.foreach_fanin(n, [&](auto const& s){
            auto index_child = _ntk.node_to_index( _ntk.get_node(s) );
            _levels[index] = max(_levels[index], _levels[index_child]);
          });
          _levels[index]++;
        }
      });
    }

    /**
     * @brief init lut libs, index from 1 to 10 is useful
     *    same as the abc: read_lut command
     */
    void init_lut_lib()
    {
      _lut_cell.areas  = {1.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
      _lut_cell.delays = {1.0f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 512.0f, 1024.0f};
    }

#pragma region CUT delay, area_flow, edge_flow
    /**
     * @brief compute the a cut's delay
     */
    float_t cut_delay(cut_t const& cut) const
    {
      if(_ps.bUseLutLib)
      {
        return _lut_cell.delays[cut.size()];
      }
      else
      {
        float_t delay_cur, delay = -1.0f;
        for(auto leaf : cut)
        {
          delay_cur = _cut_network.cuts(leaf).best()->data.delay;
          delay = std::max(delay, delay_cur);
        }
        return delay + 1.0f;
      }
    }

    /**
     * @brief compute the a cut's area
     */
    float_t cut_area(cut_t const& cut) const
    {
      if(_ps.bUseLutLib)
      {
        return _lut_cell.areas[cut.size()];
      }
      else
      {
        return cut->data.area;
      }
    }

    /**
     * @brief compute the cut edge
     *  total number of fanin edges of the cut
     */
    float_t cut_edge(cut_t const& cut) const
    {
      float_t edge = cut.size();
      for(auto leaf : cut)
      {
        // if( _ntk.is_pi( leaf ) || _ntk.is_constant( leaf ) )
        //   continue;
        edge += _fanins[leaf];
      }
      return edge;
    }

    /**
     * @brief compute the area flow of a node,
     *    focus on the best cut
     */
    float_t cut_area_flow( const cut_t& cut )
    {
      float_t area_flow = cut_area( cut );
      float_t add_on = 0.0f;
      /* \! refer to wiremap paper
      for(auto leaf : cut)
      {
        area_flow += cut_area_flow(leaf) / _fanouts[leaf];
      }
      */
      // refer to abc/ifCut.c
      for(auto leaf : cut)
      {
        if( _map_refs[leaf] == 0 )
          add_on += cut_area( _cut_network.cuts(leaf).best() );
        else
          add_on += cut_area( _cut_network.cuts(leaf).best() ) / _fanouts[leaf];
        if(area_flow >= (float_t)1e32 || add_on >= (float_t)1e32)
          area_flow = (float_t)1e32;
        else
        {
          area_flow += add_on;
          if(area_flow >= (float_t)1e32)
            area_flow = (float_t)1e32;
        }
      }
      return area_flow;
    }

    /**
     * @brief compute edge flow of a cut
     *  focus on the best cut
     */
    float_t cut_edge_flow( const cut_t& cut )
    {
      float_t edge_flow = cut.size();
      float_t add_on = 0.0f;
      for(auto leaf : cut)
      {
        if( _map_refs[leaf] == 0 )
          add_on += cut_edge( _cut_network.cuts(leaf).best() );
        else
          add_on +=  cut_edge( _cut_network.cuts(leaf).best() ) / _fanouts[leaf];
        if(edge_flow >= (float_t)1e32 || add_on >= (float_t)1e32)
          edge_flow = (float)1e32;
        else
        {
          edge_flow += add_on;
          if(edge_flow >= (float_t)1e32)
            edge_flow = (float)1e32;
        }
      }
      return edge_flow;
    }

    /**
     * @brief compute the exact local area of a node
     *  focus on the cut_node in mffs_nodes
     * using the defered to replace this function
     */
    float_t cut_exact_local_area( uint32_t index, cut_t const& cut)
    {
      return cut_area_derefed(cut);
      // float_t ela = cut_area( cut );
      // mffc_view<Ntk> node_mffc(_ntk,  index);  // the node's mffc nodes
      // for(auto leaf : cut)
      // {
      //   if( std::find(node_mffc._nodes.begin(), node_mffc._nodes.end(), leaf) != node_mffc._nodes.end())
      //   {
      //     ela += cut_area( _cut_network.cuts(leaf).best() );
      //   }
      // }
      // return ela;
    }

    /**
     * @brief compute the exact local edge of a node
     *  focus on the cut_node in mffs_nodes
     * using the defered to replace this function
     */
    float_t cut_exact_local_edge(uint32_t index, cut_t const& cut)
    {
      return cut_edge_derefed(cut);
      // float_t ele = cut_edge( cut );
      // mffc_view<Ntk> node_mffc( _ntk, index );
      // for(auto leaf : cut)
      // {
      //   if( std::find(node_mffc._nodes.begin(), node_mffc._nodes.end(), leaf) != node_mffc._nodes.end())
      //   {
      //     ele += cut_edge( _cut_network.cuts(leaf).best() );
      //   }
      // }
      // return ele;
    }
#pragma end region CUT delay, area_flow, edge_flow

#pragma region CUT ref and deref for area/edge
    /**
     * @brief
     * @param limit, reduce the recursice
     * @return the estimated cut_delay
     */
    float_t cut_delay_deref(cut_t const& cut)
    {
      float_t delay = cut_delay(cut);
      for(auto leaf : cut)
      {
        if ( _ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        if(--_map_refs[leaf] == 0)
        {
          // _map_refs[leaf] == 0 && ( the node is an and gate)
          delay += cut_delay_deref( _cut_network.cuts(leaf).best());    
        }

      }
      return delay;
    }

    /**
     * @brief
     * @return the estimated cut_delay
     */
    float_t cut_delay_ref(cut_t const& cut)
    {
      float_t delay = cut_delay(cut);
      for(auto leaf : cut)
      {
        if (_ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        if(_map_refs[leaf]++ == 0)
        {
          // _map_refs[leaf] == 0 && ( the node is an and gate)
          delay += cut_delay_ref( _cut_network.cuts(leaf).best());
        }

      }
      return delay;
    }

    /**
     * @brief
     * @return the estimated cut_area
     */
    float_t cut_area_deref(cut_t const& cut)
    {
      float_t area = cut_area(cut);
      for(auto leaf : cut)
      {
        if (_ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        if(--_map_refs[leaf] == 0)
        {
          // _map_refs[leaf] == 0 && ( the node is an and gate)
          area += cut_area_deref(_cut_network.cuts(leaf).best());
        }
        
      }
      return area;
    }

    /**
     * @brief
     * @return the estimated cut_area
     */
    float_t cut_area_ref(cut_t const& cut)
    {
      float_t area = cut_area(cut);
      for(auto leaf : cut)
      {
        if (_ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        if( _map_refs[leaf]++ == 0)
        {
          // _map_refs[leaf] == 0 && ( the node is an and gate)
          area += cut_area_ref(_cut_network.cuts(leaf).best());   // not in map_refs, then add in
        }
      }
      return area;
    }

    /**
     * @brief
     *        using the limit to solve the time solve problem
     * @return
     */
    float_t cut_area_derefed(cut_t const& cut)
    {
      if(cut.size() < 2)
        return 0.0f;
      float_t res1, res2;
      res2 = cut_area_ref(cut);
      res1 = cut_area_deref(cut);
      // if(res1 <= res2 - 3*_ps.fEpsilon || res1 >= res2 + 3*_ps.fEpsilon)
      // {
      //   printf("cut_area_derefed:\n");
      //   printf("res1             : %f\n", res1);
      //   printf("res2             : %f\n", res2);
      //   printf("res2 - 3*_ps.fEpsilon: %f\n", res2 - 3*_ps.fEpsilon);
      //   printf("res2 + 3*_ps.fEpsilon: %f\n", res2 + 3*_ps.fEpsilon);
      // }
      // assert( res1 > res2 - 3*_ps.fEpsilon);
      // assert( res1 < res2 + 3*_ps.fEpsilon);
      return res1;
    }
    /**
     * @brief
     * @return
     */
    float_t cut_area_refed(cut_t const& cut, int limit)
    {
      if(cut.size() < 2)
        return 0.0f;
      float_t res1, res2;
      res1 = cut_area_deref(cut);
      res2 = cut_area_ref(cut);
      assert( res2 > res1 - _ps.fEpsilon);
      assert( res2 < res1 + _ps.fEpsilon);
      return res2;
    }
    /**
     * @brief
     * @return
     */
    float_t cut_edge_deref(cut_t const& cut)
    {
      float_t edge = cut.size();
      // if(limit == 0)
      //   return edge;
      for(auto leaf : cut)
      {
        if (--_map_refs[leaf] > 0 || _ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        // _map_refs[leaf] == 0 && ( the node is an and gate)
        edge += cut_edge_deref(_cut_network.cuts(leaf).best());
      }
      return edge;
    }
    /**
     * @brief
     * @return
     */
    float_t cut_edge_ref(cut_t const& cut)
    {
      float_t edge = cut.size();
      // if(limit == 0)
      //   return edge;
      for(auto leaf : cut)
      {
        if ( _map_refs[leaf]++ > 0 || _ntk.is_constant( leaf ) || _ntk.is_pi( leaf ) )
          continue;
        // _map_refs[leaf] == 0 && ( the node is an and gate)
        edge += cut_edge_ref(_cut_network.cuts(leaf).best());
      }
      return edge;
    }
    /**
     * @brief
     *        using the limit to solve the time solve problem
     * @return  
     */
    float_t cut_edge_derefed(cut_t const& cut)
    {
      //
      if(cut.size() < 2)
        return cut.size();
      float_t res1, res2;
      res2 = cut_edge_ref(cut);
      res1 = cut_edge_deref(cut);
      // if(res1 <= res2 - 3*_ps.fEpsilon || res1 >= res2 + 3*_ps.fEpsilon)
      // {
      //   printf("cut_edge_derefed:\n");
      //   printf("res1             : %f\n", res1);
      //   printf("res2             : %f\n", res2);
      //   printf("res2 - 3*_ps.fEpsilon: %f\n", res2 - 3*_ps.fEpsilon);
      //   printf("res2 + 3*_ps.fEpsilon: %f\n", res2 + 3*_ps.fEpsilon);
      // } 
      // assert( res1 > res2 - 3*_ps.fEpsilon);
      // assert( res1 < res2 + 3*_ps.fEpsilon);
      return res1;
    }
    /**
     * @brief
     * @return
     */
    float_t cut_edge_refed(cut_t const& cut)
    {
      if(cut.size() < 2)
        return cut.size();
      float_t res1, res2;
      res1 = cut_edge_deref(cut);
      res2 = cut_edge_ref(cut);
      assert( res2 > res1 - _ps.fEpsilon);
      assert( res2 < res1 + _ps.fEpsilon);
      return res2;
    }
#pragma endregion CUT ref and deref for area/edge

    /**
     * @brief update the node[index]'s delay,area_flow and edge_flow
     */
    void update_node_stats(uint32_t index)
    {
      _delays[index]     = cut_delay( _cut_network.cuts(index).best());
      _area_flows[index] = cut_area_flow( _cut_network.cuts(index).best());
      _edge_flows[index] = cut_edge_flow( _cut_network.cuts(index).best());
    }

    /**
     * @brief depth oriented cut selection
     * @param mode:
     *            0-Delay:  when depth is equal, then prefer the the cut with fewer leaves, then area-flow
     *            1-Delay2: when depth is equal, then prefer the cut with a smaller area-flow, then cut-leaves
     *            2-Area:   Area Flow, then fanin-refs(cuts with smaller average fanin reference counters), then depth
     */
    template<bool FIRST>
    void mapping_depth_oriented(int mode)
    {
      if(FIRST)           // mapping_depth_oriented<true>(0)
      {
        derive_final_mapping();
        update_best_network(   mapping_qor_storage{_current_delay, _current_area}, true );
        return;
      }
      else if(mode == 0)  // mapping_depth_oriented<false>(0)
      {
        // TODO: skip critical path, but bugs arise
        // depth_view<iFPGA_NAMESPACE::aig_network> depth_aig(_ntk);
        for(auto n : _topo_order )
        {
          // if( depth_aig.is_on_critical_path(n) )
          //   continue;
          auto index_node = _ntk.node_to_index(n);
          if(_ntk.is_constant(n) || _ntk.is_pi(n))
          {
            continue;
          }
          /// not using sort, just to find the best cut is enough!
          int best_cut_index{-1};
          int best_cut_size{-1};
          float best_area{std::numeric_limits<float>::max()};
          float best_area_flow{std::numeric_limits<float>::max()};
          float best_delay{std::numeric_limits<float>::max()};
          float best_edge{std::numeric_limits<float>::max()};
          //TODO: selected the most depthed cut
          for(int cut_index = 0; cut_index < _cut_network.cuts(index_node).size(); ++cut_index)
          {
            auto cut = _cut_network.cuts(index_node)[cut_index]; 
            if(cut.size() == 1)
            {
              continue;
            }
            if( _max_cut_sizes[index_node] - cut.size() < 2 &&
                (best_cut_index == -1 || best_delay > cut->data.delay + _ps.fEpsilon || best_cut_size < cut.size() ||
                  best_area > cut->data.area + _ps.fEpsilon || best_area_flow > cut->data.area_flow + _ps.fEpsilon ||
                  best_edge > cut->data.edge_flow + _ps.fEpsilon) 
              ) 
            {
              best_cut_index = cut_index;
              best_delay     = cut->data.delay;
              best_area      = cut->data.area;
              best_edge      = cut->data.edge_flow;
              best_area_flow = cut->data.area_flow;
              best_cut_size  = cut.size();
            }
          }
          if(best_cut_index == -1)
          {
            return;
          }
          _map_refs[index_node] = 0;
          if(best_cut_index != 0)
          {
            _cut_network.cuts(index_node).update_best(best_cut_index);
            update_node_stats(index_node);
          }
        }
        derive_final_mapping();
        update_best_network(   mapping_qor_storage{_current_delay, _current_area} );
      }
      else if(mode == 1)  // mapping_depth_oriented<false>(1)
      {
        for(auto n : _topo_order )
        {
          auto index_node = _ntk.node_to_index(n);
          if(_ntk.is_constant(n) || _ntk.is_pi(n))
          {
            continue;
          }
          int best_cut_index{-1};
          int best_cut_size{-1};
          float best_area_flow{std::numeric_limits<float>::max()};
          float best_delay{std::numeric_limits<float>::max()};
          float best_edge_flow{std::numeric_limits<float>::max()};
          for(int cut_index = 0; cut_index < _cut_network.cuts(index_node).size(); ++cut_index)
          {
            auto cut = _cut_network.cuts(index_node)[cut_index]; 
            if(cut.size() == 1)
            {
              continue;
            }
            float tmp_area_flow = cut_area_flow(cut);
            float tmp_edge_flow = cut_edge_flow(cut);
            if( _max_cut_sizes[index_node] - cut.size() < 2 &&
                (best_cut_index == -1 || best_delay > cut->data.delay + _ps.fEpsilon || best_cut_size < cut.size() ||
                best_area_flow > tmp_area_flow + _ps.fEpsilon || best_edge_flow > tmp_edge_flow + _ps.fEpsilon)
              )
            {
              best_cut_index = cut_index;
              best_delay     = cut->data.delay;
              best_edge_flow = tmp_edge_flow;
              best_area_flow = tmp_area_flow;
              best_cut_size  = cut.size();
            }
          }
          if(best_cut_index == -1)
          {
            return;
          }
          _map_refs[index_node] = 0;
          if(best_cut_index != 0)
          {
            _cut_network.cuts(index_node).update_best(best_cut_index);
            update_node_stats(index_node);
          }
        }
        derive_final_mapping();
        update_best_network(   mapping_qor_storage{_current_delay, _current_area} );
      } 
      else                // mapping_depth_oriented<false>(2)   area
      {
        for(auto n : _topo_order )
        {
          auto index_node = _ntk.node_to_index(n);
          if(_ntk.is_constant(n) || _ntk.is_pi(n))
          {
            continue;
          }
          int best_cut_index{-1};
          int best_cut_size{-1};
          float best_area{std::numeric_limits<float>::max()};
          float best_area_flow{std::numeric_limits<float>::max()};
          float best_delay{std::numeric_limits<float>::max()};
          float best_edge{std::numeric_limits<float>::max()};
          for(int cut_index = 0; cut_index < _cut_network.cuts(index_node).size(); ++cut_index)
          {
            auto cut = _cut_network.cuts(index_node)[cut_index]; 
            if(cut.size() == 1)
            {
              continue;
            }
            float tmp_area_flow = cut_area_flow(cut);
            float tmp_edge_flow = cut_edge_flow(cut);
            if( _max_cut_sizes[index_node] - cut.size() < 2 &&
                (best_cut_index == -1 ||  
                  //  best_area_flow > tmp_area_flow + _ps.fEpsilon || best_edge_flow > tmp_edge_flow + _ps.fEpsilon ||
                  best_area > cut->data.area + _ps.fEpsilon || best_edge > cut->data.edge_flow + _ps.fEpsilon ||          // using this, the area will lower than the flows
                  best_cut_size < cut.size() || best_delay > cut->data.delay + _ps.fEpsilon)
              )
            {
              best_cut_index = cut_index;
              best_delay     = cut->data.delay;
              // best_edge_flow = tmp_edge_flow;
              // best_area_flow = tmp_area_flow;
              best_edge = cut->data.edge_flow;
              best_area = cut->data.area;
              best_cut_size  = cut.size();
            }
          }
          if(best_cut_index == -1)
          {
            return;
          }
          _map_refs[index_node] = 0;
          if(best_cut_index != 0)
          {
            _cut_network.cuts(index_node).update_best(best_cut_index);
            update_node_stats(index_node);
          }
        }
        derive_final_mapping();
        update_best_network(   mapping_qor_storage{_current_delay, _current_area} );
      }
      _cut_enumeration_singleton.run();
    }

    /**
     * @brief global area edge recorvery
     *  wire-map
     * @result the cuts.best() will be the selected result
     */
    void global_area_edge_recovery()
    {
      for(auto n : _topo_order )
      {
        auto index_node = _ntk.node_to_index(n);
        if(_ntk.is_constant(n) || _ntk.is_pi(n))
        {
          continue;
        }
        int best_cut_index{-1};
        int best_cut_size{-1};
        float best_area{std::numeric_limits<float>::max()};
        float best_area_flow{std::numeric_limits<float>::max()};
        float best_delay{std::numeric_limits<float>::max()};
        float best_edge{std::numeric_limits<float>::max()};
        float best_edge_flow{std::numeric_limits<float>::max()};

        if(_map_refs[index_node] > 0)
        {
          cut_area_deref( _cut_network.cuts(index_node).best());
        }

        for(int cut_index = 0; cut_index < _cut_network.cuts(index_node).size(); ++cut_index)
        {
          auto cut = _cut_network.cuts(index_node)[cut_index]; 
          if(cut.size() == 1)
          {
            continue;
          }
          float tmp_area_flow = cut_area_flow(cut);
          float tmp_edge_flow = cut_edge_flow(cut);
          if( _max_cut_sizes[index_node] - cut.size() < 2 &&
              (best_cut_index == -1 ||  
                best_area_flow > tmp_area_flow + _ps.fEpsilon || best_edge_flow > tmp_edge_flow + _ps.fEpsilon ||
                //  best_area > cut->data.area + _ps.fEpsilon || best_edge > cut->data.edge_flow + _ps.fEpsilon ||          // using this, the area will lower than the flows
                best_cut_size < cut.size() || best_delay > cut->data.delay + _ps.fEpsilon)
            )
          {
            best_cut_index = cut_index;
            best_delay     = cut->data.delay;
            // best_edge_flow = tmp_edge_flow;
            // best_area_flow = tmp_area_flow;
            best_edge = cut->data.edge_flow;
            best_area = cut->data.area;
            best_cut_size  = cut.size();
          }
        }
        if(best_cut_index == -1)
        {
          return;
        }
        if(best_cut_index != 0)
        {
          _cut_network.cuts(index_node).update_best(best_cut_index);
          update_node_stats(index_node);
        }
        if(_map_refs[index_node] > 0)
        {
          cut_area_ref( _cut_network.cuts(index_node).best());
        }
      }
      derive_final_mapping();
      update_best_network(   mapping_qor_storage{_current_delay, _current_area} );
      _cut_enumeration_singleton.run();
    }

    /**
     * @brief local area_edge recovery
     * wire-map
     * @result the cuts.best() will be the selected result
     */
    void local_area_edge_recovery()
    {
      for(auto n : _topo_order )
      {
        auto index_node = _ntk.node_to_index(n);
        if(_ntk.is_constant(n) || _ntk.is_pi(n))
        {
          continue;
        }
        int best_cut_index{-1};
        int best_cut_size{-1};
        float best_exact_area{std::numeric_limits<float>::max()};
        float best_delay{std::numeric_limits<float>::max()};
        float best_exact_edge{std::numeric_limits<float>::max()};

        if(_map_refs[index_node] > 0)
        {
          cut_area_deref( _cut_network.cuts(index_node).best());
        }

        for(int cut_index = 0; cut_index < _cut_network.cuts(index_node).size(); ++cut_index)
        {
          auto cut = _cut_network.cuts(index_node)[cut_index]; 
          if(cut.size() == 1)
          {
            continue;
          }
          float tmp_exact_area = cut_area_derefed(cut);
          float tmp_exact_edge = cut_edge_derefed(cut);
          if( _max_cut_sizes[index_node] - cut.size() < 2 &&
              (best_cut_index == -1 ||  
                best_exact_area > tmp_exact_area + _ps.fEpsilon || best_exact_edge > tmp_exact_edge + _ps.fEpsilon ||
                best_cut_size < cut.size() || best_delay > cut->data.delay + _ps.fEpsilon)
            )
          {
            best_cut_index = cut_index;
            best_delay     = cut->data.delay;
            best_exact_edge = tmp_exact_edge;
            best_exact_area = tmp_exact_area;

            best_cut_size  = cut.size();
          }
        }
        if(best_cut_index == -1)
        {
          return;
        }
        if(best_cut_index != 0)
        {
          _cut_network.cuts(index_node).update_best(best_cut_index);
          update_node_stats(index_node);
        }
        if(_map_refs[index_node] > 0)
        {
          cut_area_ref( _cut_network.cuts(index_node).best());
        }
      }
      derive_final_mapping();
      update_best_network(   mapping_qor_storage{_current_delay, _current_area} );
      _cut_enumeration_singleton.run();
    }

    /**
     * @brief update the best network insider
     */
    void update_best_network(  mapping_qor_storage qor, bool first = false)
    {
      if(first)
      {
        _ntk_best   = _ntk;
        _delay_best = _current_delay;
        _area_best  = _current_area;
      }
      else
      {
        if(qor.area*qor.delay < _delay_best*_area_best - _ps.fEpsilon)
        {
          _ntk_best   = _ntk;
          _delay_best = _current_delay;
          _area_best  = _current_area;
        }
        else if(qor.area*qor.delay >= _delay_best*_area_best- _ps.fEpsilon && qor.area*qor.delay <= _delay_best*_area_best + _ps.fEpsilon)
        {
          if(qor.delay < _delay_best  - _ps.fEpsilon)
          {
            _ntk_best   = _ntk;
            _delay_best = _current_delay;
            _area_best  = _current_area;
          }
        }
      }
    }

     /**
     * @brief perform best cut selection for mapped node
     *  select best cut according the _map_ref vector
     */
    void derive_final_mapping()
    {
      _current_area  = 0.0f;
      _current_delay = 0.0f;
      //_ntk.clear_mapping();
      _ntk = decltype(_ntk)(typename decltype(_ntk)::base_type(_ntk)); // reconstruct a new one since _ntk may have been bound with _ntk_best
      std::fill(_map_refs.begin(), _map_refs.end(), 0);

      _ntk.foreach_po( [this]( auto s){
        const auto index = _ntk.node_to_index( _ntk.get_node(s));
        _current_delay = std::max( _current_delay, _delays[index]);   // compute current delay
        ++_map_refs[index];
      });

      for(auto it = _topo_order.rbegin(); it != _topo_order.rend(); ++it)
      {
        if( _ntk.is_constant(*it) || _ntk.is_pi(*it) )
        {
          continue;
        }  
        const auto index = _ntk.node_to_index( *it );
        if( _map_refs[index] == 0)
        {
          continue;
        }
        std::vector< node<Ntk> > nodes;
        for( auto leaf : _cut_network.cuts(index).best() )
        {
          ++_map_refs[leaf];
          nodes.emplace_back( leaf );
        }
        ++_current_area;                                              // compute current area
        _ntk.add_to_mapping( *it, nodes.begin(), nodes.end() );

        if constexpr ( StoreFunction)
        {
          _ntk.set_cell_function( *it, _cut_network.truth_table( _cut_network.cuts( index ).best() ) );
        }
      }
    }

  private:
    /// input datas
    Ntk                         _ntk;
    klut_mapping_params const&  _ps;
    klut_mapping_stats          _st;
    lut_cell                    _lut_cell;          // the lut lib, include lut name,area,delay and etc
    
    cut_enumeration_stats     _st_cut_enum;
    cut_enumeration_impl_t    _cut_enumeration_singleton;
    network_cuts_t            _cut_network;

    std::vector< node<Ntk> >    _topo_order;
    std::vector<uint32_t>     _fanins;
    std::vector<uint32_t>     _fanouts;   
    std::vector<uint32_t>     _max_cut_sizes;       // store the every node's cut_set's max_leaves_size
    std::vector<uint32_t>     _map_refs;            // ref counters, a node as a leaf of the best cut of other cut
    /// the wiremap FPGA lut mapping params required
    std::vector<float_t>      _edge_flows;
    std::vector<float_t>      _area_flows;
    std::vector<float_t>      _delays;
    std::vector<uint32_t>     _levels;              // FPGA const delay model
    std::vector<float_t>      _edges;
    std::vector<float_t>      _areas;
    /// variables for result data
    Ntk&                      _ntk_best;
    float_t                   _delay_best{0.0f};
    float_t                   _area_best{0.0f};
    float_t                   _current_delay{0.0f}; // current delay for the mapping
    float_t                   _current_area{0.0f};
};  // end class klut_mapping_impl


template<class Ntk, bool StoreFunction, typename CutData = iFPGA_NAMESPACE::general_cut_data>
class klut_mapping_impl2
{
  public:
    using network_cuts_t = iFPGA_NAMESPACE::network_cuts<Ntk, StoreFunction, CutData>;
    using cut_enumeration_impl_t = iFPGA::detail::cut_enumeration_impl<Ntk, StoreFunction, CutData>;
    using cut_t = typename network_cuts_t::cut_t;
    using cut_set_t = typename network_cuts_t::cut_set_t;
    using klut_storage = klut_mapping_storage<Ntk, StoreFunction, CutData>;
    using node_t = typename Ntk::node;
    static constexpr node_t AIG_NULL = Ntk::AIG_NULL;

    /**
     * @brief Construct a new klut mapping impl2 object
     * 
     * @param ntk 
     * @param ps 
     * @param st 
     * @param choice the choice information
     */
    klut_mapping_impl2(Ntk& ntk, klut_mapping_params const& ps, klut_mapping_stats const& st)
      : _storage( std::make_shared<klut_storage>(ntk.size()) ),
        _ps(std::make_shared<klut_mapping_params>(ps)),
        _st(std::make_shared<klut_mapping_stats>(st)),
        _ntk(ntk),
        _cut_network(ntk.size()),
        _cut_enumeration_singleton( cut_enumeration_impl_t(ntk, ps.cut_enumeration_ps, _cut_enumeration_st, _cut_network) ),
        _best_cuts(ntk.size())
    {

    }

  public:
    /**
     * @brief the main process in mapping flow
     */
    void run()
    {
      init_parameters();
      init_storage();

      // print the initialization informations
      if(_ps->verbose)
      {
        print_network();
        print_params();
        print_storage();
        _cut_network.print_storage();
      }
      
      // mapping body
      perform_mapping();  
    }

    /**
     * @brief public function for QoR call on
     */
    float get_best_delay() const { return _storage->delay_final; }
    float get_best_area()  const { return _storage->area_final; }

    /// for test
    auto get_topo_order() const
    {
      return _storage->topo_order;
    }


  private:
    /**
     * @brief initialize the parameters to default value
     */
    void init_parameters()
    {
      _ps->bPreprocess   = true;    
      _ps->bArea         = false;  
      _ps->bEdge         = true;  
      _ps->bPower         = false;  
      _ps->bFancy        = false;
      _ps->bExpRed       = true;    
      _ps->bTimeDriven   = false;
      _ps->sort_mode     = ETC_DELAY;
      _ps->bUseLutLib    = false;
      _ps->uGlobal_round    = 1;
      _ps->uLocal_round    = 2;
      _ps->iDelayTraget  = -1;
      _ps->fEpsilon      = 0.005f;
      _ps->verbose       = false;
    }
    
    /**
     * @brief initizlize the sotratge's value's size and default value
     * 
     */
    void init_storage()
    {
      assert( (_ntk.size() - _ntk.num_cis() - 1) == _ntk.num_gates() );
      _storage->topo_order.resize( _ntk.size() - _ntk.num_cis() - 1 );
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
      printf("expend     : %d, default 1\n", _ps->bExpRed);
      printf("flow-iters : %d, default 1\n", _ps->uGlobal_round);
      printf("area-iters : %d, default 2\n", _ps->uLocal_round);
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

    /**
     * @brief the mapping algorithms body
     * 
     */
    void perform_mapping()
    {
      uint32_t i;
      // step1: create the CI cut_set, can skip becasue of the cut enumeration will process this step
      // step2: get the topo order:
      topologize();

      if (_ps->verbose)
      {
        printf("choice node : \n");
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

      tic_toc tt;

      // step3: perform mapping on combinational circuit
      _cut_network.add_zero_cut( 0 );
      _cut_network.set_ref(0, 1u);
      _storage->arrival_times[0] = 0.0f;

      _ntk.foreach_ci([&](uint32_t n){
        _storage->arrival_times[n] = 0.0f;
        _cut_network.set_ref(n, 1u);
        _cut_network.add_unit_cut( n );
      });

      // delay oriented mapping with preprocess
      if(_ps->bPreprocess && !_ps->bArea)
      {
        // mapping for delay
        perform_mapping_round(ETC_DELAY, 1, 1, true); // delay
        
        // mapping for delay second option
        _ps->bFancy = 1;
        clear_refs();
        perform_mapping_round(ETC_DELAY, 1, 0, true); // delay-2
        
        // mapping for area
        _ps->bFancy = 0;
        _ps->bArea  = 1;
        clear_refs();
        perform_mapping_round(ETC_DELAY, 1, 0, true); // area
        _ps->bArea  = 0;
      }
      // delay oriented mapping without preprocess
      else
      {
        perform_mapping_round(ETC_DELAY, 0, 1, true);
      }

      // area_flow oriented mapping
      for(i = 0 ; i < _ps->uGlobal_round; ++i)
      {
        perform_mapping_round(ETC_FLOW, 0 , 0, true);
      }
      
      // area oriented mapping
      for(i = 0 ; i < _ps->uLocal_round; ++i)
      {
        // perform_mapping_round(ETC_AREA, 0 , 0, (i == _ps->uLocal_round-1) );
        perform_mapping_round(ETC_AREA, 0 , 0, true);
      }

      std::cout << "Mapping time: " << tt.toc() << " secs"<< std::endl;
      // get the mapped network from the flow!
      derive_final_mapping();

      if(_ps->bDebug)
      {
        printf("Checking by mapped graph ing:\n");
        if (debug_mapped_aig_by_origin(_ntk))
        {
          std::cout << "\tequivalent" << std::endl;
        }
        else
        {
          std::cout << "\tunequivalent" << std::endl;
        }
        // printf("Checking by converted miter ing:\n");
        // if (debug_mapped_aig_by_converted_miter(_ntk))
        // {
        //   std::cout << "\tequivalent" << std::endl;
        // }
        // else
        // {
        //   std::cout << "\tunequivalent" << std::endl;
        // }
      }
    }

    /**
     * @brief the mapping rounds in every step
     * 
     * @param mode        ETC_DELAY, ETC_FLOW, ETC_AREA   
     * @param preprocess  toogles for preprocess
     * @param first       mark the first mapping round
     */
    void perform_mapping_round(ETypeCmp const& mode, bool preprocess, bool first, bool truth = false)
    {
      uint32_t i{0u};
      if(mode != ETC_DELAY || _ps->bArea)
      {
        _ps->sort_mode = ETC_FLOW;
        gf_set_etc(ETC_FLOW);
      }
      else if( _ps->bFancy)
      {
        _ps->sort_mode = ETC_AREA;
        gf_set_etc(ETC_AREA);
      }
      else
      {
        _ps->sort_mode = ETC_DELAY;
        gf_set_etc(ETC_DELAY);
      }

      // standard mapping steps for each node
      _ntk.clear_visited();
      for(i = 0 ; i < _storage->topo_order.size(); ++i)
      {
        perform_mapping_and(_storage->topo_order[i], mode, preprocess, first, truth);
        if( _ntk.is_repr(_storage->topo_order[i]) )
        {
          perform_mapping_and_choice(_storage->topo_order[i], mode, preprocess);
        }
      }

      _ntk.clear_visited();

      compute_required_times();
    }

    /**
     * @brief perform mapping for each and gate
     *    1. finds the best cut for the given node
     * @param node the node in the network _ntk
     * @param mode 
     * @param preprocess 
     * @param first 
     */
    void perform_mapping_and(node_t const& node, ETypeCmp const& mode, bool preprocess, bool first, bool truth = false)
    {
      // deref the best cut
      if(mode != ETC_DELAY && _cut_network.ref_at(node) > 0)
      {
        _cut_network.cut_area_deref(_cut_network.cuts(node).best(), _ntk , node);
      }


      // save the best cut from the previous iteration to particate the new cut_set's generation
      if(!first)
      {
        auto best_cut =  _cut_network.cuts(node).best();
        if(!preprocess || best_cut.size() <= 1)
        {
          _cut_network.push_2_capsule(node, best_cut);
        }
      }

      // generate cuts
      _cut_enumeration_singleton.merge_cuts2(node, truth);

      // update the best cut without increasing the delay, trival cut can not be the best cut of the node itself
      if( _cut_network.cuts(node).best().size() > 1 && (!preprocess ||  _cut_network.cuts(node).best()->data.delay <=  _storage->require_times[node] + _ps->fEpsilon) )
      {
        if( _best_cuts[node] < _cut_network.cuts(node).best())
        {
          _best_cuts[node] = _cut_network.cuts(node).best();
          _storage->arrival_times[node] = _cut_network.cuts( node ).best()->data.delay;
        }
      }

      // ref the best cut
      if(mode != ETC_DELAY && _cut_network.ref_at(node) > 0)
      {
        _cut_network.cut_area_ref(_cut_network.cuts(node).best(), _ntk , node);
      }

      return;
    }

    /**
     * @brief perform mapping for the choice node
     * 
     * @param node the representation node of this equivalent class
     * @param mode 
     * @param preprocess 
     */
    void perform_mapping_and_choice(node_t const& node, ETypeCmp const& mode, bool preprocess)
    {
      // deref the best cut
      if(mode != ETC_DELAY && _cut_network.ref_at(node) > 0)
      {
        _cut_network.cut_area_deref(_cut_network.cuts(node).best(), _ntk , node);
      }

      auto& cuts_repr = _cut_network.cuts(node);
      auto next_choice_node = _ntk.get_equiv_node(node);

      while(next_choice_node != AIG_NULL)
      {
        if( _cut_network.cuts(next_choice_node).size() ==0 )
        {
          next_choice_node = _ntk.get_equiv_node(next_choice_node);
          continue;
        }
        auto cut_phase = _ntk.phase(node) ^ _ntk.phase(next_choice_node);
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

        _cut_network.cuts( node ).limit( _ps->cut_enumeration_ps.cut_limit-1);

        next_choice_node = _ntk.get_equiv_node(next_choice_node);
      }

      // update the best cut without increasing the delay
      if( _cut_network.cuts(node).best().size() > 1 && (!preprocess ||  _cut_network.cuts(node).best()->data.delay <=  _storage->require_times[node] + _ps->fEpsilon) )
      {
        if( _best_cuts[node] < _cut_network.cuts(node).best())
        {
          _best_cuts[node] = _cut_network.cuts(node).best();
          _storage->arrival_times[node] = _cut_network.cuts( node ).best()->data.delay;
        }
      }

      // after insert the choice cuts, the trival cut will eliminates
      _cut_network.add_unit_cut( node );

      // ref the best cut
      if(mode != ETC_DELAY && _cut_network.ref_at(node) > 0)
      {
        _cut_network.cut_area_ref(_cut_network.cuts(node).best(), _ntk , node);
      }

      return;
    }

    /**
     * @brief compute require_times of current network
     */
    void compute_required_times()
    {
      // step1 computes area, references and nodes used in the mapping!
      std::fill(_storage->require_times.begin(), _storage->require_times.end(), std::numeric_limits<float>::max());
      _storage->area_glo = 0.0f;
      _storage->required_glo = 0.0f;
      _storage->power_glo = 0.0f;
      _storage->net_size = 0u;
      clear_refs();

      // compute the global area and global required time
      _ntk.foreach_co( [&]( auto s){
        auto node = _ntk.get_node(s);
        _storage->area_glo += mark_mapping_rec( node );
        if( _storage->required_glo < _storage->arrival_times[node] )
          _storage->required_glo = _storage->arrival_times[node];
      });

      if( _ps->bArea )
        return;
      
      // update the required time for POs by the global required time
      _ntk.foreach_co([&]( auto s){
        _storage->require_times[  _ntk.get_node(s) ] = _storage->required_glo;
      });

      // propagate required times from POs to PIs
      _cut_network.set_ref(0, 0);
      for(auto it = _storage->topo_order.rbegin(); it != _storage->topo_order.rend(); ++it)
      {
        auto anode = *it;
        if( _cut_network.ref_at(anode) == 0 )
          continue;
        auto required = _storage->require_times[ anode ];
        for( auto leaf : _cut_network.cuts( anode ).best() )
        {
          _storage->require_times[ leaf ] = std::min( _storage->require_times[ leaf ],  required - 1.0f);
        }
      }
      return;
    }

    /**
     * @brief compute the 
     * 
     * @param node 
     * @return float 
     */ 
    float mark_mapping_rec(const node_t& node)
    {
      float area{0.0f};
      if( _cut_network.ref_at(node)++ || _ntk.is_ci(node) || _ntk.is_constant(node) )
      {
        return 0.0f;
      }

      auto& best_cut = _cut_network.cuts( node ).best();
      _storage->net_size += best_cut.size();
      area = _cut_network.cut_lut_area(best_cut, _ntk, node);
      for(auto& leaf : best_cut)
      {
        area += mark_mapping_rec(leaf);
      }
      return area;
    }

    /**
     * @brief 
     * 
     * @param node 
     */
    void mark_ref_rec(const node_t& node)
    {
      if( _cut_network.ref_at(node)++ || _ntk.is_ci(node) || _ntk.is_constant(node) )
      {
        return;
      }
      _cut_network.incr_ref(node);
      for(auto& leaf : _best_cuts[node])
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
        const auto node = _ntk.get_node(s);
        tmp_delay = std::max( tmp_delay, _storage->arrival_times[node]);   // compute current delay
        mark_ref_rec(node);
      });

      for(auto it = _storage->topo_order.rbegin(); it != _storage->topo_order.rend(); ++it)
      {
        auto node = *it;
        if( _cut_network.ref_at(node) == 0)
        {
          continue;
        }
        assert(_best_cuts[node].size() > 1);  // assert the trivial cut

        std::vector< node_t > nodes;
        for( auto leaf : _best_cuts[node] )
        {
          nodes.emplace_back( leaf );
        }
        ++tmp_area;                                              // compute current area
        _ntk.add_to_mapping( node, nodes.begin(), nodes.end() );

        if constexpr ( StoreFunction)
        {
          _ntk.set_cell_function( node, _cut_network.truth_table( _best_cuts[node] ));
        }
      }
      _storage->area_final =  _storage->area_current = tmp_area;
      _storage->delay_final = _storage->delay_current = tmp_delay;
      return;
    }

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
        auto node = _ntk.get_node(s);
        if( _ntk.is_ci(node) || _ntk.is_constant(node) )  return; // only contain the and nodes
        dfs_topo_sort_rec(node, i);
      });
      _ntk.clear_visited();
    }

    /**
     * @brief the dfs-topo method
     */
    void dfs_topo_sort_rec(node_t node, uint32_t& index)
    {
      if( _ntk.visited(node) || _ntk.is_ci(node) || _ntk.is_constant(node) )  return;
      
      if(_ntk.get_equiv_node(node) != AIG_NULL)
      {
        dfs_topo_sort_rec(_ntk.get_equiv_node(node), index);
      }

      _ntk.set_visited(node, 1u);
      auto child0 = _ntk.get_child0(node);
      auto child1 = _ntk.get_child1(node);
      dfs_topo_sort_rec(_ntk.get_node( child0 ), index);
      dfs_topo_sort_rec(_ntk.get_node( child1 ), index);

      _storage->topo_order[index++] = node;
    }

    /**
     * @brief reset the node's ref be original value
     * 
     */
    void clear_refs()
    {
      _cut_network.clear_refs();
    }

  private:
    template<typename _Ntk, bool _ComputeTruth, typename _CutData>
    friend class detail::cut_enumeration_impl;

    template<typename _Ntk, bool _ComputeTruth, typename _CutData>
    friend network_cuts<_Ntk, _ComputeTruth, _CutData> cut_enumeration( _Ntk const& ntk, cut_enumeration_params const& ps, cut_enumeration_stats * pst );
  
  private:
    std::shared_ptr<klut_storage>         _storage;
    std::shared_ptr<klut_mapping_params>  _ps;
    std::shared_ptr<klut_mapping_stats>   _st;

    Ntk&                                  _ntk;

    network_cuts_t                        _cut_network;
    cut_enumeration_stats                 _cut_enumeration_st;
    cut_enumeration_impl_t                _cut_enumeration_singleton;
    std::vector<cut_t>                    _best_cuts;                 // storage the best cut of each node
};  // end class klut_mapping_impl2

};  // end namespace detail


template<class Ntk, bool StoreFunction = false, typename CutData = iFPGA_NAMESPACE::general_cut_data>
mapping_qor_storage klut_mapping(Ntk& ntk, klut_mapping_params const& ps = {}, klut_mapping_stats* pst = nullptr )
{
  klut_mapping_stats st;
  // iFPGA_NAMESPACE::detail::klut_mapping_impl<Ntk, StoreFunction, iFPGA_NAMESPACE::cut_enumeration_wiremap_cut> p(ntk, ps, st);
  iFPGA_NAMESPACE::detail::klut_mapping_impl2<Ntk, StoreFunction, CutData> p(ntk, ps, st);
  p.run();
  if ( ps.verbose )    
    st.report();
  if ( pst )
    *pst = st;
  return {p.get_best_delay(), p.get_best_area()};
}

iFPGA_NAMESPACE_HEADER_END