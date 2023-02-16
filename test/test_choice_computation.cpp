#define CATCH_CONFIG_MAIN
#include "catch213/catch.hpp"
#include "algorithms/choice_computation.hpp"
#include "algorithms/miter.hpp"
#include "algorithms/equivalence_checking.hpp"

iFPGA_NAMESPACE_USING_NAMESPACE

TEST_CASE( "simulation to create candidant equiv classes", "[cand_equiv_classes]" )
{
    //SECTION("construction")
    aig_network aig;
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();

    auto sp = aig.create_and(sa, sb);
    auto sq = aig.create_and(sb, sc);
    auto sr = aig.create_and(sp, sc);
    auto st = aig.create_and(sr, sq);
    auto su = aig.create_and(sa, sq);

    auto nr = aig.get_node(sr);
    auto nt = aig.get_node(st);
    auto nu = aig.get_node(su);

    cand_equiv_classes simulator(aig, 8);

    SECTION("compute_equiv_classes")
    {
        simulator.compute_equiv_classes();
        for(uint64_t i = 0; i < nr; ++i)
        {
            REQUIRE(simulator.get_equiv_classes(i).size() == 1);
            REQUIRE(simulator.get_sim_repr(i) == i);
        }
        REQUIRE(simulator.get_equiv_classes(nr).size() == 3);
        REQUIRE(simulator.get_sim_repr(nr) == nr);
        REQUIRE(simulator.get_equiv_classes(nt).size() == 0);
        REQUIRE(simulator.get_sim_repr(nt) == nr);
        REQUIRE(simulator.get_equiv_classes(nu).size() == 0);
        REQUIRE(simulator.get_sim_repr(nu) == nr);
    }
}

TEST_CASE( "mux optimization on sat_prove", "sat_prover")
{
    aig_with_choice aig(100);
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();

    auto sf = aig.create_ite(sa, sb, sc);
    auto sg = aig.create_xor(sa, sb);
    auto sh = aig.create_and(sa, sc);

    aig.create_po(sf);

    choice_params params;
    sat_prover prover(params, aig, aig.size());
    auto result = prover.sat_prove(sf.index, sg.index);
    REQUIRE(result == percy::success);
    result = prover.sat_prove(sg.index, sh.index);
    REQUIRE(result == percy::success);
}

TEST_CASE( "choice computation", "[choice_computation]" )
{
    aig_network aig;
    auto sa = aig.create_pi();
    auto sb = aig.create_pi();
    auto sc = aig.create_pi();

    auto sp = aig.create_and(sa, sb);
    auto sq = aig.create_and(sb, sc);
    auto sr = aig.create_and(sp, sc);
    auto st = aig.create_and(sr, sq);
    auto su = aig.create_and(sa, sq);

    aig.create_po(st);
    aig.create_po(su);

    choice_params params;
    choice_computation cc(params, aig);
    aig_with_choice awc = cc.compute_choice();
    REQUIRE(awc.size() == 9);

    auto mit = *miter<aig_network, aig_with_choice>(aig, awc);
    auto result = equivalence_checking(mit);
    REQUIRE(result);
    REQUIRE(*result);

    /* changed in topo order:
     * p -> 6
     * q -> 4
     * r -> 7
     * t -> 8
     * u -> 5
    */

    SECTION("is_repr")
    {
        for (uint64_t i = 0; i < 7; ++i)
        {
            REQUIRE_FALSE(awc.is_repr(i));
        }
        REQUIRE(awc.is_repr(7));
        REQUIRE_FALSE(awc.is_repr(8));
    }

    SECTION("get_equiv_node")
    {
        for (uint64_t i = 0; i < 7; ++i)
        {
            REQUIRE(awc.get_equiv_node(i) == awc.AIG_NULL);
        }
        REQUIRE(awc.get_equiv_node(7) == 5);
        REQUIRE(awc.phase(7) == awc.phase(5));

        REQUIRE(awc.get_equiv_node(8) == awc.AIG_NULL);
    }
}
