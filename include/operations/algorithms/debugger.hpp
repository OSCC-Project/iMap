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
#include "detail/checker_4_mapped_graph.hpp"
#include "detail/checker_4_mapped_graph_parallel.hpp"
#include "detail/convert_to_aig.hpp"
#include "equivalence_checking.hpp"
#include "miter.hpp"


iFPGA_NAMESPACE_HEADER_START

/**
 * @brief checking the mapped aig only by the lut info
 * 
 * @tparam Ntk 
 * @param ntk 
 * @return true 
 * @return false 
 */
template<typename Ntk>
bool debug_mapped_aig_by_origin(Ntk const& ntk)
{
    checker_4_mapped_graph<Ntk> checker(ntk);
    return checker.run();
}

template<typename Ntk>
bool debug_mapped_aig_by_origin_parallel(Ntk const& ntk)
{
    checker_4_mapped_graph_parallel<Ntk> checker(ntk);
    return checker.run();
}

/**
 * @brief checking the mapped aig by miter
 * 
 * @tparam Ntk 
 * @param ntk 
 */
template <typename Ntk>
bool debug_mapped_aig_by_converted_miter(Ntk const &ntk)
{
    auto klut_aig = convert_mapped_graph_to_aig(ntk);
    auto mit = miter<iFPGA_NAMESPACE::aig_network, iFPGA_NAMESPACE::aig_network, iFPGA_NAMESPACE::aig_network>(klut_aig, ntk);
    if (mit.has_value())
    {
        auto equal = equivalence_checking<iFPGA_NAMESPACE::aig_network>(mit.value());
        if (*equal == true)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

template <typename Ntk1, typename Ntk2>
bool debug_by_miter(Ntk1 const &ntk1, Ntk1 const &ntk2)
{
    auto mit = miter<Ntk1, Ntk1, Ntk2>(ntk1, ntk2);
    if (mit.has_value())
    {
        auto equal = equivalence_checking<iFPGA_NAMESPACE::aig_network>(mit.value());
        if (*equal == true)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

iFPGA_NAMESPACE_HEADER_END