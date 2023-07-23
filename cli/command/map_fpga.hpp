#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/database/views/mapping_view.hpp"
#include "include/operations/algorithms/aig_with_choice.hpp"
#include "include/operations/algorithms/klut_mapping.hpp"
#include "include/operations/algorithms/network_to_klut.hpp"

namespace alice 
{

class map_fpga_command : public command 
{
public:
    explicit map_fpga_command(const environment::ptr& env) : command(env, "performs FPGA technology mapping of AIG") 
    {
        add_option("--priority_size, -P", priority_size, "set the number of priority-cut size for cut enumeration [6, 20] [default=10]");
        add_option("--cut_size, -C", cut_size, "set the input size of cut for cut enumeration [4, 5] [default=4]");
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
        iFPGA::aig_with_choice awc(aig);
        iFPGA::mapping_view<iFPGA::aig_with_choice, true, false> mapped_aig(awc);
        iFPGA::klut_mapping_params params;
        params.cut_enumeration_ps.cut_size = cut_size;
        params.cut_enumeration_ps.cut_limit = priority_size;
        params.verbose = verbose;
        iFPGA_NAMESPACE::klut_mapping<decltype(mapped_aig), true>(mapped_aig, params);
        const auto kluts = *iFPGA_NAMESPACE::choice_to_klut<iFPGA_NAMESPACE::klut_network>( mapped_aig );

        store<iFPGA::klut_network>().extend();
        store<iFPGA::klut_network>().current() = kluts;
    }
private:
    uint32_t cut_size = 4u;
    uint32_t priority_size = 10u;
    bool verbose = false;
};
ALICE_ADD_COMMAND(map_fpga, "Technology mapping");
};