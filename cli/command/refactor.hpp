#pragma once
#include "alice/alice.hpp"
#include "include/operations/optimization/refactor.hpp"

namespace alice 
{

class refactor_command : public command 
{
public:
    explicit refactor_command(const environment::ptr& env) : command(env, "performs technology-independent refactoring of AIG") 
    {
        add_option("--max_input_size, -I", input_size, "set the max input size of the cone, <= 12 [default=10]");
        add_option("--max_cone_size, -C", cone_size, "set the max node size in the cone, <=20 [default=16]");
        add_flag("--level_preserve, -l", preserve_level, "toggles of preserving the leves [default=yes]");
        add_flag("--zero_gain, -z", zero_gain, "toggles of using zero-cost local replacement [default=no]");
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

        if( input_size > 12u ) {
            printf("WARN: the max input size of the cone should be <= 12, please refer to the command \"refactor -h\"\n");
            return;
        }

        if( cone_size > 20u ) {
            printf("WARN: the max node size in the cone should be <= 20, please refer to the command \"refactor -h\"\n");
            return;
        }

        iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
        iFPGA::refactor_params params;
        params.allow_depth_up = preserve_level;
        params.allow_zero_gain = zero_gain;
        params.max_leaves_num = input_size;
        params.max_cone_size = cone_size;
        params.verbose = verbose;
        aig = iFPGA::refactor(aig, params);

        store<iFPGA::aig_network>().current() = aig;
    }
private:
    uint32_t input_size = 10u;
    uint32_t cone_size = 16u;
    bool preserve_level = true;
    bool zero_gain = false;
    bool verbose = false;
};
ALICE_ADD_COMMAND(refactor, "Logic optimization");
};