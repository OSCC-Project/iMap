/**
 * @file aig_network.hpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The AIG(And-Inverter Graph) network class implementation
 * 
 * @version 0.1
 * @date 2021-06-03
 * @copyright Copyright (c) 2021
 */

#pragma once
#include "utils/ifpga_namespaces.hpp"

#include "details/node.hpp"
#include "details/storage.hpp"
#include "details/events.hpp"

#include "utils/range.hpp"
#include "utils/traits.hpp"
#include "details/foreach.hpp"

#include "kitty/dynamic_truth_table.hpp"
#include "kitty/partial_truth_table.hpp"

#include <vector>
#include <list>
#include <stack>
#include <memory>
#include <optional>

iFPGA_NAMESPACE_HEADER_START

struct aig_storage_data
{
  uint32_t              num_pis{0u};
  uint32_t              num_pos{0u};
  std::vector<uint32_t> latches{};
  uint32_t              trav_id{0u};  
};

/**
 * @brief aig node
 *        data[0].h1: Fan-out size (we use MSB to indicate whether a node is dead)
 *        data[0].h2: Application-specific value
 *        data[1].h1: Visited flag
 *        data[1].h2: flags
 */
 
using aig_storage = storage< fixed_node<2,2>, aig_storage_data >;

class aig_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 2u;
  static constexpr auto max_fanin_size = 2u;


  using base_type = aig_network;
  using storage = std::shared_ptr<aig_storage>;
  using node = uint64_t;
  /// redefine NULL for equiv linked list, since 0 means const0 already
  static constexpr node AIG_NULL{0x7FFFFFFFFFFFFFFFu}; 


  struct signal
  {
    signal() = default;

    signal( uint64_t index, uint64_t complement )
        : complement( complement ), index( index )
    {
    }

    explicit signal( uint64_t data )
        : data( data )
    {
    }

    signal( aig_storage::node_type::pointer_type const& p )
        : complement( p.weight ), index( p.index )
    {
    }

    union {
      struct
      {
        uint64_t complement : 1;
        uint64_t index : 63;
      };
      uint64_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return {index, 0};
    }

    signal operator-() const
    {
      return {index, 1};
    }

    signal operator^( bool complement ) const
    {
      return signal( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( signal const& other ) const
    {
      return data == other.data;
    }

    bool operator==( node const& n) const
    {
      return index == n && complement == 0;
    }

    bool operator!=( node const& n) const
    {
      return !operator==(n);
    }

    bool operator!=( signal const& other ) const
    {
      return data != other.data;
    }

    bool operator<( signal const& other ) const
    {
      return data < other.data;
    }

    operator aig_storage::node_type::pointer_type() const
    {
      return {index, complement};
    }

#if __cplusplus > 201703L
    bool operator==( aig_storage::node_type::pointer_type const& other ) const
    {
      return data == other.data;
    }
#endif
  };


  /// the flag bit in data[1].h2
  enum FLAG_TYPE
  {
    F_PHASE = 0x1,
    F_MARKA = 0x1 << 1,
    F_MARKB = 0x1 << 2,

  };
  aig_network()
      : _storage( std::make_shared<aig_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  aig_network( std::shared_ptr<aig_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }

  bool operator==(const aig_network& other) { return _storage == other._storage; }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return {0, static_cast<uint64_t>( value ? 1 : 0 )};
  }

  signal create_pi( std::string const& name = std::string() )
  {
    (void)name;

    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = _storage->inputs.size();
    _storage->inputs.emplace_back( index );
    ++_storage->data.num_pis;
    // phase
    phase(index, 0);
    return {index, 0};
  }

  uint32_t create_po( signal const& f, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = _storage->outputs.size();
    _storage->outputs.emplace_back( f.index, f.complement );
    ++_storage->data.num_pos;
    return static_cast<uint32_t>( po_index );
  }

  signal create_ro( std::string const& name = std::string() )
  {
    (void)name;

    auto const index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0].data = node.children[1].data = _storage->inputs.size();
    _storage->inputs.emplace_back( index );
    return {index, 0};
  }

  uint32_t create_ri( signal const& f, int8_t reset = 0, std::string const& name = std::string() )
  {
    (void)name;

    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const ri_index = _storage->outputs.size();
    _storage->outputs.emplace_back( f.index, f.complement );
    _storage->data.latches.emplace_back( reset );
    return static_cast<uint32_t>( ri_index );
  }

  int8_t latch_reset( uint32_t index ) const
  {
    assert( index < _storage->data.latches.size() );
    return _storage->data.latches[ index ];
  }

  bool is_combinational() const
  {
    return ( static_cast<uint32_t>( _storage->inputs.size() ) == _storage->data.num_pis &&
             static_cast<uint32_t>( _storage->outputs.size() ) == _storage->data.num_pos );
  }

  bool is_constant( node const& n ) const
  {
    return n == 0;
  }

  bool is_ci( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data;
  }

  bool is_pi( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data < static_cast<uint64_t>(_storage->data.num_pis);
  }

  bool is_ro( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data >= static_cast<uint64_t>(_storage->data.num_pis);
  }

  bool constant_value( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return a;
  }

  signal create_not( signal const& a )
  {
    return !a;
  }
#pragma endregion

#pragma region Create binary functions
  bool find_and( signal a , signal b)
  {
    /* order inputs */
    if (a.index > b.index)
    {
      std::swap(a, b);
    }

    /* trivial cases */
    if (a.index == b.index || a.index == 0)
    {
      return true;
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;

    /* structural hashing */
    uint64_t found = _storage->hash_find(node);
    return found != 0;
  }

  signal create_and( signal a, signal b )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : get_constant( false );
    }
    else if ( a.index == 0 )
    {
      return a.complement ? b : get_constant( false );
    }

    storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;

    /* structural hashing */
    uint64_t found = _storage->hash_find(node);
    if(found)
    {
      assert( !is_dead( found) );
      return {found, 0};
    }

    const auto index = _storage->nodes.size();

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      //_storage->reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    _storage->nodes.push_back( node );
    _storage->hash_insert( node, index );

    // phase
    phase(index, (phase(a.index) ^ a.complement) & (phase(b.index) ^ b.complement));

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      fn( index );
    }

    return {index, 0};
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return !create_and( !a, !b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return create_and( !a, !b );
  }

  signal create_lt( signal const& a, signal const& b )
  {
    return create_and( !a, b );
  }

  signal create_le( signal const& a, signal const& b )
  {
    return !create_and( a, !b );
  }

  signal create_xor( signal const& a, signal const& b )
  {
    const auto fcompl = a.complement ^ b.complement;
    const auto c1 = create_and( +a, -b );
    const auto c2 = create_and( +b, -a );
    return create_and( !c1, !c2 ) ^ !fcompl;
  }

  signal create_xnor( signal const& a, signal const& b )
  {
    return !create_xor( a, b );
  }
