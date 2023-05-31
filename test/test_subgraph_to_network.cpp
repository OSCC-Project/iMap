#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "utils/ifpga_namespaces.hpp"
#include "optimization/detail/database_npn4_aig.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "aig_subgraph_storage for basic constructor", "[aig_subgraph_storage constructor]" )
{
  // we only conside the number is OK!
  database_npn4_aig subgraph(database_subgraphs_aig);

  CHECK( subgraph.num_pis() == 4u );
  CHECK( subgraph.num_pos() == 222u );
  CHECK( subgraph.num_gates() == 796u );

}