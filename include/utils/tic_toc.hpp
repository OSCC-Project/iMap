/**
 * @file tic_toc.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief time analyse class
 * @version 0.1
 * @date 2021-07-12
 * @copyright Copyright (c) 2021
 */
#pragma once
#include "utils/ifpga_namespaces.hpp"

#include <ctime>
#include <chrono>
#include <cstdlib>

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the tic_toc class mainly used to calcute the procedure's execute time.
 */
class tic_toc{
public:
  tic_toc()
  {
    tic();
  }
  
  inline void tic()
  {
    _start = std::chrono::system_clock::now();
  }
  
  inline double toc()
  {
    _end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = _end - _start;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(elapsed_seconds);
    return double( duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
  }
private:
    std::chrono::time_point<std::chrono::system_clock> _start , _end;
};  /// end class tic_toc

iFPGA_NAMESPACE_HEADER_END

