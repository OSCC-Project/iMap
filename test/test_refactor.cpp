#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "network/aig_network.hpp"
#include "optimization/refactor.hpp"
#include "algorithms/miter.hpp"
#include "algorithms/equivalence_checking.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "refactor test case1", "[refactor-case1]" )
{
/**       o                                          o
**        |                                          |
**      ab+ac                                      a(b+c)
**       / \                                        / \\
**     ab   ac           ----->                    a   b+c
**    /  \ /  \                                        //\\
**    b   a    c                                       b  c
**/                                                    
    aig_network aig;
    auto a = aig.create_pi();
    auto b = aig.create_pi();
    auto c = aig.create_pi();

    auto ab = aig.create_and(a, b);
    auto ac = aig.create_and(a, c);
    auto o = aig.create_and(ab, ac);

    aig.create_po(o);

    refactor_params param;
    auto new_aig = iFPGA_NAMESPACE::refactor(aig, param);

    // check nodes num
    REQUIRE( new_aig.size() < aig.size());
    REQUIRE( new_aig.size() == 6 );

    // equivalence checking
    auto mit = *miter<aig_network, aig_network>(aig, new_aig);
    auto result = equivalence_checking(mit);
    REQUIRE(result);
    REQUIRE(*result);
}

TEST_CASE( "refactor test case2", "[refactor-case2]" )
{
/**
**             o                                  o     
**             ||                                 |   
**        ac+ad+bc+bd                         (a+b)(c+d)
**            / \                               // \\  
**           /   \                             a+b c+d
**        ab+ac bd+cd           ----->        //\\  //\\                   
**         //\\ //\\                          a  b  c  d  
**        ab ac bd cd
**         / X X X \   
**         a  b c  d
**       
**    
**/       
    aig_network aig;
    auto a = aig.create_pi();
    auto b = aig.create_pi();
    auto c = aig.create_pi();
    auto d = aig.create_pi();

    auto and1 = aig.create_and(a, b);
    auto and2 = aig.create_and(a, c);
    auto and3 = aig.create_and(b, d);
    auto and4 = aig.create_and(c, d);

    auto or1 = aig.create_or(and1, and2);
    auto or2 = aig.create_or(and3, and4);
    auto or3 = aig.create_or(or1, or2);

    aig.create_po(or3);

    refactor_params param;
    auto new_aig = iFPGA_NAMESPACE::refactor(aig, param);

    // check nodes num
    REQUIRE( new_aig.size() < aig.size());
    REQUIRE( new_aig.size() == 8);

    // equivalence checking
    auto mit = *miter<aig_network, aig_network>(aig, new_aig);
    auto result = equivalence_checking(mit);
    REQUIRE(result);
    REQUIRE(*result);
}