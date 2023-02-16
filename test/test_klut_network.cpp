#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "network/klut_network.hpp"

#include <iostream>
#include <assert.h>
iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "klut_network basic function test", "[klut_network]" )
{
  iFPGA_NAMESPACE::klut_network klut;
  assert( klut.size() == 2 );

  const auto c0 = klut.get_node( klut.get_constant( false ) );
  const auto c1 = klut.get_node( klut.get_constant( true ) );
  const auto a = klut.create_pi();
  const auto b = klut.create_pi();

  kitty::dynamic_truth_table tt_nand( 2u ), tt_le( 2u ), tt_ge( 2u ), tt_or( 2u );
  kitty::create_from_hex_string( tt_nand, "7" );
  kitty::create_from_hex_string( tt_le, "2" );
  kitty::create_from_hex_string( tt_ge, "4" );
  kitty::create_from_hex_string( tt_or, "e" );

  // XOR with NAND
  const auto n1 = klut.create_node( {a, b}, tt_nand );
  const auto n2 = klut.create_node( {a, n1}, tt_nand );
  const auto n3 = klut.create_node( {b, n1}, tt_nand );
  const auto n4 = klut.create_node( {n2, n3}, tt_nand );
  klut.create_po( n4 );

  std::vector<node<iFPGA_NAMESPACE::klut_network>> nodes;
  klut.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );
  assert( klut.fanout_size( n4 ) == 1 );
  klut.foreach_po( [&]( auto f ) {
    assert( f == n4 );
    return false;
  } );

  // XOR with AND and OR
  const auto n5 = klut.create_node( {a, b}, tt_le );
  const auto n6 = klut.create_node( {a, b}, tt_ge );
  const auto n7 = klut.create_node( {n5, n6}, tt_or );

  nodes.clear();
  klut.foreach_node( [&]( auto node ) { nodes.push_back( node ); } );

  assert( klut.fanout_size( n7 ) == 0 );
  printf("pass\n");
  // substitute nodes
  klut.substitute_node( n4, n7 );
      
  assert( klut.size() == 11 );
  assert( klut.fanout_size( n4 ) == 0 );
  assert( klut.fanout_size( n7 ) == 1 );
  klut.foreach_po( [&]( auto f ) {
    assert( f == n7 );
    return false;
  } );

}
