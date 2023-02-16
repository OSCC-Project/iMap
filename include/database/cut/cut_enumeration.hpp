/**
 * @file cut_enumeration.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The cut enuemration (k-feasible-cut) implementation
 * 
 * @version 0.1
 * @date 2021-06-04
 * @copyright Copyright (c) 2021
 */

#pragma once
#include "utils/ifpga_namespaces.hpp"
#include "utils/common_properties.hpp"
#include "cut/cut.hpp"
#include "cut/cut_set.hpp"
#include "cut/detail/cut_data.hpp"
#include "utils/truth_table_cache.hpp"

#include "kitty/constructors.hpp"
#include "kitty/dynamic_truth_table.hpp"
#include "kitty/operations.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

iFPGA_NAMESPACE_HEADER_START

/*! \brief Parameters for cut_enumeration.
 *
 * The data structure `cut_enumeration_params` holds configurable parameters
 * with default arguments for `cut_enumeration`.
 */
struct cut_enumeration_params
{
  /*! \brief Maximum number of leaves for a cut. */
  uint32_t cut_size{4u};

  /*! \brief Maximum number of cuts for a node. */
  uint32_t cut_limit{25u};

  /*! \brief Maximum number of fan-ins for a node. */
  uint32_t fanin_limit{10u};

  /*! \brief Prune cuts by removing don't cares. */
  bool minimize_truth_table{false};

  /*! \brief Be verbose. */
  bool verbose{false};

  /*! \brief Be very verbose. */
  bool very_verbose{false};
};

struct cut_enumeration_stats
{
  /*! \brief Total time. */
  double time_total{0};

  /*! \brief Time for truth table computation. */
  double time_truth_table{0};

  /*! \brief Prints report. */
  void report() const
  {
    std::cout << printf( "[i] total time       = %0.6f secs\n",  time_total  );
    std::cout << printf( "[i] truth table time = %0.6f secs\n",  time_truth_table  );
  }
};


static constexpr uint32_t max_cut_size = 7;

template<bool ComputeTruth, typename T = empty_cut_data>
struct cut_data;

template<typename T>
struct cut_data<true, T>
{
  uint32_t func_id{0u};
  T data;
};

template<typename T>
struct cut_data<false, T>
{
  T data;
};

template<bool ComputeTruth, typename T>
using cut_type = cut<max_cut_size, cut_data<ComputeTruth, T>>;

/* forward declarations */
/*! \cond PRIVATE */
template<typename Ntk, bool ComputeTruth, typename CutData>
struct network_cuts;

template<typename Ntk, bool ComputeTruth = false, typename CutData = empty_cut_data>
network_cuts<Ntk, ComputeTruth, CutData> cut_enumeration( Ntk const& ntk, cut_enumeration_params const& ps = {}, cut_enumeration_stats * pst = nullptr );

/* function to update a cut */
template<typename CutData>
struct cut_enumeration_update_cut
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  // static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )  // node is defined in Ntk
  static void apply( Cut& cut, NetworkCuts& cuts, Ntk const& ntk, node<Ntk> const& n )  // node is defined in Ntk
  {
    (void)cut;
    (void)cuts;
    (void)ntk;
    (void)n;
  }
};

namespace detail
{
template<typename Ntk, bool ComputeTruth, typename CutData>
class cut_enumeration_impl;
}
/*! \endcond */

/**
 * @brief some configurable params for networks
 * 
 */
struct network_cuts_params
{
  // the max size of priority cut nums
  uint16_t max_cut_num{8};
  // the max cut's leaves num
  uint16_t max_cut_size{7};
};

/*! \brief Cut database for a network.
 *
 * The function `cut_enumeration` returns an instance of type `network_cuts`
 * which contains a cut database and can be queried to return all cuts of a
 * node, or the function of a cut (if it was computed).
 *
 * An instance of type `network_cuts` can only be constructed from the
 * `cut_enumeration` algorithm.
 */
template<typename Ntk, bool ComputeTruth, typename CutData>
struct network_cuts
{
public:
  static constexpr uint32_t max_cut_num = 12;
  using cut_t     = cut_type<ComputeTruth, CutData>;
  using cut_set_t = cut_set<cut_t, max_cut_num>;

// private:
  explicit network_cuts( uint32_t size ) 
    : _cuts( size ),
      _capsule_best_cuts(size), 
      _refs(size, 0u)
  {
    kitty::dynamic_truth_table zero( 0u ), proj( 1u );
    kitty::create_nth_var( proj, 0u );

    _truth_tables.insert( zero );
    _truth_tables.insert( proj );
  }

public:
  /*! \brief Returns the cut set of a node */
  cut_set_t& cuts( uint32_t node_index ) { return _cuts[node_index]; }

