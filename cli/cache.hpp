#pragma once

#include "alice/alice.hpp"
#include "include/database/network/aig_network.hpp"
#include "include/database/network/klut_network.hpp"
#include "include/operations/io/detail/write_verilog.hpp"
#include "include/operations/io/reader.hpp"
#include "include/operations/io/writer.hpp"

namespace alice {

// add aiger cache to the command environment
ALICE_ADD_STORE(iFPGA::aig_network, "aig", "a", "AIG", "AIGs")
ALICE_PRINT_STORE(iFPGA::aig_network, os, element) {
    os << "AIG PI/PO =" << element.num_pis() << "/" << element.num_pos() << "\n";
}
ALICE_DESCRIBE_STORE(iFPGA::aig_network, element) {
    return fmt::format("{} nodes", element.size());
}

// add klut network cache to the command environment
ALICE_ADD_STORE(iFPGA::klut_network, "lut", "l", "LUT", "LUTs")
ALICE_PRINT_STORE(iFPGA::klut_network, os, element) {
    os << "KLUT PI/PO =" << element.num_pis() << "/" << element.num_pos() << "\n";
}
ALICE_DESCRIBE_STORE(iFPGA::klut_network, element) {
    return fmt::format("{} nodes", element.size());
}

// add klut network cache to the command environment
ALICE_ADD_STORE(iFPGA::write_verilog_params, "port", "p", "PORT", "PORTs")
ALICE_PRINT_STORE(iFPGA::write_verilog_params, os, element) {
    os << "Module name with PI/PO =" << element.module_name << element.input_names.size() << "/" << element.output_names.size() << "\n";
}
ALICE_DESCRIBE_STORE(iFPGA::write_verilog_params, element) {
    return fmt::format("Module name {} with PI/PO = {}/{}", element.module_name, element.input_names.size(), element.output_names.size());
}

// IOs
// ALICE_ADD_FILE_TYPE(aiger, "aiger");
// ALICE_READ_FILE(iFPGA::aig_network, aiger, filename, cmd) {
//     iFPGA::aig_network aig;
//     iFPGA::write_verilog_params ports;
//     iFPGA::Reader reader(filename, aig, ports);
//     return aig;
// }
// ALICE_WRITE_FILE(iFPGA::aig_network, aiger, element, filename, cmd) {
//     iFPGA::Writer writer(element, iFPGA::klut_network());
//     writer.write2AIG(filename);
// }

// ALICE_ADD_FILE_TYPE(klut, "klut");
// ALICE_WRITE_FILE(iFPGA::klut_network, klut, element, filename, cmd) {
//     iFPGA::Writer writer(iFPGA::aig_network(), element);
//     writer.write2LUT(filename);
// }

// ALICE_ADD_FILE_TYPE(verilog, "verilog");
// ALICE_WRITE_FILE(iFPGA::aig_network, verilog, element, filename, cmd) {
//     iFPGA::Writer writer(element, iFPGA::klut_network());
//     writer.write2Verilog(filename);
// }

// ALICE_ADD_FILE_TYPE(dot, "dot");
// ALICE_WRITE_FILE(iFPGA::aig_network, dot, element, filename, cmd) {
//     iFPGA::Writer writer(element, iFPGA::klut_network());
//     writer.write2Dot(filename);
// }
}; // namespace alice