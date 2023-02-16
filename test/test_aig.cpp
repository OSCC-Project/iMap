#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "network/aig_network.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "create and use primary inputs in an AIG", "[aig]" )
{
  aig_network aig;

  auto a = aig.create_pi();

  CHECK( aig.size() == 2 );
  CHECK( aig.num_pis() == 1 );

  CHECK( std::is_same_v<std::decay_t<decltype( a )>, aig_network::signal> );

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = !a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = +a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = -a;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 0 );

  a = a ^ true;

  CHECK( a.index == 1 );
  CHECK( a.complement == 1 );
}

TEST_CASE( "create and use register in an AIG", "[aig]" )
{
  aig_network aig;

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();

  CHECK( aig.size() == 4 );
  CHECK( aig.num_registers() == 0 );
  CHECK( aig.num_pis() == 3 );
  CHECK( aig.num_pos() == 0 );

  const auto f1 = aig.create_and( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( !f1 );

  const auto f2 = aig.create_and( f1, x3 );
  aig.create_ri( f2 );

  const auto ro = aig.create_ro();
  aig.create_po( ro );

  CHECK( aig.num_pos() == 3 );
  CHECK( aig.num_registers() == 1 );

  aig.foreach_po( [&]( auto s, auto i ) {
    switch ( i )
    {
    case 0:
      CHECK( s == f1 );
      break;
    case 1:
      CHECK( s == !f1 );
      break;
    case 2:
      // Check if the output (connected to the register) data is the same as the node data being registered.
      CHECK( f2.data == aig.po_at( i ).data );
      break;
    default:
      CHECK( false );
      break;
    }
  } );
}

TEST_CASE( "structural properties of an AIG", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );

  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );
  CHECK( aig.num_pis() == 2 );
  CHECK( aig.num_pos() == 2 );
  CHECK( aig.num_gates() == 2 );
  CHECK( aig.fanin_size( aig.get_node( x1 ) ) == 0 );
  CHECK( aig.fanin_size( aig.get_node( x2 ) ) == 0 );
  CHECK( aig.fanin_size( aig.get_node( f1 ) ) == 2 );
  CHECK( aig.fanin_size( aig.get_node( f2 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );
}


TEST_CASE( "node and signal iteration in an AIG", "[aig]" )
{
  aig_network aig;

  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_or( x1, x2 );
  aig.create_po( f1 );
  aig.create_po( f2 );

  CHECK( aig.size() == 5 );

  /* iterate over nodes */
  uint32_t mask{0}, counter{0};
  aig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 31 );
  CHECK( counter == 10 );

  mask = 0;
  aig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 31 );

  mask = counter = 0;
  aig.foreach_node( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 1 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_node( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 1 );

  /* iterate over PIs */
  mask = counter = 0;
  aig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  aig.foreach_pi( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_pi( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 2 );

  /* iterate over POs */
  mask = counter = 0;
  aig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_po( [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  aig.foreach_po( [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_po( [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); return false; } );
  CHECK( mask == 8 );

  /* iterate over gates */
  mask = counter = 0;
  aig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; } );
  CHECK( mask == 24 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); } );
  CHECK( mask == 24 );

  mask = counter = 0;
  aig.foreach_gate( [&]( auto n, auto i ) { mask |= ( 1 << n ); counter += i; return false; } );
  CHECK( mask == 8 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_gate( [&]( auto n ) { mask |= ( 1 << n ); return false; } );
  CHECK( mask == 8 );

  /* iterate over fanins */
  mask = counter = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; } );
  CHECK( mask == 6 );
  CHECK( counter == 1 );

  mask = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); } );
  CHECK( mask == 6 );

  mask = counter = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s, auto i ) { mask |= ( 1 << aig.get_node( s ) ); counter += i; return false; } );
  CHECK( mask == 2 );
  CHECK( counter == 0 );

  mask = 0;
  aig.foreach_fanin( aig.get_node( f1 ), [&]( auto s ) { mask |= ( 1 << aig.get_node( s ) ); return false; } );
  CHECK( mask == 2 );
}
TEST_CASE( "substitude nodes with propagation in AIGs (test case 1)", "[aig]" )
{

  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();
  const auto x3 = aig.create_pi();
  const auto x4 = aig.create_pi();

  const auto f1 = aig.create_and( x1, x2 );
  const auto f2 = aig.create_and( x3, x4 );
  const auto f3 = aig.create_and( x1, x3 );
  const auto f4 = aig.create_and( f1, f2 );
  const auto f5 = aig.create_and( f3, f4 );

  aig.create_po( f5 );

  CHECK( aig.size() == 10u );
  CHECK( aig.num_gates() == 5u );
  CHECK( aig._storage->hash.size() == 5u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );

  CHECK( aig._storage->nodes[f5.index].children[0u].index == f3.index );
  CHECK( aig._storage->nodes[f5.index].children[1u].index == f4.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 1u );
  CHECK( !aig.is_dead( aig.get_node( f1 ) ) );

  aig.substitute_node( aig.get_node( x2 ), x3 );

  // Node of signal f1 is now relabelled
  CHECK( aig.size() == 10u );
  CHECK( aig.num_gates() == 4u );
  CHECK( aig._storage->hash.size() == 4u );
  CHECK( aig._storage->nodes[f1.index].children[0u].index == x1.index );
  CHECK( aig._storage->nodes[f1.index].children[1u].index == x2.index );

  CHECK( aig._storage->nodes[f5.index].children[0u].index == f3.index );
  CHECK( aig._storage->nodes[f5.index].children[1u].index == f4.index );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 2u );
  CHECK( aig.is_dead( aig.get_node( f1 ) ) );

