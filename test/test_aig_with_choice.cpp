#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "algorithms/aig_with_choice.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "AIG network with choice nodes", "[choice node]" )
{
    //SECTION("construction")
    aig_with_choice aig(9);
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();

    auto sp = aig.create_and(sa, sb);
    auto sq = aig.create_and(sb, sc);
    auto sr = aig.create_and(sp, sc);
    auto st = aig.create_and(sr, sq);
    auto su = aig.create_and(sa, sq);

    auto np = aig.get_node(sp);
    auto nq = aig.get_node(sq);
    auto nr = aig.get_node(sr);
    auto nt = aig.get_node(st);
    auto nu = aig.get_node(su);

    SECTION("init_choices")
    {
        //aig.init_choices();
        REQUIRE_FALSE(aig.is_repr(np));
        REQUIRE_FALSE(aig.is_repr(nq));
        REQUIRE_FALSE(aig.is_repr(nr));
        REQUIRE_FALSE(aig.is_repr(nt));
        REQUIRE_FALSE(aig.is_repr(nu));

        REQUIRE(aig.get_equiv_node(np) == aig_with_choice::AIG_NULL);
        REQUIRE(aig.get_equiv_node(nq) == aig_with_choice::AIG_NULL);
        REQUIRE(aig.get_equiv_node(nr) == aig_with_choice::AIG_NULL);
        REQUIRE(aig.get_equiv_node(nt) == aig_with_choice::AIG_NULL);
        REQUIRE(aig.get_equiv_node(nu) == aig_with_choice::AIG_NULL);
    }

    SECTION("set_choice")
    {
        //aig.init_choices();
        REQUIRE_FALSE(aig.set_choice(nt, nr));
        REQUIRE(aig.set_choice(nu, nr));
        REQUIRE(aig.get_equiv_node(nr) == nu);
    }

}
