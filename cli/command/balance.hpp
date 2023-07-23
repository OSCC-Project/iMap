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
        aig = iFPGA::balance_and(aig);

        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().current() = aig;
    }
private:
    bool verbose = false;
};
ALICE_ADD_COMMAND(balance, "Logic optimization");
};