#pragma endregion

#pragma region Createy ternary functions
  signal create_ite( signal cond, signal f_then, signal f_else )
  {
    bool f_compl{false};
    if ( f_then.index < f_else.index )
    {
      std::swap( f_then, f_else );
      cond.complement ^= 1;
    }
    if ( f_then.complement )
    {
      f_then.complement = 0;
      f_else.complement ^= 1;
      f_compl = true;
    }

    return create_and( !create_and( !cond, f_else ), !create_and( cond, f_then ) ) ^ !f_compl;
  }

  signal create_maj( signal const& a, signal const& b, signal const& c )
  {
    return create_or( create_and( a, b ), create_and( c, !create_and( !a, !b ) ) );
  }

  signal create_xor3( signal const& a, signal const& b, signal const& c )
  {
    return create_xor( create_xor( a, b ), c );
  }
#pragma endregion

#pragma region Create nary functions
  signal create_nary_and( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( true ), [this]( auto const& a, auto const& b ) { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_xor( a, b ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( aig_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( children.size() == 2u );
    return create_and( children[0u], children[1u] );
  }
#pragma endregion

#pragma region Restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    uint32_t fanin = 0u;
    if ( node.children[0].index == old_node )
    {
      fanin = 0u;
      new_signal.complement ^= node.children[0].weight;
    }
    else if ( node.children[1].index == old_node )
    {
      fanin = 1u;
      new_signal.complement ^= node.children[1].weight;
    }
    else
    {
      return std::nullopt;
    }

    // determine potential new children of node n
    signal child1 = new_signal;
    signal child0 = node.children[fanin ^ 1];

    if ( child0.index > child1.index )
    {
      std::swap( child0, child1 );
    }

    // check for trivial cases?
    if ( child0.index == child1.index )
    {
      const auto diff_pol = child0.complement != child1.complement;
      return std::make_pair( n, diff_pol ? get_constant( false ) : child1 );
    }
    else if ( child0.index == 0 ) /* constant child */
    {
      return std::make_pair( n, child0.complement ? child1 : get_constant( false ) );
    }

    // node already in hash table
    storage::element_type::node_type _hash_obj;
    _hash_obj.children[0] = child0;
    _hash_obj.children[1] = child1;
    if(uint64_t found = _storage->hash_find(_hash_obj); found != 0 && found != old_node)
    {
      return std::make_pair(n, signal( found, 0));
    }

    // remember before
    const auto old_child0 = signal{node.children[0]};
    const auto old_child1 = signal{node.children[1]};

    // erase old node in hash table
    _storage->hash_erase(node);

    // insert updated node into hash table
    node.children[0] = child0;
    node.children[1] = child1;
    _storage->hash_insert(node, n);

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;

    for ( auto const& fn : _events->on_modified )
    {
      fn( n, {old_child0, old_child1} );
    }

    return std::nullopt;
  }

  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    if ( is_dead( old_node ) )
      return;

    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        if ( old_node != new_signal.index )
        {
          /* increment fan-in of new node */
          _storage->nodes[new_signal.index].data[0].h1++;
        }
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs, constants, or already dead nodes */
    if ( n == 0 || is_ci( n ) || is_dead( n ) )
      return;

    /* delete the node (ignoring it's current fanout_size) */
    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */
    _storage->hash_erase(nobj);

    for ( auto const& fn : _events->on_delete )
    {
      fn( n );
    }

    /* if the node has been deleted, then deref fanout_size of
       fanins and try to take them out if their fanout_size become 0 */
    for ( auto i = 0u; i < 2u; ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  inline bool is_dead( node const& n ) const
  {
    return ( _storage->nodes[n].data[0].h1 >> 31 ) & 1;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( {old_node, new_signal} );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) || is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      replace_in_outputs( _old, _new );

      /* recursively reset old node */
      if ( _old != _new.index )
      {
        take_out_node( _old );
      }
    }
  }

  void substitute_nodes( std::list<std::pair<node, signal>> substitutions )
  {
    auto clean_substitutions = [&]( node const& n )
    {
      substitutions.erase( std::remove_if( std::begin( substitutions ), std::end( substitutions ),
                                           [&]( auto const& s ){
                                             if ( s.first == n )
                                             {
                                               node const nn = get_node( s.second );
                                               if ( is_dead( nn ) )
                                                 return true;

                                               /* deref fanout_size of the node */
                                               if ( fanout_size( nn ) > 0 )
                                               {
                                                 decr_fanout_size( nn );
                                               }
                                               /* remove the node if it's fanout_size becomes 0 */
                                               if ( fanout_size( nn ) == 0 )
                                               {
                                                 take_out_node( nn );
                                               }
                                               /* remove substitution from list */
                                               return true;
                                             }
                                             return false; /* keep */
                                           } ),
                           std::end( substitutions ) );
    };

    /* register event to delete substitutions if their right-hand side
       nodes get deleted */
    _events->on_delete.push_back( clean_substitutions );

    /* increment fanout_size of all signals to be used in
       substitutions to ensure that they will not be deleted */
    for ( const auto& s : substitutions )
    {
      incr_fanout_size( get_node( s.second ) );
    }

    while ( !substitutions.empty() )
    {
      auto const [old_node, new_signal] = substitutions.front();
      substitutions.pop_front();

      for ( auto index = 1u; index < _storage->nodes.size(); ++index )
      {
        /* skip CIs and dead nodes */
        if ( is_ci( index ) || is_dead( index ) )
          continue;

        /* skip nodes that will be deleted */
        if ( std::find_if( std::begin( substitutions ), std::end( substitutions ),
                           [&index]( auto s ){ return s.first == index; } ) != std::end( substitutions ) )
          continue;

        /* replace in node */
        if ( const auto repl = replace_in_node( index, old_node, new_signal ); repl )
        {
          incr_fanout_size( get_node( repl->second ) );
          substitutions.emplace_back( *repl );
        }
      }

      /* replace in outputs */
      replace_in_outputs( old_node, new_signal );

      /* replace in substitutions */
      for ( auto& s : substitutions )
      {
        if ( get_node( s.second ) == old_node )
        {
          s.second = is_complemented( s.second ) ? !new_signal : new_signal;
          incr_fanout_size( get_node( new_signal ) );
        }
      }

      /* finally remove the node: note that we never decrement the
         fanout_size of the old_node. instead, we remove the node and
         reset its fanout_size to 0 knowing that it must be 0 after
         substituting all references. */
      assert( !is_dead( old_node ) );
      take_out_node( old_node );

      /* decrement fanout_size when released from substitution list */
      decr_fanout_size( get_node( new_signal ) );
    }

    _events->on_delete.pop_back();
  }
#pragma endregion

#pragma region Structural properties
  auto size() const
  {
    return static_cast<uint32_t>( _storage->nodes.size() );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_latches() const
  {
      return static_cast<uint32_t>( _storage->data.latches.size() );
  }

  uint32_t num_pis() const
  {
    return _storage->data.num_pis;
  }

  void clear_pos()
  {
    _storage->outputs.clear();
    _storage->data.num_pos = 0;
  }

  auto num_pos() const
  {
    return _storage->data.num_pos;
  }

  auto num_registers() const
  {
    assert( static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis ) == static_cast<uint32_t>( _storage->outputs.size() - _storage->data.num_pos ) );
    return static_cast<uint32_t>( _storage->inputs.size() - _storage->data.num_pis );
  }

  auto num_gates() const
  {
    return static_cast<uint32_t>( _storage->hash_size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    return 2;
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1++ & UINT32_C( 0x7FFFFFFF );  // & avoid overflow
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  bool is_and( node const& n ) const
  {
    return n > 0 && !is_ci( n );
  }

  bool is_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_maj( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_ite( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor3( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_xor( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    (void)n;
    kitty::dynamic_truth_table _and( 2 );
    _and._bits[0] = 0x8;
    return _and;
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return signal( n, 0 );
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *(_storage->inputs.begin() + index);
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *(_storage->outputs.begin() + index);
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pis );
    return *(_storage->inputs.begin() + index);
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->data.num_pos );
    return *(_storage->outputs.begin() + index);
  }

  node ro_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() - _storage->data.num_pis );
    return *(_storage->inputs.begin() + _storage->data.num_pis + index);
  }

  signal ri_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() - _storage->data.num_pos );
    return *(_storage->outputs.begin() + _storage->data.num_pos + index);
  }

  uint32_t ci_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ){
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true;
      });
    return i;
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data );
    assert( _storage->nodes[n].children[0].data < _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t po_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_po( [&]( const auto& x, auto index ){
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true;
      });
    return i;
  }

  uint32_t ro_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data );
    assert( _storage->nodes[n].children[0].data >= _storage->data.num_pis );

    return static_cast<uint32_t>( _storage->nodes[n].children[0].data - _storage->data.num_pis );
  }

  uint32_t ri_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_ri( [&]( const auto& x, auto index ){
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true;
      });
    return i;
  }

  signal ro_to_ri( signal const& s ) const
  {
    return *( _storage->outputs.begin() + _storage->data.num_pos + _storage->nodes[s.index].children[0].data - _storage->data.num_pis );
  }

  node ri_to_ro( signal const& s ) const
  {
    return *( _storage->inputs.begin() + _storage->data.num_pis + ri_index( s ) );
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    auto r = range<uint64_t>( _storage->nodes.size() );
    detail::foreach_element_if( r.begin(), r.end(),
                                [this]( auto n ) { return !is_dead( n ); },
                                fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.begin() + _storage->data.num_pis, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.begin() + _storage->data.num_pos, fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin() + _storage->data.num_pis, _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin() + _storage->data.num_pos, _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_register( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, std::pair<signal,node>, void> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal,node>, void> ||
                   detail::is_callable_with_index_v<Fn, std::pair<signal,node>, bool> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal,node>, bool> );

    assert( _storage->inputs.size() - _storage->data.num_pis == _storage->outputs.size() - _storage->data.num_pos );
    auto ro = _storage->inputs.begin() + _storage->data.num_pis;
    auto ri = _storage->outputs.begin() + _storage->data.num_pos;
    if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal,node>, bool> )
    {
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        if ( !fn( std::make_pair(ri++, ro++) ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal,node>, bool> )
    {
      uint32_t index{0};
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        if ( !fn( std::make_pair(ri++, ro++), index++ ) )
          return;
      }
    }
    else if constexpr( detail::is_callable_without_index_v<Fn, std::pair<signal,node>, void> )
    {
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        fn( std::make_pair(*ri++, *ro++) );
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal,node>, void> )
    {
      uint32_t index{0};
      while ( ro != _storage->inputs.end() && ri != _storage->outputs.end() )
      {
        fn( std::make_pair(*ri++, *ro++), index++ );
      }
    }
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); /* start from 1 to avoid constant */
    detail::foreach_element_if( r.begin(), r.end(),
                                [this]( auto n ) { return !is_ci( n ) && !is_dead( n ); },
                                fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    /* we don't use foreach_element here to have better performance */
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{_storage->nodes[n].children[0]} ) )
        return;
      fn( signal{_storage->nodes[n].children[1]} );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if ( !fn( signal{_storage->nodes[n].children[0]}, 0 ) )
        return;
      fn( signal{_storage->nodes[n].children[1]}, 1 );
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn( signal{_storage->nodes[n].children[0]} );
      fn( signal{_storage->nodes[n].children[1]} );
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn( signal{_storage->nodes[n].children[0]}, 0 );
      fn( signal{_storage->nodes[n].children[1]}, 1 );
    }
  }

  signal get_child0(node const& p) const
  {
    return _storage->nodes[p].children[0];
  }

  signal get_child1(node const& p) const
  {
    return _storage->nodes[p].children[1];
  }

