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
#include "database/network/aig_network.hpp"
#include "database/network/klut_network.hpp"
#include "kitty/isop.hpp"
#include "kitty/dynamic_truth_table.hpp"

#include <unordered_map>
#include <assert.h>

iFPGA_NAMESPACE_HEADER_START

iFPGA_NAMESPACE::aig_network convert_klut_to_aig(iFPGA_NAMESPACE::klut_network const &klut)
{
    using signal_a = iFPGA_NAMESPACE::aig_network::signal;
    //using signal_k = iFPGA_NAMESPACE::klut_network::signal;
    //using node_a = iFPGA_NAMESPACE::aig_network::node;
    using node_k = iFPGA_NAMESPACE::klut_network::node;

    iFPGA_NAMESPACE::aig_network aig;

    std::unordered_map<node_k, signal_a> klut_aig_map;
    // constant-0 and constant-1
    klut_aig_map[0] = signal_a(0,0);
    klut_aig_map[1] = signal_a(0,1);

    // create pi
    klut.foreach_pi([&](auto const& n){
        assert( klut_aig_map.find(n) == klut_aig_map.end() );
        klut_aig_map[n] = aig.create_pi();
    });

    // create internal nodes
    klut.foreach_gate([&](auto const& n){
        assert( klut.is_function(n) );

        auto cell_func = klut.node_function(n);
        uint nvar = cell_func.num_vars();

        std::vector<node_k> leaves;
        klut.foreach_fanin(n, [&](auto const& fanin){
            leaves.emplace_back(fanin);
        });

        auto cubes = kitty::isop(cell_func);
        /**
         * abc + cd ...
         */
        std::vector<signal_a> nary_or_signal;
        for (auto cb : cubes)
        {
            std::vector<signal_a> nary_and_signal;
            for(uint i = 0u; i < nvar; ++i)
            {
                assert(klut_aig_map.find(leaves[i]) != klut_aig_map.end());
                if(cb.get_mask(i))
                {
                    if(cb.get_bit(i))
                    {
                        nary_and_signal.emplace_back(klut_aig_map[leaves[i]]);
                    }
                    else
                    {
                        nary_and_signal.emplace_back(!klut_aig_map[leaves[i]]);
                    }
                }
            }
            auto tmp_signal = aig.create_nary_and(nary_and_signal);

            nary_or_signal.emplace_back(tmp_signal);
        }
        auto tmp_signal = aig.create_nary_or(nary_or_signal);
        klut_aig_map[n] = tmp_signal;
    });

    // create pos
    klut.foreach_po([&](auto const& s){
        assert( klut_aig_map.find(s) != klut_aig_map.end() );
        aig.create_po( klut_aig_map[s] );
    });

    return aig;
}

template<typename Ntk>
iFPGA_NAMESPACE::aig_network convert_mapped_graph_to_aig(Ntk const &lutnet)
{
    using signal_a = iFPGA_NAMESPACE::aig_network::signal;
    //using node_a   = iFPGA_NAMESPACE::aig_network::node;
    //using signal_m = typename Ntk::signal;
    using node_m   = typename Ntk::node;

    iFPGA_NAMESPACE::aig_network aig;
    std::unordered_map<node_m, signal_a> lut_aig_map;
    
    // constant
    lut_aig_map[0] = signal_a(0, 0);

    // create pi
    lutnet.foreach_pi([&](auto const& n){
        assert( lut_aig_map.find(n) == lut_aig_map.end() );
        lut_aig_map[n] = aig.create_pi();
    });

    // create internal nodes, luts are connected by nodes, so we do not consider the complement
    lutnet.foreach_gate([&](auto const& n){
        if ( lutnet.is_cell_root(n) )
        {
            auto cell_func = lutnet.cell_function(n);
            uint nvar = cell_func.num_vars();
            std::vector<node_m> leaves;
            lutnet.foreach_cell_fanin(n, [&leaves](auto const& l){
                leaves.emplace_back(l);
            });

            auto cubes = kitty::isop(cell_func);

            std::vector<signal_a> nary_or_signal;
            for (auto cb : cubes)
            {
                std::vector<signal_a> nary_and_signal;
                for (uint i = 0u; i < nvar; ++i)
                {
                    assert(lut_aig_map.find(leaves[i]) != lut_aig_map.end());
                    if (cb.get_mask(i))
                    {
                        if (cb.get_bit(i))
                        {
                            nary_and_signal.emplace_back(lut_aig_map[leaves[i]]);
                        }
                        else
                        {
                            nary_and_signal.emplace_back(!lut_aig_map[leaves[i]]);
                        }
                    }
                }
                auto tmp_signal = aig.create_nary_and(nary_and_signal);

                nary_or_signal.emplace_back(tmp_signal);
            }
            auto tmp_signal = aig.create_nary_or(nary_or_signal);
            lut_aig_map[n] = tmp_signal;
        }
    });

    // create pos, we need to consider the complemented of pos
    lutnet.foreach_po([&](auto const& s){
        assert( lut_aig_map.find( lutnet.get_node(s) ) != lut_aig_map.end() );
        if( lutnet.is_complemented(s) )
        {
            aig.create_po(!lut_aig_map[lutnet.get_node(s)]);
        }
        else
        {
            aig.create_po(lut_aig_map[lutnet.get_node(s)]);
        }
    });

    return aig;
}

iFPGA_NAMESPACE_HEADER_END