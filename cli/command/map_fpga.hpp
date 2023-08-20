#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/database/views/mapping_view.hpp"
#include "include/operations/algorithms/aig_with_choice.hpp"
#include "include/operations/algorithms/choice_computation.hpp"
#include "include/operations/algorithms/choice_miter.hpp"

#include "include/operations/algorithms/klut_mapping.hpp"
#include "include/operations/algorithms/network_to_klut.hpp"

#include "include/operations/optimization/rewrite.hpp"
#include "include/operations/optimization/refactor.hpp"
#include "include/operations/optimization/and_balance.hpp"
namespace alice 
{

class map_fpga_command : public command 
{
public:
    explicit map_fpga_command(const environment::ptr& env) : command(env, "performs FPGA technology mapping of AIG") 
    {
        add_option("--priority_size, -P", priority_size, "set the number of priority-cut size for cut enumeration [6, 20] [default=10]");
        add_option("--cut_size, -C", cut_size, "set the input size of cut for cut enumeration [2, 6] [default=6]");
        add_option("--type, -t", type, "set the type of mapping, 0/1 means mapping without/with choice, [default=0]");
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
            printf("WARN: the priority size should be in the range [6, 20], please refer to the command \"map_fpga -h\"\n");
            return;
        }

        if(cut_size < 2u || cut_size > 6u) {
            printf("WARN: the cut size should be in the range [2, 6], please refer to the command \"map_fpga -h\"\n");
            return;
        }

        if(type != 0 && type != 1) {
            printf("WARN: the type should be 0 or 1, please refer to the command \"map_fpga -h\"\n");
            return;
        }

        if( store<iFPGA::klut_network>().empty() ) {
            store<iFPGA::klut_network>().extend();
        }

        if(type == 1) { // mapping with choice

            iFPGA::aig_network aig_c1 = store<iFPGA::aig_network>().current()._storage;
            iFPGA::rewrite_params ps_rewrite;
            iFPGA::refactor_params ps_refactor;

            ps_rewrite.b_preserve_depth = true;
            ps_rewrite.b_use_zero_gain = false;
            aig_c1 = iFPGA::rewrite(aig_c1, ps_rewrite);
            aig_c1 = iFPGA::refactor(aig_c1, ps_refactor);
            aig_c1 = iFPGA::balance_and( aig_c1 );
            ps_rewrite.b_use_zero_gain = true;
            aig_c1 = iFPGA::rewrite(aig_c1, ps_rewrite);

            iFPGA::aig_network aig_c2 = aig_c1._storage;
            ps_rewrite.b_preserve_depth = false;
            ps_rewrite.b_use_zero_gain = false;
            aig_c2 = iFPGA::rewrite(aig_c2, ps_rewrite);
            aig_c2 = iFPGA::refactor(aig_c2, ps_refactor);
            aig_c2 = iFPGA::balance_and( aig_c2 );
            ps_rewrite.b_preserve_depth = true;
            aig_c2 = iFPGA::rewrite(aig_c2, ps_rewrite);
            ps_rewrite.b_use_zero_gain = true;
            aig_c2 = iFPGA::rewrite(aig_c2, ps_rewrite);
            aig_c2 = iFPGA::balance_and( aig_c2 );
            aig_c2 = iFPGA::refactor(aig_c2, ps_refactor);
            aig_c2 = iFPGA::rewrite(aig_c2, ps_rewrite);
            aig_c2 = iFPGA::balance_and( aig_c2 );
            
            iFPGA::choice_miter cm;
            cm.add_aig( std::make_shared<iFPGA_NAMESPACE::aig_network>(aig_c2) );
            cm.add_aig( std::make_shared<iFPGA_NAMESPACE::aig_network>(aig_c1) );
            cm.add_aig( std::make_shared<iFPGA_NAMESPACE::aig_network>(store<iFPGA::aig_network>().current()._storage) );

            iFPGA::choice_params params_choice;
            iFPGA::choice_computation cc(params_choice, cm.merge_aigs_to_miter());

            iFPGA::aig_with_choice awc = cc.compute_choice();
            iFPGA::mapping_view<iFPGA::aig_with_choice, true, false> mapped_aig(awc);
            iFPGA::klut_mapping_params param_mapping;
            param_mapping.cut_enumeration_ps.cut_size = cut_size;
            param_mapping.cut_enumeration_ps.cut_limit = priority_size;
            param_mapping.verbose = verbose;
            iFPGA_NAMESPACE::klut_mapping<decltype(mapped_aig), true>(mapped_aig, param_mapping);
            const auto kluts = *iFPGA_NAMESPACE::choice_to_klut<iFPGA_NAMESPACE::klut_network>( mapped_aig );

            store<iFPGA::klut_network>().current() = kluts;
        }
        else {       // mapping without choice
            iFPGA::aig_network aig = store<iFPGA::aig_network>().current();
            iFPGA::aig_with_choice awc(aig);
            iFPGA::mapping_view<iFPGA::aig_with_choice, true, false> mapped_aig(awc);
            iFPGA::klut_mapping_params params;
            params.cut_enumeration_ps.cut_size = cut_size;
            params.cut_enumeration_ps.cut_limit = priority_size;
            params.verbose = verbose;
            iFPGA_NAMESPACE::klut_mapping<decltype(mapped_aig), true>(mapped_aig, params);
            const auto kluts = *iFPGA_NAMESPACE::choice_to_klut<iFPGA_NAMESPACE::klut_network>( mapped_aig );

            store<iFPGA::klut_network>().current() = kluts;
        }        

    }
private:
    uint32_t priority_size = 10u;
    uint32_t cut_size = 6u;
    int type = 0;               // 0 means mapping without choice, 1 means mapping with choice;
    bool verbose = false;
};
ALICE_ADD_COMMAND(map_fpga, "Technology mapping");
};