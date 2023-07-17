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
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

#include "utils/ifpga_namespaces.hpp"
#include "network/aig_network.hpp"
#include "utils/util.hpp"

// reader
#include "detail/aiger_reader.hpp"
#include "detail/write_verilog.hpp"

struct ports_names_storage
{
  std::string module_name = "top";
  std::vector<std::pair<std::string, uint32_t>> input_names;
  std::vector<std::pair<std::string, uint32_t>> output_names;
};

iFPGA_NAMESPACE_HEADER_START
// for escaped identifier
bool is_simple_char(char c)
{
  return std::isalpha(c) || c == '_' || c == '$';
}

bool is_simple_identifier(const std::string& name)
{
  assert(!name.empty());
  return (std::isalpha(name[0]) || name[0] == '_')  &&
          std::all_of(name.begin(), name.end(), is_simple_char);
}

std::string to_escaped_identifier(const std::string& name)
{
  std::string escaped(name);
  if(!is_simple_identifier(name))
  {
      escaped = '\\' + name + ' ';
  }
  return escaped;
}

template<typename Ntk>
class aig_reader : public aiger_reader<Ntk> 
{
private:
  mutable write_verilog_params _port_infos;
public:
  explicit aig_reader( Ntk& ntk, NameMap<Ntk>* names = nullptr ) 
    :aiger_reader<Ntk>::aiger_reader(ntk, names) 
  {}

  ~aig_reader() {}
  
  void on_input_name(unsigned index, const std::string& name) const override
  {
    // keep base class manipulation
    this->aiger_reader<Ntk>::on_input_name(index, name);
    // store port name
    _port_infos.input_names.emplace_back(std::make_pair(to_escaped_identifier(name), 0));
  }
  
  void on_output_name(unsigned index, const std::string& name) const override
  {
    // keep base class manipulation
    this->aiger_reader<Ntk>::on_output_name(index, name);
    // store port name
    _port_infos.output_names.emplace_back(std::make_pair(to_escaped_identifier(name), 0));
  }
  
  /**
   * @brief return port names for write verilog
   */
  const write_verilog_params& get_params() const
  { 
    return _port_infos;
  }
}; 


enum FileType{
  aag = 0,
  aig,
  verilog,
  blif
};

/**
 * @brief class Reader for feeding aig network   
 */
template<typename Ntk>
class Reader{
public:
  Reader() { }
  Reader( std::string path_in , Ntk& aig ,write_verilog_params& portnames,FileType type = FileType::aag)
    : _path_in(path_in) , _type(type) , _state(false)
  {
    assert( isFileExist(path_in) );

    /// way1: use NameMap
    NameMap<Ntk> nameMap;
    auto const result = lorina::read_aiger( path_in , aiger_reader<Ntk>{_aig, &nameMap});

    /// way2: use inherited class 
    //aig_reader<iFPGA_NAMESPACE::aig_network> reader(_aig);
    //auto const result = lorina::read_aiger( path_in , reader );

    if( result == lorina::return_code::success )
    {
      // std::cout<< "\tINFO: read the aig file SUCCESS, path from: "<< path_in << std::endl;
      _state = true;
      aig = _aig;
      /// Network + NameMap -> write_verilog_params
      {
        using signal = typename Ntk::signal;
        std::map<uint64_t, uint32_t> signalCount; // for one signal has multiple names
        for(auto input: aig._storage->inputs)
        {
          signal s = signal{input, 0};
          auto names = nameMap[s];
          if(!names.empty())
          {
            auto name = names[signalCount[s.data]++];
            if(!name.empty())
              portnames.input_names.emplace_back(std::make_pair(to_escaped_identifier(name), 0));
          }
        }
        for(auto output : aig._storage->outputs)
        {
          signal s = signal{output.index, output.weight};
          auto names = nameMap[s];
          if(!names.empty())
          {
            auto name = names[signalCount[s.data]++];
            if(!name.empty())
              portnames.output_names.emplace_back(std::make_pair(to_escaped_identifier(name), 0));
          }
        }
      }
    }
    else
    {
      std::cerr<< "\tERROR: read the aig file FAIL, path from: "<< path_in << std::endl;
    }
  }

  /**
   * @brief the deconstrtuctor function
   *  because the aig_network is mainly store in _storage and _events , 
   *  we shold delete it with shared_ptr's reset() function
   */
  ~Reader()
  {
    _state = false;
    _path_in = "";
  }

  inline bool isSuccess(){ return _state; }

  inline const iFPGA_NAMESPACE::aig_network& getNetwork()
  { 
    assert( isSuccess() );
    return _aig; 
  }

private:
  std::string                   _path_in;           // the logic synthesis file's path
  int                           _type;              // the reader file's type
  bool                          _state;             // true: reader a file from _path sucess , false not
  Ntk                           _aig;               // the aig class
};

iFPGA_NAMESPACE_HEADER_END