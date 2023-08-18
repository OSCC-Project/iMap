#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/operations/io/detail/write_verilog.hpp"
#include "include/operations/io/detail/write_dot.hpp"

namespace alice {

class write_dot_command : public command {
public:
    explicit write_dot_command(const environment::ptr &env) :
        command(env, "Write the DOT file for visualization.") {
        add_option("--filename, -f", filename, "set the output file path.");
        add_option("--type, -t", type, "write the DOT file for AIG or mapping result, 0 for AIG, 1 for FPGA, 2 for ASIC, [default = 0]");
    }

    rules validity_rules() const { return {}; }

protected:
    void execute() {
        if (type == 0) {
            if( store<iFPGA::aig_network>().empty() ) {
                printf("WARN: there is no any FPGA mapping result, please refer to the command \"map_fpga\"\n");
                return;
            }
            iFPGA::write_dot( store<iFPGA::aig_network>().current(), filename);
        }
        else if (type == 1) {
            if( store<iFPGA::klut_network>().empty() ) {
                printf("WARN: there is no any FPGA mapping result, please refer to the command \"map_fpga\"\n");
                return;
            }
            iFPGA::write_dot( store<iFPGA::klut_network>().current(), filename);
            return;
        }
        else if (type == 2) {
            printf("FAIL, comming soon!\n");
        }
        else {
            printf("FAIL, not supported type!\n");
            return;
        }
    }
private:
    std::string filename;
    int type{0};
};
ALICE_ADD_COMMAND(write_dot, "I/O");
}; // namespace alice