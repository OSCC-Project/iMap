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

#pragma once
#include "io/reader.hpp"
#include "io/writer.hpp"

#include "optimization/rewrite.hpp"
#include "optimization/and_balance.hpp"
#include "optimization/refactor.hpp"

#include "network/aig_network.hpp"
#include "network/klut_network.hpp"
#include "algorithms/network_to_klut.hpp"
#include "algorithms/choice_computation.hpp"
#include "algorithms/choice_miter.hpp"

#include "algorithms/klut_mapping.hpp"
#include "utils/util.hpp"
#include "algorithms/debugger.hpp"

#include "views/depth_view.hpp"
#include "optimization/balancing/sop_balancing.hpp"

#include "configer.hpp"

#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include <vector>
#include <string>
#include <tuple>
#include <limits>

iFPGA_NAMESPACE_HEADER_START
/**
 * @brief the toggles to control the mapping flow
 */
struct flow_manager_params
{
  bool      b_debug{false};
  bool      b_use_balance{false};
  bool      b_use_rewrite{false};
  uint32_t  i_opt_iterations{2};
  bool      b_verbose{true};
  bool      b_very_verbose{false};
};

struct flow_manager
{
public:
  flow_manager(std::string const & paht_choice, 
               std::string const & path_configuration,
               std::string const & path_path_output_verilog, 
               std::string const & path_path_output_lut)
  : _path_input(paht_choice), 
    _path_configuration(path_configuration), 
    _path_output_verilog(path_path_output_verilog), 
    _path_output_lut(path_path_output_lut)
  {  }

  /**
   * @brief the main process flow
   */
  void run()
  {
    tic_toc tt;

    printf("\033[0;32;47m [STEP]: Configuration ing... \033[0m \n");
    /// configure params
    configure();
    
    printf("\033[0;32;47m [STEP]: Load network ing... \033[0m \n");
    /// store input networks
    if( read_in(_path_input) == false ){
      std::cerr << "Load input wrong!" << std::endl;
    }
    if( _ps.b_verbose){
      print_circuit();
    }
    

    printf("\033[0;32;47m [STEP]: Technology mapping ing... \033[0m \n");
    /// technology mapping

    auto res = mapper();
    report(std::get<0>(res), std::get<2>(res));
    
    if(_ps.b_debug)
    {
      printf("\033[0;32;47m [STEP]: Formal Verfication ing... \033[0m \n");
      std::cout << "Checking by the miter of klut network and origianl aig network:" << std::endl;
      auto klut_aig = convert_klut_to_aig(std::get<0>(res));
      if (debug_by_miter<iFPGA_NAMESPACE::aig_network, iFPGA_NAMESPACE::aig_network>(klut_aig, _original_aig))
      {
        std::cout << "\tequivalent" << std::endl;
      }
      else
      {
        std::cout << "\tunequivalent" << std::endl;
      }
    }

    /// write out the results
    printf("\033[0;32;47m [STEP]: Download result ing... \033[0m \n");
    if( write_out(std::get<1>(res), std::get<0>(res)) == false){
      std::cerr << "Download ouputs wrong!" << std::endl;
    }
  }

private:
  /**
   * @brief config the params we set
   *
   * @return true
   * @return false
   */
  bool configure()
  {
    configer _configer(_path_configuration);
    _ps.b_debug = _configer.get_value<bool>({"flow_manager", "debug"});
    _ps.i_opt_iterations = _configer.get_value<uint>({"flow_manager", "iterations"});
    _ps.b_use_balance = _configer.get_value<bool>({"flow_manager", "use_balance"});
    _ps.b_use_rewrite = _configer.get_value<bool>({"flow_manager", "use_rewrite"});
    _ps.b_verbose = _configer.get_value<bool>({"flow_manager", "verbose"});
    _ps.b_very_verbose = _configer.get_value<bool>({"flow_manager", "very_verbose"});

    _ps_mapper.cut_enumeration_ps.cut_size = _configer.get_value<uint>({"klut_mapping", "cut_size"});
    _ps_mapper.cut_enumeration_ps.cut_limit = _configer.get_value<uint>({"klut_mapping", "cut_limit"});
    _ps_mapper.uFlowIters = _configer.get_value<uint>({"klut_mapping", "uGlobal_round"});
    _ps_mapper.uAreaIters = _configer.get_value<uint>({"klut_mapping", "uLocal_round"});
    _ps_mapper.bDebug = _configer.get_value<bool>({"klut_mapping", "debug"});
    _ps_mapper.verbose = _configer.get_value<bool>({"klut_mapping", "verbose"});

    _ps_rewrite.cut_enumeration_ps.cut_size = _configer.get_value<uint>({"rewrite", "cut_size"});
    _ps_rewrite.cut_enumeration_ps.cut_limit = _configer.get_value<uint>({"rewrite", "cut_limit"});
    _ps_rewrite.cut_enumeration_ps.minimize_truth_table = _configer.get_value<uint>({"rewrite", "min_candidate_cut_size"});
    _ps_rewrite.b_use_zero_gain = _configer.get_value<bool>({"rewrite", "use_zero_gain"});
    _ps_rewrite.b_preserve_depth = _configer.get_value<bool>({"rewrite", "preserve_depth"});

    return true;
  }

