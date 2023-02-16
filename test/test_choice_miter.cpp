#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "algorithms/choice_miter.hpp"
#include "network/aig_network.hpp"
#include "algorithms/equivalence_checking.hpp"
#include "algorithms/miter.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "merge aigs to miter", "[choice_miter]" )
{
  // create first aig_network
    std::shared_ptr<iFPGA::aig_network> aig_0 = std::make_shared<iFPGA::aig_network>();

    const auto x1 = aig_0->create_pi();
    const auto x2 = aig_0->create_pi();
    const auto x3 = aig_0->create_pi();

    const auto f1 = aig_0->create_nor( x1, x2 );
    const auto f2 = aig_0->create_and( x1, x2 );
    const auto f3 = aig_0->create_nor( f1, f2 );
    const auto f4 = aig_0->create_nor( x3, f2 );
    const auto f5 = aig_0->create_lt( f1, x3 );
    const auto f6 = aig_0->create_nor( f4, f5 );

    aig_0->create_po( f3 );
    aig_0->create_po( f6 );
    aig_0->create_po( x3 );

    // create second aig_network
    std::shared_ptr<iFPGA::aig_network> aig_1 = std::make_shared<iFPGA::aig_network>();

    const auto x4 = aig_1->create_pi();
    const auto x5 = aig_1->create_pi();
    const auto x6 = aig_1->create_pi();

    const auto f7 = aig_1->create_nor( x4, x5 );
    const auto f8 = aig_1->create_and( x4, x5 );
    const auto f9 = aig_1->create_nor( f7, f8 );
    const auto f10 = aig_1->create_and( f7, x6 );
    const auto f11 = aig_1->create_and( f8, !x6 );
    const auto f12 = aig_1->create_nor( f10, f11 );

    aig_1->create_po( f9 );
    aig_1->create_po( f12 );
    aig_1->create_po( x6 );

    // merge aig to miter
    choice_miter * choice_miter = new iFPGA::choice_miter();
    choice_miter->add_aig(aig_0);
    choice_miter->add_aig(aig_1);
    iFPGA::aig_network mit = choice_miter->merge_aigs_to_miter();

    // check num of pi and po same
    CHECK( aig_0->num_pis() == aig_1->num_pis() );

    // check miter is right
    CHECK( mit.num_pis() == aig_0->num_pis() );
    CHECK( mit.num_pos() == aig_0->num_pos() );
    CHECK( mit.size() == 13 );

    // generate two XOR circuits
    auto check_miter1 = miter<iFPGA::aig_network, iFPGA::aig_network, iFPGA::aig_network>( *aig_0, mit );
    auto check_miter2 = miter<iFPGA::aig_network, iFPGA::aig_network, iFPGA::aig_network>( *aig_1, mit );
    
    // check the miter is equal to a1 and a2
    if(check_miter1.has_value() && check_miter2.has_value())
    {
      auto res = equivalence_checking(check_miter1.value()) && equivalence_checking(check_miter2.value());
      CHECK( res == true);
    }
    else
      CHECK(1 == 0);
    delete choice_miter;
}
