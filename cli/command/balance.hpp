#pragma once
#include "alice/alice.hpp"
#include "include/operations/optimization/and_balance.hpp"

namespace alice 
{

class balance_command : public command 
{
public:
    explicit balance_command(const environment::ptr& env) : command(env, "performs technology-independent AND-tree balance of AIG") 
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
        store<iFPGA::aig_network>().current() = iFPGA::balance_and( store<iFPGA::aig_network>().current() );
    }
private:
    bool verbose = false;
};
ALICE_ADD_COMMAND(balance, "Logic optimization");
};