  /**
   * @brief implement the compress2 algorithm refer to abc compress2
   *
   */
  iFPGA_NAMESPACE::aig_network dch_compress(iFPGA_NAMESPACE::aig_network const &aig)
  {
    iFPGA_NAMESPACE::aig_network caig(aig);
    iFPGA_NAMESPACE::rewrite_params ps_rewrite;
    iFPGA_NAMESPACE::refactor_params ps_refactor;
    ps_rewrite.b_preserve_depth = true;
    ps_rewrite.b_use_zero_gain = false;
    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);

    // caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

    caig = iFPGA_NAMESPACE::balance_and(caig);

    ps_rewrite.b_use_zero_gain = true;
    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);
    return caig;
  }

  /**
   * @brief implement the compress2 algorithm refer to abc compress2
   *
   */
  iFPGA_NAMESPACE::aig_network dch_compress2(iFPGA_NAMESPACE::aig_network const &aig)
  {
    iFPGA_NAMESPACE::aig_network caig(aig);
    iFPGA_NAMESPACE::rewrite_params ps_rewrite;
    iFPGA_NAMESPACE::refactor_params ps_refactor;
    ps_rewrite.b_preserve_depth = false;
    ps_rewrite.b_use_zero_gain = false;
    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);
    ps_rewrite.b_preserve_depth = true;

    // caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

    caig = iFPGA_NAMESPACE::balance_and(caig);

    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);

    ps_rewrite.b_use_zero_gain = true;
    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);

    caig = iFPGA_NAMESPACE::balance_and(caig);

    // caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

    caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);

    caig = iFPGA_NAMESPACE::balance_and(caig);
    return caig;
  }

  /**
   * @brief merge the choice miter for choice flow
   * 
   */
  iFPGA_NAMESPACE::aig_with_choice choice_synthesis()
  {
    _compress_aig = dch_compress(_original_aig);
    _compress2_aig = dch_compress2(_compress_aig);

    choice_miter cm;
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_compress2_aig));
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_compress_aig));
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_original_aig));

    _merged_aig = cm.merge_aigs_to_miter();

    choice_params params;
    choice_computation cc(params, _merged_aig);
    return cc.compute_choice();
  }

  /**
   * @brief merge the choice miter for choice flow
   * 
   */
  iFPGA_NAMESPACE::aig_with_choice choice_synthesis_abc()
  {
    write_verilog_params port_names_tmp;
    iFPGA_NAMESPACE::Reader reader1(_path_input + "/compress.aig", _compress_aig, port_names_tmp);
    iFPGA_NAMESPACE::Reader reader2(_path_input + "/compress2.aig", _compress2_aig, port_names_tmp);
    assert(reader1.isSuccess() && reader2.isSuccess());

    choice_miter cm;
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_compress2_aig));
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_compress_aig));
    cm.add_aig(std::make_shared<iFPGA_NAMESPACE::aig_network>(_original_aig));

    _merged_aig = cm.merge_aigs_to_miter();

    choice_params params;
    choice_computation cc(params, _merged_aig);
    return cc.compute_choice();
  }

  /**
   * @brief the FPGA mapper
   * @return return the klut_network, mapped_aig and the QoR
   */
  std::tuple<iFPGA_NAMESPACE::klut_network, mapping_view< iFPGA_NAMESPACE::aig_network, true>, iFPGA_NAMESPACE::mapping_qor_storage> 
  mapper()
  {
    aig_with_choice awc(1u);

    if (_path_input.find(".aig") != std::string::npos)
    {
      awc = choice_synthesis();
    }
    else
    {
      awc = choice_synthesis_abc();
    }

    mapping_view<iFPGA_NAMESPACE::aig_with_choice, true, false> mapped_aig(awc);

    auto qor = iFPGA_NAMESPACE::klut_mapping<decltype(mapped_aig), true>(mapped_aig, _ps_mapper);

    return std::make_tuple(*iFPGA_NAMESPACE::choice_to_klut<iFPGA_NAMESPACE::klut_network>( mapped_aig ), mapped_aig, qor);
  }

  /**
   * @brief loaed the network into the member network
   *
   * @param path the input choice folder
   * @return true read success
   * @return false otherwise
   */
  bool read_in(const std::string &path)
  {

    write_verilog_params port_names;
    if( path.find(".aig") != std::string::npos )
    {
      iFPGA_NAMESPACE::Reader reader(path , _original_aig, _port_names);
      if (reader.isSuccess())
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      iFPGA_NAMESPACE::Reader reader(path + "/original.aig", _original_aig, _port_names);
      if (reader.isSuccess())
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }

  /**
   * @brief loaed the network into the member network
   */
  bool write_out(const iFPGA_NAMESPACE::aig_network &aig, const iFPGA_NAMESPACE::klut_network &klut)
  {
    // make sure the output path is exsit, if not, we will mkdir
    auto idx = _path_output_lut.find("/");
    if (idx != std::string::npos)
    {
      std::size_t pos_end_col = _path_output_lut.find_last_of("/");
      std::string command_mkdir = "mkdir -p " + _path_output_lut.substr(0, pos_end_col);
      [[maybe_unused]] int res = system(command_mkdir.c_str());
    }
    idx = _path_output_verilog.find("/");
    if (idx != std::string::npos)
    {
      std::size_t pos_end_col = _path_output_verilog.find_last_of("/");
      std::string command_mkdir = "mkdir -p " + _path_output_verilog.substr(0, pos_end_col);
      [[maybe_unused]] int rest = system(command_mkdir.c_str());
    }

    // create the output files
    iFPGA_NAMESPACE::Writer writer(aig, klut);

    if(_ps.b_debug)
    {
      writer.write2Dot(_path_input + "/awc.dot");
      writer.write2AIG(_path_input + "/awc.aig");
    }

    if (_path_output_verilog.empty())
    {
      _path_output_verilog = _path_input + "_ifpga" + ".v";
    }
    writer.write2Verilog(_path_output_verilog, _port_names);
    if (_path_output_lut.empty())
    {
      _path_output_lut = _path_input + "_ifpga_lut" + ".v";
    }
    writer.write2LUT(_path_output_lut, _port_names);

    return true;
  }

  /**
   * @brief report the QoR for comparation
   */
  void report(const klut_network& klut, iFPGA_NAMESPACE::mapping_qor_storage qor)
  {
    printf("\tReport mapping result:\n\t\tArea :   %d\n \t\tDelay:   %.0f\n", klut.num_gates(), qor.delay);
    printf("\tLUTs statics:\n");
    std::vector<uint32_t> lut_num = klut.get_Statics_LUT(_ps_mapper.cut_enumeration_ps.cut_size );
    if(lut_num.size() == 0)
    {
      printf("\t\tNO LUT\n");
    }
    for(uint i = 0; i < lut_num.size(); ++i)
    {
      printf("\t\tLUT fanins:%d\t numbers :%u\n", i+1, lut_num[i]);
    }
  }

  /**
   * @brief printf the ports information
   * 
   */
  void print_circuit()
  {
    printf("\033[0;32;40m Circuit-Information: \033[0m \n");
    printf("PORTS infor:\n");
    printf("module name: %s\n", _port_names.module_name.c_str());
    printf("input ports(%lu)\n", _port_names.input_names.size());
    if(_ps.b_very_verbose)
    {
      for(auto input : _port_names.input_names){
        if(input.second == 0u){
          printf("%s,  ", input.first.c_str());
        }
        else{
          std::string bus_port = input.first + "[" + std::to_string(input.second-1) + ":0]";
          printf("%s,  ", bus_port.c_str());
        }
      }
      printf("\n");
    }
    printf("output ports(%lu)\n  ", _port_names.output_names.size());
    if(_ps.b_very_verbose)
    {
      for(auto output : _port_names.output_names){
        if(output.second == 0u){
          printf("%s,  ", output.first.c_str());
        }
        else{
          std::string bus_port = output.first + "[" + std::to_string(output.second-1) + ":0]";
          printf("%s,  ", bus_port.c_str());
        }
      }
      printf("\n");
    }
    printf("Network Size:\n");
    printf("  original  network: %u\n", _original_aig.size());
    printf("  compress  network: %u\n", _compress_aig.size());
    printf("  compress2 network: %u\n", _compress2_aig.size());
    printf("  merged    network: %u\n", _merged_aig.size());
  }

private:

  /// mapping inputs
  iFPGA_NAMESPACE::aig_network          _merged_aig;
  iFPGA_NAMESPACE::aig_network          _original_aig;
  iFPGA_NAMESPACE::aig_network          _compress_aig;
  iFPGA_NAMESPACE::aig_network          _compress2_aig;
  write_verilog_params                  _port_names;

  flow_manager_params                   _ps;
  iFPGA_NAMESPACE::klut_mapping_params  _ps_mapper;
  iFPGA_NAMESPACE::rewrite_params       _ps_rewrite;
  iFPGA_NAMESPACE::refactor_params      _ps_refactor;

  /// configurations
  std::string                _path_input;
  std::string                _path_configuration;         // config for comsumer
  std::string                _path_output_verilog;
  std::string                _path_output_lut;
};  // end class map_manager

iFPGA_NAMESPACE_HEADER_END 