#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto v1 = *begin++;
    auto v2 = *begin++;

    return ( v1 ^ c1.weight ) && ( v2 ^ c2.weight );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto tt1 = *begin++;
    auto tt2 = *begin++;

    return ( c1.weight ? ~tt1 : tt1 ) & ( c2.weight ? ~tt2 : tt2 );
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto tt1 = *begin++;
    auto tt2 = *begin++;

    assert( tt1.num_bits() > 0 && "truth tables must not be empty" );
    assert( tt1.num_bits() == tt2.num_bits() );
    assert( tt1.num_bits() >= result.num_bits() );
    assert( result.num_blocks() == tt1.num_blocks() || ( result.num_blocks() == tt1.num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    result.resize( tt1.num_bits() );
    result._bits.back() = ( c1.weight ? ~(tt1._bits.back()) : tt1._bits.back() ) & ( c2.weight ? ~(tt2._bits.back()) : tt2._bits.back() );
    result.mask_bits();
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[0].h2 = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h2;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[1].h1 = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const
  {
    return _storage->data.trav_id;
  }

  void incr_trav_id() const
  {
    ++_storage->data.trav_id;
  }
#pragma endregion

#pragma region flags
  bool phase(node const& n) const
  {
    return _storage->nodes[n].data[1].h2 & F_PHASE; 
  }

  void phase(node const& n, bool b)
  {
    if(b)
    {
      _storage->nodes[n].data[1].h2 |= F_PHASE;
    }
    else
    {
      _storage->nodes[n].data[1].h2 &= ~F_PHASE;
    }
  }

  bool mark_a(node const& n) const
  {
    return _storage->nodes[n].data[1].h2 & F_MARKA;
  }

  void mark_a(node const& n, bool b)
  {
    if(b)
    {
      _storage->nodes[n].data[1].h2 |= F_MARKA;
    }
    else
    {
      _storage->nodes[n].data[1].h2 &= ~F_MARKA;
    }
  }

  bool mark_b(node const& n) const
  {
    return _storage->nodes[n].data[1].h2 & F_MARKB;
  }

  void mark_b(node const& n, bool b)
  {
    if(b)
    {
      _storage->nodes[n].data[1].h2 |= F_MARKB;
    }
    else
    {
      _storage->nodes[n].data[1].h2 &= ~F_MARKB;
    }
  }


#pragma endregion

#pragma region General methods
  auto& events() const
  {
    return *_events;
  }
#pragma endregion

public:
  std::shared_ptr<aig_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
};  // end class aig_network

iFPGA_NAMESPACE_HEADER_END


namespace std
{
  template<>  // 模板特例化
  struct hash< iFPGA_NAMESPACE::aig_network::signal >
  {
    uint64_t operator() ( iFPGA_NAMESPACE::aig_network::signal const& s) const noexcept
    {
      uint64_t k = s.data;
      k ^= k >> 33;
      k *= 0xff51afd7ed558ccd;
      k ^= k >> 33;
      k *= 0xc4ceb9fe1a85ec53;
      k ^= k >> 33;
      return k;
    }
  };  // end struct hash

} // end namespace std
