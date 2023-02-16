#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "network/aig_network.hpp"
#include "optimization/and_balance.hpp"
#include "algorithms/miter.hpp"
#include "algorithms/equivalence_checking.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "basic and4 balance", "[and4-balance]" )
{
    aig_network aig;
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();
    auto sd = aig.create_pi();

    auto sp = aig.create_and(sa, sb);
    auto sq = aig.create_and(sp, sc);
    auto sr = aig.create_and(sq, sd);

    aig.create_po(sr);

    auto balancer = and_balance(aig);
    auto new_aig = balancer.run();

    // level checking
    auto dv = depth_view(new_aig);
    REQUIRE(dv.size() == 8);
    REQUIRE(dv.level(0) == 0);
    REQUIRE(dv.level(1) == 0);
    REQUIRE(dv.level(2) == 0);
    REQUIRE(dv.level(3) == 0);
    REQUIRE(dv.level(4) == 0);
    REQUIRE(dv.level(5) == 1);
    REQUIRE(dv.level(6) == 1);
    REQUIRE(dv.level(7) == 2);

    // equivalence checking
    auto mit = *miter<aig_network, aig_network>(aig, new_aig);
    auto result = equivalence_checking(mit);
    REQUIRE(result);
    REQUIRE(*result);
}

TEST_CASE( "and balance", "[and-balance]" )
{
/**
**        |                                          |
**       / \                                        / \\ 
**      o   g                                      o     o
**     / \\                                       / \   / \\ 
**    o     o                                    o   g  o    o
**   / \\  // \              ----->             / \\   //\   / \\ 
**  a   b  c  o                                a   b   c  f  d  e 
**           // \  
**          o   f
**         / \\ 
**        d   e  
**/       
    aig_network aig;
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();
    auto sd = aig.create_pi();
    auto se = aig.create_pi();
    auto sf = aig.create_pi();
    auto sg = aig.create_pi();

    auto sp = aig.create_and(sa, !sb);

    auto sq = aig.create_and(sd, !se);
    auto sr = aig.create_and(!sq, sf);
    auto ss = aig.create_and(!sc, sr);

    auto st = aig.create_and(sp, !ss);
    auto su = aig.create_and(sg, st);

    aig.create_po(su);

    // orignal level checking
    {
        auto dv = depth_view(aig);
        REQUIRE(dv.level(sp.index) == 1);
        REQUIRE(dv.level(sq.index) == 1);
        REQUIRE(dv.level(sr.index) == 2);
        REQUIRE(dv.level(ss.index) == 3);
        REQUIRE(dv.level(st.index) == 4);
        REQUIRE(dv.level(su.index) == 5);
    }

    auto balancer = and_balance(aig);
    auto new_aig = balancer.run();

    // balanced level checking
    auto dv = depth_view(new_aig);
    REQUIRE(dv.size() == 14);
    REQUIRE(dv.depth() == 3);
    REQUIRE(dv.level(8) == 1);
    REQUIRE(dv.level(9) == 1);
    REQUIRE(dv.level(10) == 2);
    REQUIRE(dv.level(11) == 1);
    REQUIRE(dv.level(12) == 2);
    REQUIRE(dv.level(13) == 3);

    // equivalence checking
    auto mit = *miter<aig_network, aig_network>(aig, new_aig);
    auto result = equivalence_checking(mit);
    REQUIRE(result);
    REQUIRE(*result);
}