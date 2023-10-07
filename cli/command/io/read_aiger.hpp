#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/operations/io/detail/write_verilog.hpp"
#include "include/operations/io/detail/aiger_reader.hpp"
#include "include/operations/io/reader.hpp"

namespace alice {

class read_aiger_command : public command {
public:
    explicit read_aiger_command(const environment::ptr &env) :
        command(env, "Read the And-Inverter Graph (AIG) format file.") {
        add_option("--filename, -f", filename, "set the input file path.");
    }

    rules validity_rules() const { return {}; }

protected:
    void execute() {
        if( !store<iFPGA::aig_network>().empty() ) {    // clear the previous store
            store<iFPGA::aig_network>().clear();
            store<iFPGA::klut_network>().clear();
            store<iFPGA::write_verilog_params>().clear();
        }
        // the history AIGs indexed with {0,1,2,3,4}
        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().extend();
        store<iFPGA::aig_network>().extend();
        // current AIG
        store<iFPGA::aig_network>().extend();
        store<iFPGA::write_verilog_params>().extend();
        iFPGA::Reader<iFPGA::aig_network> reader(filename, store<iFPGA::aig_network>().current(), store<iFPGA::write_verilog_params>().current());
    }
private:
    std::string filename;
};
ALICE_ADD_COMMAND(read_aiger, "I/O");
}; // namespace alice