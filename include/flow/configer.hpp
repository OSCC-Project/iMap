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

#include "json/nlohmann/json.hpp"
#include "utils/util.hpp"
#include <assert.h>

iFPGA_NAMESPACE_HEADER_START

class configer{
public:
  /**
   * @brief the default constructer which can set the param's value, then we can get these value
   * 
   */
  configer(){  }
 
  /**
   * @brief mainly a parser for read the configuration
   * 
   * @param file 
   */
  configer(std::string file): _config_file(file)
  {
    assert( ends_with(file, ".json") == true );
    std::ifstream ifs(file);
    _data = nlohmann::json::parse(ifs);
  } 

public:
  /**
   * @brief Get the value object
   * 
   * @tparam T 
   * @param param_level the params follow the hierarchical relationship
   * 
   * 
   * @return T 
   */
  template<typename T>
  T get_value( std::vector<std::string> param_level){
    nlohmann::json value = _data;
    for(uint i = 0; i < param_level.size(); ++i){
      value = value[ param_level[i] ];
    }
    if( !value.is_null() ){
      return value.get<T>();
    }
    else{
      std::cerr << "config file is wrong!" << std::endl;
      assert(false);
      return (T)NULL;
    }
  }

private:
  nlohmann::json  _data;
  std::string     _config_file;
  
};  // end class configer

iFPGA_NAMESPACE_HEADER_END