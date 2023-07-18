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
#include "utils/ifpga_namespaces.hpp"
#include "network/aig_network.hpp"
#include "network/klut_network.hpp"
#include "utils/util.hpp"

#include "detail/writer_lut.hpp"
#include "detail/write_verilog.hpp"
#include "detail/write_dot.hpp"
#include "detail/write_aiger.hpp"

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <vector>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief class Writer for other format files   
 */
template<typename Aig, typename Klut>
class Writer
{
public:
  Writer(const Aig& aig, const Klut& lut)
    : _aig( aig ), _lut(lut)
  {}

  ~Writer() {}

  bool write2AIG( const std::string& path )
  {
    std::ostringstream out;
    write_aiger(_aig , out);
    std::ofstream fout(path , std::ios::out);
    fout << out.str();
    fout.close();
    if( isFileExist(path) )
    {
      std::cout<< "\tINFO: generate the aiger file SUCCESS, path: "<< path << std::endl;
      return true;
    }
        
    else
    {
      std::cerr << "\tERROR: generate the aiger file FAIL, path: " << path << std::endl;
      return false;
    }
  }

  bool write2Dot( const std::string& path )
  {
    std::ostringstream out;
    write_dot(_aig , out);
    std::ofstream fout(path , std::ios::out);
    fout << out.str();
    fout.close();
    if( isFileExist(path) )
    {
      std::cout<< "\tINFO: generate the dot file SUCCESS, path: "<< path << std::endl;
      return true;
    }
        
    else
    {
      std::cerr<< "\tERROR: generate the dot file FAIL, path: "<< path << std::endl;
      return false;
    }
  }

  bool write2Verilog( const std::string& path, write_verilog_params const& portnames = {})
  {
    std::ostringstream out;
    write_verilog(_aig , out, portnames);
    std::ofstream fout(path , std::ios::out);
    fout << out.str();
    fout.close();
    if( isFileExist(path) )
    {
      std::cout<< "\tINFO: generate the verilog file SUCCESS, path: "<< path << std::endl;
      return true;
    }
        
    else
    {
      std::cerr<< "\tERROR: generate the verilog file FAIL, path: "<< path << std::endl;
      return false;
    }
  }

  bool write2LUT( const std::string& path, write_verilog_params const& portnames = {})
  {
    std::ostringstream out;
    iFPGA_NAMESPACE::write_lut(_lut, out, portnames);
    std::ofstream fout(path , std::ios::out);
    fout << out.str();
    fout.close();
    if( isFileExist(path) )
    {
      std::cout<< "\tINFO: generate the LUT file SUCCESS, path: "<< path << std::endl;
      return true;
    }
        
    else
    {
      std::cerr<< "\tERROR: generate the LUT file FAIL, path: "<< path << std::endl;
      return false;
    }
  }

private:
  Aig _aig;
  Klut _lut;
};  // end class Writer

iFPGA_NAMESPACE_HEADER_END
