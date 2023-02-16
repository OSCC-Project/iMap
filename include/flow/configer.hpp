/**
 * @file configer.hpp
 * @author liwei ni (nlwmode@gmail.com)
 * @brief  configer mainly for parsing configuretion file or some defult configuration
 * @version 0.1
 * @date 2022-09-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
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