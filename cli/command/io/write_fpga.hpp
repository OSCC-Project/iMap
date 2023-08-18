#pragma once
#include "alice/alice.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/operations/io/detail/write_verilog.hpp"
#include "include/operations/io/detail/writer_lut.hpp"

namespace alice {

class write_fpga_command : public command {
public:
    explicit write_fpga_command(const environment::ptr &env) :
        command(env, "Write the FPGA mapping result.") {
        add_option("--filename, -f", filename, "set the output file path.");
    }

    rules validity_rules() const { return {}; }

protected:
    void execute() {
        if( store<iFPGA::klut_network>().empty() ) {
            printf("WARN: there is no any FPGA mapping result, please refer to the command \"map_fpga\"\n");
            return;
        }
        iFPGA::klut_network& klut = store<iFPGA::klut_network>().current();
        if( store<iFPGA::write_verilog_params>().empty() ) {
            iFPGA::write_lut(klut, filename);
        }
        else {
            iFPGA::write_lut(klut, filename, store<iFPGA::write_verilog_params>().current());
        }
    }
private:
    std::string filename;
};
ALICE_ADD_COMMAND(write_fpga, "I/O");
}; // namespace alice