#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/database/views/depth_view.hpp"

namespace alice 
{

class print_stats_command : public command 
{
public:
    explicit print_stats_command(const environment::ptr& env) : command(env, "print the stats for current Network!") 
    {
    }

    rules validity_rules() const
    {
        return { has_store_element<iFPGA::aig_network>(env) };
    }
protected:
    void execute()
    {
        iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
        iFPGA::depth_view<iFPGA::aig_network> daig(aig);
        printf("Stats of aig: pis=%d, pos=%d, area=%d, depth=%d\n", aig.num_pis(), aig.num_pos(), aig.num_gates(), daig.depth());
    }
};
ALICE_ADD_COMMAND(print_stats, "Utils");
};