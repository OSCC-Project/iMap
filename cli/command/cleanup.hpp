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
        add_flag("--verbose, -v", verbose, "toggles of report verbose information");
    }

    rules validity_rules() const { return {}; }
protected:
    void execute()
    {
        if( store<iFPGA::aig_network>().empty() ) {
            printf("WARN: there is no any stored AIG file, please refer to the command \"read_aiger\"\n");
            return;
        }
        store<iFPGA::aig_network>().current() = iFPGA::cleanup_dangling( store<iFPGA::aig_network>().current() );
    }
private:
    bool verbose = false;
};
ALICE_ADD_COMMAND(cleanup, "Utils");
};