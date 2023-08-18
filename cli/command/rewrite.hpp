#pragma once
#include "alice/alice.hpp"
#include "include/operations/optimization/rewrite.hpp"

namespace alice 
{

class rewrite_command : public command 
{
public:
    explicit rewrite_command(const environment::ptr& env) : command(env, "performs technology-independent rewriting of AIG") 
    {
        add_option("--priority_size, -P", priority_size, "set the number of priority-cut size for cut enumeration [6, 20] [default=10]");
        add_option("--cut_size, -C", cut_size, "set the input size of cut for cut enumeration [2, 4] [default=4]");
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

        if(priority_size < 6u || priority_size > 20u) {
            printf("WARN: the priority size should be in the range [6, 20], please refer to the command \"rewrite -h\"\n");
            return;
        }

        if(cut_size < 2u || cut_size > 4u) {
            printf("WARN: the cut size should be in the range [2, 4], please refer to the command \"rewrite -h\"\n");
            return;
        }

        iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
        iFPGA::rewrite_params params;
        params.b_preserve_depth = preserve_level;
        params.b_use_zero_gain = zero_gain;
        params.cut_enumeration_ps.cut_size = cut_size;
        params.cut_enumeration_ps.cut_limit = priority_size;
        aig = iFPGA::rewrite(aig, params);

        store<iFPGA::aig_network>().current() = aig;
    }
private:
    uint32_t cut_size = 4u;
    uint32_t priority_size = 10u;
    bool preserve_level = true;
    bool zero_gain = false;
    bool verbose = false;
};
ALICE_ADD_COMMAND(rewrite, "Logic optimization");
};