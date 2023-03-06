/**
 * @file ifpga.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief The code of FPGA mapping code
 * @version 0.1
 * @date 2021-07-12
 * @copyright Copyright (c) 2021
 */
#include "flow/flow_manager.hpp"
#include "args/args.hxx"
#include "utils/mem_usage.hpp"

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
  manager.run();

  printf("\tPeak memory: %ld bytes\n", getPeakRSS());
  return 0;
}
