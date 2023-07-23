#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/operations/algorithms/cleanup.hpp"

namespace alice 
{

class cleanup_command : public command 
{
public:
    explicit cleanup_command(const environment::ptr& env) : command(env, "clean up the dangling nodes for AIGc") 
    {
        add_option("--verbose, -v", verbose, "toggles of report verbose information");
    }

    rules validity_rules() const
    {
        return { has_store_element<iFPGA::aig_network>(env) };
    }
protected:
    void execute()
    {
        iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().current() = iFPGA::cleanup_dangling(aig);
    }
private:
    bool verbose = false;
};
ALICE_ADD_COMMAND(cleanup, "Utils");
};