/**
 * @file debugger.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-11-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include "ifpga_namespaces.hpp"
#include "detail/checker_4_mapped_graph.hpp"
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