//   aig = cleanup_dangling( aig );
//   CHECK( aig.num_gates() == 4u );
}
TEST_CASE( "substitute input by constant in NAND-based XOR circuit", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.num_gates() == 4u );
//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x6 );

  aig.substitute_node( aig.get_node( x1 ), aig.get_constant( true ) );

//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x3 );

  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f4 ) ) == 0u );
}

TEST_CASE( "substitute node by constant in NAND-based XOR circuit", "[aig]" )
{
  aig_network aig;
  const auto x1 = aig.create_pi();
  const auto x2 = aig.create_pi();

  const auto f1 = aig.create_nand( x1, x2 );
  const auto f2 = aig.create_nand( x1, f1 );
  const auto f3 = aig.create_nand( x2, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  CHECK( aig.num_gates() == 4u );
//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x6 );

  aig.substitute_node( aig.get_node( f3 ), aig.get_constant( false ) );

//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x2 );

  CHECK( aig.num_gates() == 2u );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( f3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( f4 ) ) == 0u );
  CHECK( !aig.is_dead( aig.get_node( f1 ) ) );
  CHECK( !aig.is_dead( aig.get_node( f2 ) ) );
  CHECK( aig.is_dead( aig.get_node( f3 ) ) );
  CHECK( aig.is_dead( aig.get_node( f4 ) ) );
}

TEST_CASE( "substitute node with complemented node", "[aig]" )
{
  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();

  auto const f1 = aig.create_and( x1, x2 );
  auto const f2 = aig.create_and( x1, f1 );
  aig.create_po( f2 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );

//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x8 );

  aig.substitute_node( aig.get_node( f2 ), !f2 );

  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 2 );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f1 ) ) == 1 );
  CHECK( aig.fanout_size( aig.get_node( f2 ) ) == 1 );

//   CHECK( simulate<kitty::static_truth_table<2u>>( aig )[0]._bits == 0x7 );
}

TEST_CASE( "substitute multiple nodes", "[aig]" )
{
  using node = aig_network::node;
  using signal = aig_network::signal;

  aig_network aig;
  auto const x1 = aig.create_pi();
  auto const x2 = aig.create_pi();
  auto const x3 = aig.create_pi();

  auto const n4 = aig.create_and( !x1, x2 );
  auto const n5 = aig.create_and( x1, n4 );
  auto const n6 = aig.create_and( x3, n5 );
  auto const n7 = aig.create_and( n4, x2 );
  auto const n8 = aig.create_and( !n5, !n7 );
  auto const n9 = aig.create_and( !n8, n4 );

  aig.create_po( n6 );
  aig.create_po( n9 );

  aig.substitute_nodes( std::list<std::pair<node, signal>>{
      {aig.get_node( n5 ), aig.get_constant( false )},
      {aig.get_node( n9 ), n4}
    } );

  CHECK( !aig.is_dead( aig.get_node( aig.get_constant( false ) ) ) );
  CHECK( !aig.is_dead( aig.get_node( x1 ) ) );
  CHECK( !aig.is_dead( aig.get_node( x2 ) ) );
  CHECK( !aig.is_dead( aig.get_node( x3 ) ) );
  CHECK( !aig.is_dead( aig.get_node( n4 ) ) );
  CHECK( aig.is_dead( aig.get_node( n5 ) ) );
  CHECK( aig.is_dead( aig.get_node( n6 ) ) );
  CHECK( aig.is_dead( aig.get_node( n7 ) ) );
  CHECK( aig.is_dead( aig.get_node( n8 ) ) );
  CHECK( aig.is_dead( aig.get_node( n9 ) ) );

  CHECK( aig.fanout_size( aig.get_node( aig.get_constant( false ) ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x1 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x2 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( x3 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n4 ) ) == 1u );
  CHECK( aig.fanout_size( aig.get_node( n5 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n6 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n7 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n8 ) ) == 0u );
  CHECK( aig.fanout_size( aig.get_node( n9 ) ) == 0u );

  aig.foreach_po( [&]( signal const o, uint32_t index ){
    switch ( index )
    {
    case 0:
      CHECK( o == aig.get_constant( false ) );
      break;
    case 1:
      CHECK( o == n4 );
      break;
    default:
      CHECK( false );
    }
  });
}

// int main()
// {
//     std::cout << "###########################################" << std::endl;                                         
//     std::cout << "#      _______ ______  _____ _______      #" << std::endl;
//     std::cout << "#     |__   __|  ____|/ ____|__   __|     #" << std::endl;
//     std::cout << "#         | |  | |__  | (___    | |       #" << std::endl; 
//     std::cout << "#         | |  |  __|  \___ \   | |       #" << std::endl; 
//     std::cout << "#         | |  | |____ ____) |  | |       #" << std::endl; 
//     std::cout << "#         |_|  |______|_____/   |_|       #" << std::endl;
//     std::cout << "###########################################" << std::endl;  
                             
//     aig_network aig;

//     const auto x1 = aig.create_pi();
//     const auto x2 = aig.create_pi();
//     const auto f1 = aig.create_and( x1, x2 );
//     const auto f2 = aig.create_or( x1, x2 );
//     aig.create_po( f1 );
//     aig.create_po( f2 );

//     assert( aig.size() == 5 );
//     return 1;
// }
