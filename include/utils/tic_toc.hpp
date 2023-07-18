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

