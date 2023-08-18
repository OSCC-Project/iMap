#pragma once
#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/operations/io/detail/write_aiger.hpp"

namespace alice {

class write_aiger_command : public command {
public:
    explicit write_aiger_command(const environment::ptr &env) :
        command(env, "Write the And-Inverter Graph (AIG) format file.") {
        add_option("--filename, -f", filename, "set the output file path.");
    }

    rules validity_rules() const { return {}; }

protected:
    void execute() {
        if( store<iFPGA::aig_network>().empty() ) {    // clear the previous store
            printf("WARN: there is no any stored AIG file, please refer to the command \"read_aiger\"\n");
        }
        iFPGA::write_aiger( store<iFPGA::aig_network>().current(), filename);
    }
private:
    std::string filename;
};
ALICE_ADD_COMMAND(write_aiger, "I/O");
}; // namespace alice