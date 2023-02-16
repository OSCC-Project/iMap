#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "cut/cut_enumeration.hpp"
#include "network/aig_network.hpp"
#include "network/klut_network.hpp"

#include <iostream>
#include <vector>
#include <assert.h>

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "cut_enumeration for aig_network", "[aig_network cut enumeration]" )
{
  iFPGA_NAMESPACE::aig_network aig;  
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( f1, a );
  const auto f3 = aig.create_nand( f1, b );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );
  const auto cuts = iFPGA_NAMESPACE::cut_enumeration( aig );
  const auto to_vector = []( auto const& cut ) {
    return std::vector<uint32_t>( cut.begin(), cut.end() );
  };
  /* all unit cuts are in the back */
  aig.foreach_node( [&]( auto n ) {
    if ( aig.is_constant( n ) )
      return;
    auto const& set = cuts.cuts( aig.node_to_index( n ) );
    assert( to_vector( set[static_cast<uint32_t>( set.size() - 1 )] ) == std::vector<uint32_t>{aig.node_to_index( n )} );
  } );
  const auto i1 = aig.node_to_index( aig.get_node( f1 ) );
  const auto i2 = aig.node_to_index( aig.get_node( f2 ) );
  const auto i3 = aig.node_to_index( aig.get_node( f3 ) );
  const auto i4 = aig.node_to_index( aig.get_node( f4 ) );
  
  assert( cuts.cuts( i1 ).size() == 2 );
  assert( cuts.cuts( i2 ).size() == 3 );
  assert( cuts.cuts( i3 ).size() == 3 );
  assert( cuts.cuts( i4 ).size() == 5 );
}

TEST_CASE( "cut_enumeration for klut_network", "[klut_network cut enumeration]" )
{
  iFPGA_NAMESPACE::klut_network klut;
  const auto a = klut.create_pi();
  const auto b = klut.create_pi();
  const auto g1 = klut.create_not( a );
  const auto g2 = klut.create_and( g1, b );
  const auto g3 = klut.create_not( b );
  const auto g4 = klut.create_and( a, g3 );
  kitty::dynamic_truth_table or_func( 2u );
  kitty::create_from_binary_string( or_func, "1110" );
  const auto g5 = klut.create_node( {g2, g4}, or_func );
  klut.create_po( g5 );
  iFPGA_NAMESPACE::cut_enumeration_params ps;
  const auto cuts = cut_enumeration<iFPGA_NAMESPACE::klut_network, true>( klut, ps );
  assert( (cuts.cuts( klut.node_to_index( klut.get_node( g1 ) ) ).size() == 2u) );
  assert( (cuts.cuts( klut.node_to_index( klut.get_node( g3 ) ) ).size() == 2u) );
  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g1 ) ) ) ) {
    assert( cut->size() == 1u );
  }
  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g3 ) ) ) ) {
    assert( cut->size() == 1u );
  }
  for ( auto const& cut : cuts.cuts( klut.node_to_index( klut.get_node( g5 ) ) ) ) {
    if ( cut->size() == 2u && *cut->begin() == 2u ) {
      assert( (cuts.truth_table( *cut )._bits[0] == 0x6u) );
    }
  }

}