  /*! \brief Returns the cut set of a node */
  cut_set_t const& cuts( uint32_t node_index ) const { return _cuts[node_index]; }
  
  /*! \brief return the best cuts of node_index */
  cut_set_t const& cuts_capsule( uint32_t node_index) const { return _capsule_best_cuts[node_index]; }
  
  /*! \brief push cut c into the capsule */
  void push_2_capsule(uint32_t node_index, cut_t const& c) { _capsule_best_cuts[node_index].insert(c); }

  /*! \brief Returns the truth table of a cut */
  template<bool enabled = ComputeTruth, typename = std::enable_if_t<std::is_same_v<Ntk, Ntk> && enabled>>
  auto truth_table( cut_t const& cut ) const
  {
    return _truth_tables[cut->func_id];
  }

  /*! \brief Returns the total number of tuples that were tried to be merged */
  auto total_tuples() const
  {
    return _total_tuples;
  }

  /*! \brief Returns the total number of cuts in the database. */
  auto total_cuts() const
  {
    return _total_cuts;
  }

  void incre_total_cuts(uint32_t size)
  {
    _total_cuts += size;
  }
  
  void incre_total_tuples(uint32_t size)
  {
    _total_tuples += size;
  }

  kitty::dynamic_truth_table extend_truth_table_at(uint32_t func_id, cut_t& res)
  {
    return kitty::extend_to(_truth_tables[func_id], res.size() );
  }

  kitty::dynamic_truth_table at(uint32_t id)
  {
    return _truth_tables[id];
  }

  /*! \brief Returns the number of nodes for which cuts are computed */
  auto nodes_size() const
  {
    return _cuts.size();
  }

  /* compute positions of leave indices in cut `sub` (subset) with respect to
   * leaves in cut `sup` (super set).
   *
   * Example:
   *   compute_truth_table_support( {1, 3, 6}, {0, 1, 2, 3, 6, 7} ) = {1, 3, 4}
   */
  std::vector<uint8_t> compute_truth_table_support( cut_t const& sub, cut_t const& sup ) const
  {
    std::vector<uint8_t> support;
    support.reserve( sub.size() );

    auto itp = sup.begin();
    for ( auto i : sub )
    {
      itp = std::find( itp, sup.end(), i );
      support.push_back( static_cast<uint8_t>( std::distance( sup.begin(), itp ) ) );
    }

    return support;
  }

  /*! \brief Inserts a truth table into the truth table cache.
   *
   * This message can be used when manually adding or modifying cuts from the
   * cut sets.
   *
   * \param tt Truth table to add
   * \return Literal id from the truth table store
   */
  uint32_t insert_truth_table( kitty::dynamic_truth_table const& tt )
  {
    return _truth_tables.insert( tt );
  }

  //uint32_t truth_table_at( uint32_t id )
  //{
  //  return _truth_tables[id];
  //}

#pragma region X

  void clear_refs()
  {
    std::fill(_refs.begin(), _refs.end(), 0u);
  }

  auto& ref_at( node<Ntk> const& n )
  {
    return _refs[n];
  }

  void set_ref( node<Ntk> const& n, uint32_t v )
  {
    _refs[n] = v;
  }

  auto incr_ref( node<Ntk> const& n )
  {
    return ++_refs[n];
  }

  auto decr_ref( node<Ntk> const& n )
  {
    return --_refs[n];
  }

  float cut_lut_area (cut_t& cut, Ntk const& ntk, node<Ntk> const& node)  
  { 
    return 1.0f; 
  }

  float cut_lut_delay(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)  
  { 
    return 1.0f;
  }

