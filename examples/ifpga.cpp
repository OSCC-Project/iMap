// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Shanghai Anlogic Infotech Co.,Ltd.
// Copyright (c) 2023-2025 Peking University
//
// iMAP-FPGA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************

#include "flow/flow_manager.hpp"
#include "args/args.hxx"
#include "utils/mem_usage.hpp"
#include "utils/tic_toc.hpp"

#include <iostream>
#include <cstdlib>

int main(int argc, char** argv){
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //        HEADER
  ////////////////////////////////////////////////////////////////////////////////////////////////
  std::cout << "        ifpga (Identifical FPGA program)       " << std::endl;
  std::cout << "FPGA Logical Synthesis tools.For now ,the architecture is based on our iEDA program." << std::endl;
  std::cout << "We supprot the AIG file IO ,and the result will be writed into dot file for" << std::endl;
  std::cout << "visualization and verilog file for the gate-level netlist.\n" << std::endl;
  ////////////////////////////////////////////////////////////////////////////////////////////////
  //        args && configure
  ////////////////////////////////////////////////////////////////////////////////////////////////
  args::ArgumentParser parser("We only consider the input and configuration files.", "");
  args::HelpFlag help(parser, "help", "display this help console", {'h', "help"});
  args::ValueFlag<std::string> sInput_choice(parser, "path_i", "input path of choice folder", {'i' , "input"});
  args::ValueFlag<std::string> sOutput_verilog(parser, "path_v", "output verilog path", {'v' , "verilog"});
  args::ValueFlag<std::string> sOutput_lut(parser, "path_l", "output lut path", {'l' , "lut"});
  args::ValueFlag<std::string> sInput_config(parser, "path_c", "configutation path", {'c' , "configure"});
  try
  {
    parser.ParseCLI(argc, argv);
  }
  catch (args::Help const&)
  {
    std::cout << parser;
    return -1;
  }
  catch (args::ParseError const& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return -1;
  }
  catch (args::ValidationError const& e)
  {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return -1;
  }

  // for choice-flow support, the input of ifpga is a path include the original.aig, compress.aig, compress2.zig folder
  auto input = args::get(sInput_choice);

  auto config = args::get(sInput_config);

  iFPGA_NAMESPACE::flow_manager manager(input, config, args::get(sOutput_verilog), args::get(sOutput_lut));
  iFPGA_NAMESPACE::tic_toc tt;
  manager.run();
  printf("Mapping time: %f secs\n", tt.toc());
  printf("Peak memory: %ld bytes\n", getPeakRSS());
  return 0;
}