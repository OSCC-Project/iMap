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

#include <chrono>
#include <iostream>
#include <type_traits>

#include <fmt/format.h>

#include "ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

/*! \brief Stopwatch interface
 *
 * This class implements a stopwatch interface to track time.  It starts
 * tracking time at construction and stops tracking time at deletion
 * automatically.  A reference to a duration object is passed to the
 * constructor.  After stopping the time the measured time interval is added
 * to the durationr reference.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      { // some block
        stopwatch t( time );

        // do some work
      } // stopwatch is stopped here

      std::cout << fmt::format( "{:5.2f} seconds passed\n", to_seconds( time ) );
   \endverbatim
 */
template<class Clock = std::chrono::steady_clock>
class stopwatch
{
public:
  using clock = Clock;
  using duration = typename Clock::duration;
  using time_point = typename Clock::time_point;

  /*! \brief Default constructor.
   *
   * Starts tracking time.
   */
  explicit stopwatch( duration& dur )
      : dur( dur ),
        beg( clock::now() )
  {
  }

  /*! \brief Default deconstructor.
   *
   * Stops tracking time and updates duration.
   */
  ~stopwatch()
  {
    dur += ( clock::now() - beg );
  }

private:
  duration& dur;
  time_point beg;
};

/*! \brief Calls a function and tracks time.
 *
 * The function that is passed as second parameter can be any callable object
 * that takes no parameters.  This construction can be used to avoid
 * pre-declaring the result type of a computation that should be tracked.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      auto result = call_with_stopwatch( time, [&]() { return function( parameters ); } );
   \endverbatim
 *
 * \param dur Duration reference (time will be added to it)
 * \param fn Callable object with no arguments
 */
template<class Fn, class Clock = std::chrono::steady_clock>
std::invoke_result_t<Fn> call_with_stopwatch( typename Clock::duration& dur, Fn&& fn )
{
  stopwatch<Clock> t( dur );
  return fn();
}

/*! \brief Constructs an object and calls time.
 *
 * This function can track the time for the construction of an object and
 * returns the constructed object.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      stopwatch<>::duration time{0};

      // create vector with 100000 elements initialized to 42
      auto result = make_with_stopwatch<std::vector<int>>( time, 100000, 42 );
   \endverbatim
 */
template<class T, class... Args, class Clock = std::chrono::steady_clock>
T make_with_stopwatch( typename Clock::duration& dur, Args... args )
{
  stopwatch<Clock> t( dur );
  return T{std::forward<Args>( args )...};
}

/*! \brief Utility function to convert duration into seconds. */
template<class Duration>
inline double to_seconds( Duration const& dur )
{
  return std::chrono::duration_cast<std::chrono::duration<double>>( dur ).count();
}

template<class Clock = std::chrono::steady_clock>
class print_time
{
public:
  print_time()
      : _t( new stopwatch<Clock>( _d ) )
  {
  }

  ~print_time()
  {
    delete _t;
    std::cout << fmt::format( "[i] run-time: {:5.2f} secs\n", to_seconds( _d ) );
  }

private:
  stopwatch<Clock>* _t{nullptr};
  typename stopwatch<Clock>::duration _d{};
};

iFPGA_NAMESPACE_HEADER_END