  float cut_area_deref(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  { 
    float area = 1.0f;
    for(auto leaf : cut)
    {
      assert(_refs[leaf] > 0);
      if( --_refs[leaf] > 0 || ntk.is_pi(leaf) || ntk.is_constant(leaf) )
        continue;
      area += cut_area_deref(cuts( leaf ).best() , ntk, leaf);
    }
    return area;
  }

  float cut_area_ref(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  { 
    float area = 1.0f ;
    for(auto leaf : cut)
    {
      assert(_refs[leaf] >= 0);
      if( _refs[leaf]++ > 0 || ntk.is_pi(leaf) || ntk.is_constant(leaf) )
        continue;
      area += cut_area_ref(cuts( leaf ).best() , ntk, leaf);
    }
    return area;
  }

  float cut_area_derefed(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float res1, res2;
    if(cut.size() < 2)  
      return 0;
    res1 = cut_area_ref(cut, ntk, node);
    res2 = cut_area_deref(cut, ntk, node);
    assert(std::fabs(res1 - res2) < 0.005f);
    return res2;
  }

  float cut_area_refed(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float res1, res2;
    if(cut.size() < 2)  
      return 0;
    res1 = cut_area_deref(cut, ntk, node);
    res2 = cut_area_ref(cut, ntk, node);

    assert(std::fabs(res1 - res2) < 0.005f);
    return res2;
  }

  float cut_edge_deref(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float edge = (float)cut.size();
    for(auto leaf : cut)
    {
      assert(_refs[leaf] > 0);
      if( --_refs[leaf] > 0 || ntk.is_pi(leaf) || ntk.is_constant(leaf) )
        continue;
      edge += cut_edge_deref(cuts( leaf ).best() , ntk, leaf );
    }
    return edge;
  }

  float cut_edge_ref(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float edge = (float)cut.size();
    for(auto leaf : cut)
    {
      assert(_refs[leaf] >= 0);
      if( _refs[leaf]++ > 0 || ntk.is_pi(leaf) || ntk.is_constant(leaf) )
        continue;
      edge += cut_edge_ref(cuts( leaf ).best() , ntk, leaf);
    }
    return edge;
  }

  float cut_edge_derefed(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float res1, res2;
    if(cut.size() < 2)  
      return 0;
    res1 = cut_edge_ref(cut, ntk, node);
    res2 = cut_edge_deref(cut, ntk, node);

    assert(std::fabs(res1 - res2) < 0.005f);

    return res2;
  }

  float cut_edge_refed(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float res1, res2;
    if(cut.size() < 2)  
      return 0;
    res1 = cut_edge_deref(cut, ntk, node);
    res2 = cut_edge_ref(cut, ntk, node);

    assert(std::fabs(res1 - res2) < 0.005f);

    return res2;
  }
  float cut_area_flow(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float area_flow = 1.0f, addon_area_flow;
    for(auto leaf : cut)
    {
      const auto& best_leaf_cut = cuts( node ).best();
      if( !(ntk.fanout_size(leaf) ^ 0) || ntk.is_constant(leaf) )
        addon_area_flow = best_leaf_cut->data.area;
      else
        addon_area_flow = best_leaf_cut->data.area / ntk.fanout_size(leaf);

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

  float cut_edge_flow(cut_t& cut, Ntk const& ntk, node<Ntk> const& node)
  {
    float edge_flow = (float)cut.size(), addon_edge_flow;
    for(auto leaf : cut)
    {
      const auto& best_leaf_cut = cuts( node ).best();
      if( !(ntk.fanout_size(leaf) ^ 0) || ntk.is_constant(leaf) )
        addon_edge_flow = best_leaf_cut->data.edge;
      else
        addon_edge_flow = best_leaf_cut->data.edge / ntk.fanout_size(leaf);

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

#pragma endregion

private:
  template<typename _Ntk, bool _ComputeTruth, typename _CutData>
  friend class detail::cut_enumeration_impl;

  template<typename _Ntk, bool _ComputeTruth, typename _CutData>
  friend network_cuts<_Ntk, _ComputeTruth, _CutData> cut_enumeration( _Ntk const& ntk, cut_enumeration_params const& ps, cut_enumeration_stats * pst );

// private:
public:
  void add_zero_cut( uint32_t index )
  {
    auto& cut = _cuts[index].add_cut( &index, &index ); /* fake iterator for emptyness */

    if constexpr ( ComputeTruth )
    {
      cut->func_id = 0;
    }
  }

  void add_unit_cut( uint32_t index )
  {
    auto& cut = _cuts[index].add_cut( &index, &index + 1 );

    if constexpr ( ComputeTruth )
    {
      cut->func_id = 2;
    }
  }

  void print_cuts()
  {
    uint32_t i;
    for(i = 0u; i < _cuts.size(); ++i)
    {
      std::cout << _cuts[i] << std::endl;
    }
    printf("\n");
  }

  void print_capsule_cuts()
  {
    uint32_t i;
    for(i = 0u; i < _capsule_best_cuts.size(); ++i)
    {
      std::cout << _capsule_best_cuts[i] << std::endl;
    }
    printf("\n");
  }

  void print_storage()
  {
    printf("\033[0;32;40m Netwrok-Cuts-Storage: \033[0m \n");
    printf("_cuts size              : %lu\n", _cuts.size());
    printf("_capsule_best_cuts size : %lu\n", _capsule_best_cuts.size());
    printf("_refs size              : %lu\n", _refs.size());
  }

private:
  /* compressed representation of cuts */
  std::vector<cut_set_t> _cuts;           
  /* store the previous best cuts of the nodes, not include the PIs and constants */
  std::vector<cut_set_t> _capsule_best_cuts;
  /*  count the cut generation iterations*/
  uint8_t _uCount_iter{0u};    
  /* cut truth tables */
  truth_table_cache<kitty::dynamic_truth_table> _truth_tables;

  /* statistics */
  uint32_t _total_tuples{};
  std::size_t _total_cuts{};
  std::vector<uint32_t> _refs;      // the ref count of node in cut_network.
};

/*! \cond PRIVATE */
namespace detail
{

template<typename Ntk, bool ComputeTruth, typename CutData>
class cut_enumeration_impl
{
public:
  using cut_t = typename network_cuts<Ntk, ComputeTruth, CutData>::cut_t;
  using cut_set_t = typename network_cuts<Ntk, ComputeTruth, CutData>::cut_set_t;

  explicit cut_enumeration_impl( Ntk const& ntk, cut_enumeration_params const& ps, cut_enumeration_stats& st, network_cuts<Ntk, ComputeTruth, CutData>& cuts )
      : ntk( ntk ),
        ps( ps ),
        st( st ),
        cuts( cuts )
  {
    assert( ps.cut_limit < cuts.max_cut_num && "cut_limit exceeds the compile-time limit for the maximum number of cuts" );
  }

public:
  void run()
  {
    cuts._uCount_iter++;
    if(cuts._uCount_iter == 1u)
    {
      ntk.foreach_node( [this]( auto node ) {
        const auto index = ntk.node_to_index( node );

        if ( ps.very_verbose )
        {
          std::cout << printf( "[i] compute cut for node at index %d \n", index );
        }
        if ( ntk.is_constant( node ) )
        {
          cuts.add_zero_cut( index );
        }
        else if ( ntk.is_pi( node ) )
        {
          cuts.add_unit_cut( index );
        }
        else
        {
          if constexpr ( Ntk::min_fanin_size == 2 && Ntk::max_fanin_size == 2 )
          {
            merge_cuts2( index );
          }
          else
          {
            merge_cuts( index );
          }
        }
      } );
    }
    else  // after the first cut generation, the following iterations to generate cuts...
    {
      ntk.foreach_node( [this]( auto node ) {
        const auto index = ntk.node_to_index( node );

        if ( ps.very_verbose )
        {
          std::cout << printf( "[i] compute cut for node at index %d \n", index );
        }

        if ( ntk.is_constant( node ) )
        {
          return;
        }
        else if ( ntk.is_pi( node ) )
        {
          return;
        }
        else
        {
          if constexpr ( Ntk::min_fanin_size == 2 && Ntk::max_fanin_size == 2 )
          {
            merge_cuts2( index );
          }
          else
          {
            merge_cuts( index );
          }
        }
      } );
    }

    // printf("check best cuts of each node:\n");
    // cuts.print_cuts();
    // cuts.print_capsule_cuts();
  }

  /**
   * @brief Set the comparison function type
   * 
   * @param etc reference as the enum ETypeCmp
   */
  void set_cmp_type(ETypeCmp const& etc)
  {
    gf_set_etc(etc);
  }

// private:
  
  /**
   * @brief merge the cut by priority cut
   *      ref: Alan Mishchenko et al. Combinational and Sequential Mapping with Priority Cuts. ICCAD-2007
   *      1. compute cut_set the fanin's best cut cross-product 
   *      2. store the privious best cut to current
   *      3. limited the cut_set size to C
   * @param index the node's index for priority cut computation 
   * @param limit the limited priority cut size
   * 
   */
  void merge_cuts2( uint32_t index, bool truth = false)
  {
    auto node = ntk.index_to_node( index );
    const auto fanin = 2;
    uint32_t pairs{1};
    std::array<uint32_t, 2> children_id;      // store the children's index
    ntk.foreach_fanin( node, [this, &pairs, &children_id]( auto child, auto i ) {
      auto child_index = ntk.node_to_index( ntk.get_node( child ) );
      lcuts[i] = &cuts.cuts( child_index );   
      children_id[i] = child_index;
      pairs *= static_cast<uint32_t>( lcuts[i]->size() );
    } );

    lcuts[2] = &cuts.cuts( index );
    auto& rcuts = *lcuts[fanin];

    rcuts.clear();                // update current node's cutset by fanins

    cut_t new_cut;

    std::vector<cut_t const*> vcuts( fanin );

    cuts.incre_total_tuples(pairs);

    for ( auto const& c1 : *lcuts[0] )
    {
      for ( auto const& c2 : *lcuts[1] )
      {
        if ( !c1->merge( *c2, new_cut, ps.cut_size ) )
        {
          continue;
        }

        if ( rcuts.is_dominated( new_cut ) )
        {
          continue;
        }

        if constexpr ( ComputeTruth )
        {
          if(truth)
          {
            vcuts[0] = c1;
            vcuts[1] = c2;
            new_cut->func_id = compute_truth_table( index, vcuts, new_cut );
          }
        }

        cut_enumeration_update_cut<CutData>::apply( new_cut, cuts, ntk, node );

        rcuts.insert( new_cut );
      }
    }
    //  lcuts[0]  merge with lcuts[1]'s best cuts
    for(auto const& c1 : *lcuts[0])
    {
      for(auto const& c2 : cuts.cuts_capsule(children_id[1]))
      {
        if ( !c1->merge( *c2, new_cut, ps.cut_size ) )
        {
          continue;
        }

        if ( rcuts.is_dominated( new_cut ) )
        {
          continue;
        }
        if constexpr ( ComputeTruth )
        {
          if(truth)
          {
            vcuts[0] = c1;
            vcuts[1] = c2;
            new_cut->func_id = compute_truth_table( index, vcuts, new_cut );            
          }
        }

        cut_enumeration_update_cut<CutData>::apply( new_cut, cuts, ntk, node );

        rcuts.insert( new_cut );
      } 
    }
    //  lcuts[1]  merge with lcuts[0]'s best cuts,
    for(auto const& c1 : cuts.cuts_capsule(children_id[0])) // keep c1 be child0, c2 be child1
    {
      for(auto const& c2 : *lcuts[1])
      {
        if ( !c1->merge( *c2, new_cut, ps.cut_size ) )
        {
          continue;
        }

        if ( rcuts.is_dominated( new_cut ) )
        {
          continue;
        }

        if constexpr ( ComputeTruth )
        {
          if(truth)
          {
            vcuts[0] = c1;
            vcuts[1] = c2;
            new_cut->func_id = compute_truth_table( index, vcuts, new_cut );
          }
        }

        cut_enumeration_update_cut<CutData>::apply( new_cut, cuts, ntk, node );

        rcuts.insert( new_cut );
      } 
    }
    
    /* limit the maximum number of cuts, and reserve one position for trival cut */
    rcuts.limit( ps.cut_limit - 1 );

    cuts._total_cuts += rcuts.size();
    /* add trival cut ,and it directlt add at the end of cuts */ 
    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
    {
      cuts.add_unit_cut( index );
    }
  }

  /**
   * @brief compute the truth table of a cut
   * 
   * @param index 
   * @param vcuts 
   * @param res 
   * @return uint32_t 
   */
  uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
  {

    std::vector<kitty::dynamic_truth_table> tt( vcuts.size() );
    auto i = 0;

    for ( auto const& cut : vcuts )
    {
      assert( ( ( *cut )->func_id >> 1 ) <= cuts._truth_tables.size() );
      tt[i] = kitty::extend_to( cuts._truth_tables[( *cut )->func_id], res.size() );
      const auto supp = cuts.compute_truth_table_support( *cut, res );
      kitty::expand_inplace( tt[i], supp );
      ++i;
    }

    auto tt_res = ntk.compute( ntk.index_to_node( index ), tt.begin(), tt.end() );

    if ( ps.minimize_truth_table )
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
        return cuts._truth_tables.insert( tt_res_shrink );
      }
    }

    return cuts._truth_tables.insert( tt_res );
  }

private:
  Ntk const& ntk;
  cut_enumeration_params const& ps;
  cut_enumeration_stats& st;
  network_cuts<Ntk, ComputeTruth, CutData>& cuts;

  std::array<cut_set_t*, Ntk::max_fanin_size + 1> lcuts;
};
} /* namespace detail */

template<typename Ntk, bool ComputeTruth, typename CutData>
network_cuts<Ntk, ComputeTruth, CutData> cut_enumeration( Ntk const& ntk, cut_enumeration_params const& ps, cut_enumeration_stats * pst )
{
  cut_enumeration_stats st;
  network_cuts<Ntk, ComputeTruth, CutData> res( ntk.size() );
  detail::cut_enumeration_impl<Ntk, ComputeTruth, CutData> p( ntk, ps, st, res );
  p.run();

  if ( ps.verbose )
  {
    st.report();
  }
  if ( pst )
  {
    *pst = st;
  }

  return res;
}

iFPGA_NAMESPACE_HEADER_END
