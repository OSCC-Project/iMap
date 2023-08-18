#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/database/views/mapping_view.hpp"
#include "include/operations/algorithms/aig_with_choice.hpp"
#include "include/operations/algorithms/klut_mapping.hpp"
#include "include/operations/algorithms/network_to_klut.hpp"
#include "include/operations/algorithms/detail/convert_to_aig.hpp"

namespace alice 
{

class lut_opt_command : public command 
{
public:
    explicit lut_opt_command(const environment::ptr& env) : command(env, "performs technology-independent LUT-opt of AIG") 
    {
        add_option("--priority_size, -P", priority_size, "set the number of priority-cut size for cut enumeration [6, 20] [default=10]");
        add_option("--cut_size, -C", cut_size, "set the input size of cut for cut enumeration [2, 8] [default=6]");
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

        if( priority_size < 6u || priority_size > 20u ) {
            printf("WARN: the priority size should be in the range [6, 20], please refer to the command \"lut_opt -h\"\n");
            return;
        }

        if( cut_size < 2u || cut_size > 8u ) {
            printf("WARN: the cut size should be in the range [2, 8], please refer to the command \"lut_opt -h\"\n");
            return;
        }

        iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
        iFPGA::aig_with_choice awc(aig);
        iFPGA::mapping_view<iFPGA::aig_with_choice, true, false> mapped_aig(awc);
        iFPGA::klut_mapping_params params;
        params.cut_enumeration_ps.cut_size = cut_size;
        params.cut_enumeration_ps.cut_limit = priority_size;
        params.bZeroGain = zero_gain;
        params.verbose = verbose;
        iFPGA_NAMESPACE::klut_mapping<decltype(mapped_aig), true>(mapped_aig, params);
        const auto kluts = *iFPGA_NAMESPACE::choice_to_klut<iFPGA_NAMESPACE::klut_network>( mapped_aig );
        iFPGA::aig_network res = iFPGA::convert_klut_to_aig( kluts );

        store<iFPGA::aig_network>().current() = res;
    }
private:
    uint32_t cut_size = 6u;
    uint32_t priority_size = 10u;
    bool zero_gain = false;
    bool verbose = false;
};
ALICE_ADD_COMMAND(lut_opt, "Logic optimization");
};