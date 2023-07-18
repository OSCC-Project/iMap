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

#include <cstdint>
#include <iostream>
#include <string>

#include <fmt/format.h>
#include "utils/ifpga_namespaces.hpp"

iFPGA_NAMESPACE_HEADER_START

/*! \brief Prints progress bars.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      { // some block
        progress_bar bar( 100, "|{0}| neg. index = {1}, index squared = {2}" );

        for ( auto i = 0; i < 100; ++i )
        {
          bar( i, -i, i * i );
        }
      } // progress bar is deleted at exit of this block
   \endverbatim
 */
class progress_bar
{
public:
  /*! \brief Constructor.
   *
   * This constructor is used when a the total number of iterations is known,
   * and the progress should be visually printed.  When using this constructor,
   * the current iteration must be provided to the `operator()` call.
   *
   * \param size Number of iterations (for progress bar)
   * \param fmt Format strind; used with `fmt::format`, first placeholder `{0}`
   *            is used for progress bar, the others for the parameters passed
   *            to `operator()`
   * \param enable If true, output is printed, otherwise not
   * \param os Output stream
   */
  progress_bar( uint32_t size, std::string const& fmt, bool enable = true, std::ostream& os = std::cout )
      : _show_progress( true ),
        _size( size ),
        _fmt( fmt ),
        _enable( enable ),
        _os( os ) {}

  /*! \brief Constructor.
   *
   * This constructor is used when a the total number of iterations is not
   * known.  In this case, the progress is not visually printed. When using this
   * constructor, just pass the arguments for the `fmt` string.
   *
   * \param fmt Format strind; used with `fmt::format`, parameters are passed
   *            to `operator()`
   * \param enable If true, output is printed, otherwise not
   * \param os Output stream
   */
  progress_bar( std::string const& fmt, bool enable = true, std::ostream& os = std::cout )
      : _show_progress( false ),
        _fmt( fmt ),
        _enable( enable ),
        _os( os ) {}

  /*! \brief Deconstructor
   *
   * Will remove the last printed line and restore the cursor.
   */
  ~progress_bar()
  {
    done();
  }

  /*! \brief Prints and updates the progress bar status.
   *
   * This updates the progress and re-prints the progress line.  The previous
   * print of the line is removed.  If the progress bar has a progress segment
   * the first argument must be an integer indicating the current progress
   * with respect to the `size` parameter passed to the constructor.
   *
   * \param first Progress position or first argument
   * \param args Vardiadic argument pack with values for format string
   */
  template<typename First, typename... Args>
  void operator()( First first, Args... args )
  {
    if ( _show_progress )
    {
      show_with_progress( first, args... );
    }
    else
    {
      show_without_progress( first, args... );
    }
  }

  /*! \brief Removes the progress bar.
   *
   * This method is automatically invoked when the progress bar is deleted.  In
   * some cases, one may wish to invoke this method manually.
   */
  void done()
  {
    if ( _enable )
    {
      _os << "\u001B[G" << std::string( 79, ' ' ) << "\u001B[G\u001B[?25h" << std::flush;
    }
  }

private:
  template<typename... Args>
  void show_with_progress( uint32_t pos, Args... args )
  {
    if ( !_enable )
      return;

    int spidx = static_cast<int>( ( 6.0 * pos ) / _size );
    _os << "\u001B[G" << fmt::format( _fmt, spinner.substr( spidx * 5, 5 ), args... ) << std::flush;
  }

  template<typename... Args>
  void show_without_progress( Args... args )
  {
    if ( !_enable )
      return;

    _os << "\u001B[G" << fmt::format( _fmt, args... ) << std::flush;
  }

private:
  bool _show_progress;
  uint32_t _size;
  std::string _fmt;
  bool _enable;
  std::ostream& _os;

  std::string spinner{"     .    ..   ...  .... ....."};
};

iFPGA_NAMESPACE_HEADER_END