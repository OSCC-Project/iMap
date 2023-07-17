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

#include "ifpga_namespaces.hpp"
#include "utils/traits.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <cstring>

iFPGA_NAMESPACE_HEADER_START
    namespace detail
    {

        inline void encode(std::vector<unsigned char> &buffer, uint32_t lit)
        {
            unsigned char ch;
            while (lit & ~0x7f)
            {
                ch = (lit & 0x7f) | 0x80;
                buffer.push_back(ch);
                lit >>= 7;
            }
            ch = lit;
            buffer.push_back(ch);
        }

    } // namespace detail

    /*! \brief Writes a combinational AIG network in binary AIGER format into a file
     *
     * This function should be only called on "clean" aig_networks, e.g.,
     * immediately after `cleanup_dangling`.
     *
     * **Required network functions:**
     * - `num_cis`
     * - `num_cos`
     * - `foreach_gate`
     * - `foreach_fanin`
     * - `foreach_po`
     * - `get_node`
     * - `is_complemented`
     * - `node_to_index`
     *
     * \param aig Combinational AIG network
     * \param os Output stream
     */
    template <typename Ntk>
    inline void write_aiger(Ntk const &aig, std::ostream &os)
    {
        static_assert(is_network_type_v<Ntk>, "Ntk is not a network type");
        static_assert(has_num_cis_v<Ntk>, "Ntk does not implement the num_cis method");
        static_assert(has_num_cos_v<Ntk>, "Ntk does not implement the num_cos method");
        static_assert(has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method");
        static_assert(has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method");
        static_assert(has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method");
        static_assert(has_get_node_v<Ntk>, "Ntk does not implement the get_node method");
        static_assert(has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method");

        assert(aig.is_combinational() && "Network has to be combinational");

        using node = typename Ntk::node;
        using signal = typename Ntk::signal;

        uint32_t const M = aig.num_cis() + aig.num_gates();

        /* HEADER */
        char string_buffer[1024];
        sprintf(string_buffer, "aig %u %u %u %u %u\n", M, aig.num_pis(), /*latches*/ 0, aig.num_pos(), aig.num_gates());
        os.write(&string_buffer[0], sizeof(unsigned char) * std::strlen(string_buffer));

        /* POs */
        aig.foreach_po([&](signal const &f)
                       {
    sprintf( string_buffer, "%u\n", uint32_t( 2 * aig.node_to_index( aig.get_node( f ) ) + aig.is_complemented( f ) ) );
    os.write( &string_buffer[0], sizeof( unsigned char ) * std::strlen( string_buffer ) ); });

        /* GATES */
        std::vector<unsigned char> buffer;
        aig.foreach_gate([&](node const &n)
                         {
    std::vector<uint32_t> lits;
    lits.push_back( 2 * aig.node_to_index( n ) );

    aig.foreach_fanin( n, [&]( signal const& fi ) {
      lits.push_back( 2 * aig.node_to_index( aig.get_node( fi ) ) + aig.is_complemented( fi ) );
    } );

    if ( lits[1] > lits[2] )
    {
      auto const tmp = lits[1];
      lits[1] = lits[2];
      lits[2] = tmp;
    }

    assert( lits[2] < lits[0] );
    detail::encode( buffer, lits[0] - lits[2] );
    detail::encode( buffer, lits[2] - lits[1] ); });

        for (const auto &b : buffer)
        {
            os.put(b);
        }

        /* symbol table */
        if constexpr (has_has_name_v<Ntk> && has_get_name_v<Ntk>)
        {
            aig.foreach_pi([&](node const &i, uint32_t index)
                           {
      if ( !aig.has_name( aig.make_signal( i ) ) )
        return;

      sprintf( string_buffer, "i%u %s\n",
               uint32_t( index ),
               aig.get_name( aig.make_signal( i ) ).c_str() );
      os.write( &string_buffer[0], sizeof( unsigned char ) * std::strlen( string_buffer ) ); });
        }
        if constexpr (has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk>)
        {
            aig.foreach_po([&](signal const &f, uint32_t index)
                           {
      if ( !aig.has_output_name( index ) )
        return;

      sprintf( string_buffer, "o%u %s\n",
               uint32_t( index ),
               aig.get_output_name( index ).c_str() );
      os.write( &string_buffer[0], sizeof( unsigned char ) * std::strlen( string_buffer ) ); });
        }

        /* COMMENT */
        os.put('c');
    }

    /*! \brief Writes a combinational AIG network in binary AIGER format into a file
     *
     * This function should be only called on "clean" aig_networks, e.g.,
     * immediately after `cleanup_dangling`.
     *
     * **Required network functions:**
     * - `num_cis`
     * - `num_cos`
     * - `foreach_gate`
     * - `foreach_fanin`
     * - `foreach_po`
     * - `get_node`
     * - `is_complemented`
     * - `node_to_index`
     *
     * \param aig Combinational AIG network
     * \param filename Filename
     */
    template <typename Ntk>
    inline void write_aiger(Ntk const &aig, std::string const &filename)
    {
        std::ofstream os(filename.c_str(), std::ofstream::out);
        write_aiger(aig, os);
        os.close();
    }

iFPGA_NAMESPACE_HEADER_END