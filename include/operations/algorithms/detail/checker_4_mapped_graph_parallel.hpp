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
#include "kitty/cnf.hpp"
#include "kitty/dynamic_truth_table.hpp"
#include "percy/solvers/bsat2.hpp"
#include "operations/algorithms/cnf.hpp"

#include "utils/tic_toc.hpp"

#include "fmt/format.h"

#include <cstring>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <thread>
#include <omp.h>

iFPGA_NAMESPACE_HEADER_START

class my_sat_solver
{
public:
    explicit my_sat_solver()
    {
        _current_sat_var = 1u;
        _solver.set_nr_vars(1000);
        _sat_vars_map[0] = _current_sat_var++; // constant 0
    }

    percy::synth_result solve(uint32_t conflict_limit)
    {
        return _solver.solve(conflict_limit);
    }

    uint32_t incr_current_sat_var()
    {
        return _current_sat_var++;
    }

    /**
     * @brief get the sat_vars of a node
     *
     * @param n
     * @return uint32_t
     */
    uint32_t get_sat_vars_of_node(uint32_t const &n)
    {
        return _sat_vars_map[n];
    }

    /**
     * @brief auto set the vars of a node
     *
     * @param n
     * @return uint32_t
     */
    uint32_t set_sat_vars_of_node(uint32_t const &n)
    {
        _sat_vars_map[n] = _current_sat_var++;
        return _sat_vars_map[n];
    }

    /**
     * @brief prepare the node-variables mapping
     *
     * @param n
     * @return uint32_t
     */
    uint32_t prepare_node_for_sat(uint32_t const &n)
    {
        if (get_sat_vars_of_node(n) <= 1u)
        {
            set_sat_vars_of_node(n);
        }
        return get_sat_vars_of_node(n);
    }

    void add_clause(const std::vector<uint32_t> &lits)
    {
        _solver.add_clause(lits);
    }

    uint32_t add_and2_to_solver(uint32_t const &n, uint32_t child0, bool compl0, uint32_t child1, bool compl1)
    {
        uint32_t var_node = prepare_node_for_sat(n);
        uint32_t var_child0 = prepare_node_for_sat(child0);
        uint32_t var_child1 = prepare_node_for_sat(child1);

        // add clause on and node
        std::vector<uint32_t> lits1(2, 0);
        std::vector<uint32_t> lits2(2, 0);
        std::vector<uint32_t> lits3(3, 0);

        /**
         * c = a && b
         * (a + !c)(b + !c)(!a + !b + c)
         */
        lits1[0] = make_lit(var_child0, false ^ compl0);
        lits1[1] = make_lit(var_node, true);

        lits2[0] = make_lit(var_child1, false ^ compl1);
        lits2[1] = make_lit(var_node, true);

        lits3[0] = make_lit(var_child0, true ^ compl0);
        lits3[1] = make_lit(var_child1, true ^ compl1);
        lits3[2] = make_lit(var_node, false);

        _solver.add_clause(lits1);
        _solver.add_clause(lits2);
        _solver.add_clause(lits3);

        return var_node;
    }

    void add_compl_equal_to_solver(uint32_t const &var1, uint32_t const &var2)
    {
        uint32_t var_node = _current_sat_var++; // the dummy node, because of the 2 choice nodes do not have an ancestor
        (void) var_node;
        std::vector<uint32_t> lits(2, 0);
        lits[0] = make_lit(var1, false);
        lits[1] = make_lit(var2, false);
        int rett = _solver.add_clause(lits);
        assert(rett);
        (void)rett;
        lits[0] = make_lit(var1, true);
        lits[1] = make_lit(var2, true);
        rett = _solver.add_clause(lits);
        assert(rett);
        (void)rett;
    }

    void add_equal_to_solver(uint32_t const &var1, uint32_t const &var2)
    {
        uint32_t var_node = _current_sat_var++; // the dummy node, because of the 2 choice nodes do not have an ancestor
        (void) var_node;
        std::vector<uint32_t> lits(2, 0);
        lits[0] = make_lit(var1, true);
        lits[1] = make_lit(var2, false);
        int rett = _solver.add_clause(lits);
        assert(rett);
        (void)rett;
        lits[0] = make_lit(var1, false);
        lits[1] = make_lit(var2, true);
        rett = _solver.add_clause(lits);
        assert(rett);
        (void)rett;
    }

private:
    percy::bsat_wrapper _solver;
    std::unordered_map<uint32_t, uint32_t> _sat_vars_map;
    uint32_t _current_sat_var;
};

template <typename Ntk>
class checker_4_mapped_graph_parallel
{
public:
    using node = typename Ntk::node;
    using signal = typename Ntk::signal;
    static constexpr node AIG_NULL = Ntk::AIG_NULL;

    explicit checker_4_mapped_graph_parallel(Ntk const &ntk)
        : _ntk(ntk)
    {
    }

public:
    bool run()
    {

        bool error = false;
        uint32_t i = 0;
        std::vector<node> CNS;
        _ntk.foreach_gate([&](auto const &n)
                          { 
                            if ( _ntk.is_cell_root(n) )
                            {
                                CNS.emplace_back(n);
                            } });
        collect_fanouts();
        collect_choices();

        uint size = CNS.size();
        tic_toc tt;
#if 1 // direct parallel
        printf("start-x: ");
        error = false;
        omp_set_num_threads(88);
        tt.tic();
#pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
        for (i = 0; i < size; ++i)
        {
        //     if(error)
        //         continue;
        //     else
        //     {
                if (!is_equal_cell(CNS[i]))
                {
                    error = true;
                }
            // }
        }
        error ? printf("nonequivalent\t") : printf("equivalent\t");
        printf("time-x: %f \t end-x\n", tt.toc());


//         printf("start-2x: ");
//         error = false;
//         omp_set_num_threads(2);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-2x: %f \t end-2x\n", tt.toc());

//         printf("start-4x: ");
//         error = false;
//         omp_set_num_threads(4);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-4x: %f \t end-4x\n", tt.toc());

//         printf("start-8x: ");
//         error = false;
//         omp_set_num_threads(8);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-8x: %f \t end-8x\n", tt.toc());

//         printf("start-16x: ");
//         error = false;
//         omp_set_num_threads(16);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-16x: %f \t end-16x\n", tt.toc());

//         printf("start-24x: ");
//         error = false;
//         omp_set_num_threads(24);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-24x: %f \t end-24x\n", tt.toc());

//         printf("start-32x: ");
//         error = false;
//         omp_set_num_threads(32);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-32x: %f \t end-32x\n", tt.toc());

//         printf("start-40x: ");
//         error = false;
//         omp_set_num_threads(40);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-40x: %f \t end-40x\n", tt.toc());

//         printf("start-48x: ");
//         error = false;
//         omp_set_num_threads(48);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-48x: %f \t end-48x\n", tt.toc());

//         printf("start-56x: ");
//         error = false;
//         omp_set_num_threads(56);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-56x: %f \t end-56x\n", tt.toc());

//         printf("start-64x: ");
//         error = false;
//         omp_set_num_threads(64);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-64x: %f \t end-64x\n", tt.toc());

//         printf("start-72x: ");
//         error = false;
//         omp_set_num_threads(72);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-72x: %f \t end-72x\n", tt.toc());

//         printf("start-80x: ");
//         error = false;
//         omp_set_num_threads(80);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-80x: %f \t end-80x\n", tt.toc());

//         printf("start-88x: ");
//         error = false;
//         omp_set_num_threads(88);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-88x: %f \t end-88x\n", tt.toc());

//         printf("start-96x: ");
//         error = false;
//         omp_set_num_threads(96);
//         tt.tic();
// #pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
//         for (i = 0; i < size; ++i)
//         {
//             if(error)
//                 continue;
//             else
//             {
//                 if (!is_equal_cell(CNS[i]))
//                 {
//                     error = true;
//                 }
//             }
//         }
//         error ? printf("nonequivalent\t") : printf("equivalent\t");
//         printf("time-96x: %f \t end-96x\n", tt.toc());

        return !error;

#else   // topo-order parallel

        printf("direct random parallel:\n");
        printf("start-48x: ");
        error = false;
        omp_set_num_threads(48);
        tt.tic();
#pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
        for (i = 0; i < size; ++i)
        {
            if(error)
                continue;
            else
            {
                if (!is_equal_cell(CNS[i]))
                {
                    error = true;
                }
            }
        }
        error ? printf("nonequivalent\t") : printf("equivalent\t");
        printf("time-48x: %f \t end-48x\n", tt.toc());

        printf("Topo parallel:\n");
        uint32_t j = 0;
        uint32_t batch_size = 48;
        printf("start-topo-x: ");
        error = false;
        omp_set_num_threads(48);
        for (i = 0; i < size; i += batch_size)
        {
            uint32_t begin = i;
            uint32_t end = i+batch_size;
            if(end <= size)
            {
#pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
                for(j = begin; j < end ; ++j)
                {
                    if(error)
                        continue;
                    else
                    {
                        if (!is_equal_cell(CNS[j]))
                        {
                            error = true;
                        }
                    }
                }
            }
            else if (end > size)
            {
#pragma omp parallel for shared(error, _ntk, _fanouts, _choice_repr_map)
                for(j = begin; j < size ; ++j)
                {
                    if(error)
                        continue;
                    else
                    {
                        if (!is_equal_cell(CNS[j]))
                        {
                            error = true;
                        }
                    }
                }

            }
        }
        error ? printf("nonequivalent\t") : printf("equivalent\t");
        printf("time-topo-x: %f \t end-topo-x\n", tt.toc());
        return !error;
#endif
    }
private:
    bool is_equal_cell(node const &root)
    {
        my_sat_solver solver;
        kitty::dynamic_truth_table cell_func = _ntk.cell_function(root);
        std::vector<node> leaves;
        _ntk.foreach_cell_fanin(root, [&leaves](auto const &l)
                                { leaves.emplace_back(l); });
        solver.prepare_node_for_sat(root);
        for (auto leaf : leaves)
        {
            solver.prepare_node_for_sat(leaf);
        }

        // add cell function constraint
        uint32_t num_vars = cell_func.num_vars();
        auto clauses = kitty::cnf_characteristic(cell_func);

        auto vars_func_root = solver.incr_current_sat_var();

        for (auto clause : clauses)
        {
            auto bits = clause._bits;
            auto mask = clause._mask;

            std::vector<uint32_t> lits;
            for (uint i = 0u; i < num_vars; ++i)
            {
                if ((mask >> i) & 1)
                {
                    if ((bits >> i) & 1) // 1 - positive, 0 - negative
                    {
                        lits.push_back(make_lit(solver.get_sat_vars_of_node(leaves[i]), false));
                    }
                    else
                    {
                        lits.push_back(make_lit(solver.get_sat_vars_of_node(leaves[i]), true));
                    }
                }
            }
            if ((mask >> num_vars) & 1) // output bit
            {
                if ((bits >> num_vars) & 1)
                {
                    lits.push_back(make_lit(vars_func_root, false));
                }
                else
                {
                    lits.push_back(make_lit(vars_func_root, true));
                }
            }
            solver.add_clause(lits);
        }

        // add subgraph constraint
        // std::set<node> candidates = collect_fanin_cone_nodes_root_to_leaf(root, *min_element(leaves.begin(), leaves.end()));
        std::set<node> candidates = collect_fanin_cone_nodes_leaf_to_root(root, leaves);
        for (auto n : candidates)
        {
            if (_ntk.is_pi(n) || _ntk.is_constant(n))
            {
                continue;
            }

            std::vector<signal> children(2);
            _ntk.foreach_fanin(n, [&](auto const &c, int index)
                               { children[index] = c; });

            solver.add_and2_to_solver(n, _ntk.get_node(children[0]), children[0].complement, _ntk.get_node(children[1]), children[1].complement);
            if (is_choice(n))
            {
                // add constraint
                auto phase = _ntk.phase(n) ^ _ntk.phase(_choice_repr_map[n]);

                if (phase)
                {
                    solver.add_compl_equal_to_solver(solver.prepare_node_for_sat(n), solver.prepare_node_for_sat(_choice_repr_map[n]));
                }
                else
                {
                    solver.add_equal_to_solver(solver.prepare_node_for_sat(n), solver.prepare_node_for_sat(_choice_repr_map[n]));
                }
                n = _choice_repr_map[n]; // propagate through repr node to the fanout cone
            }
        }

        // add xor constraint
        solver.add_compl_equal_to_solver(vars_func_root, solver.prepare_node_for_sat(root));
        auto ret = solver.solve(10000);
        if (ret == percy::failure)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief collect the fanout view of all the node
     *
     */
    void collect_fanouts()
    {
        _ntk.foreach_gate([&](auto const &n)
                          { _ntk.foreach_fanin(n, [&](auto const &c)
                                               { _fanouts[_ntk.get_node(c)].insert(n); }); });
    }

    /**
     * @brief collecn all the choice node and its representation
     *
     */
    void collect_choices()
    {
        _ntk.foreach_gate([&](auto const &n)
                          {
            if ( _ntk.is_repr(n) ){
                auto next_node = _ntk.get_equiv_node(n);
                while( next_node != AIG_NULL ){
                    _choice_repr_map[next_node] = n;
                    next_node = _ntk.get_equiv_node(next_node);
                }
            } });
    }

    /**
     * @brief
     * @param root
     * @param min_limit
     * @return std::set<node>
     */
    std::set<node> collect_fanin_cone_nodes_root_to_leaf(node const &root, node const &min_limit)
    {
        std::set<node> candidates;
        std::queue<node> que;
        que.push(root);
        while (!que.empty())
        {
            auto n = que.front();
            que.pop();
            if (candidates.find(n) != candidates.end())
            {
                continue;
            }

            candidates.insert(n);
            if (_ntk.is_repr(n))
            {
                auto next_node = _ntk.get_equiv_node(n);
                while (next_node != AIG_NULL)
                {
                    que.push(next_node);
                    next_node = _ntk.get_equiv_node(next_node);
                }
            }
            bool conti = true;
            _ntk.foreach_fanin(n, [&](auto const &c)
                               {    
                                if(_ntk.get_node(c) < min_limit)
                                {
                                    conti = false;
                                } });
            if (conti)
            {
                _ntk.foreach_fanin(n, [&](auto const &c)
                                   { que.push(_ntk.get_node(c)); });
            }
        }
        return candidates;
    }

    /**
     * @brief collect fanin cone from leaves to root direction
     *  TODO: fix bugs here!
     * @param root
     * @param leaves
     * @return std::set<node>
     */
    std::set<node> collect_fanin_cone_nodes_leaf_to_root(node const &root, const std::vector<node> &leaves)
    {
        std::set<node> candidates;
        std::queue<node> que;

        for (auto leaf : leaves)
        {
            que.push(leaf);
        }

        while (!que.empty())
        {
            auto n = que.front();
            que.pop();
            if (candidates.find(n) != candidates.end())
            {
                continue;
            }
            candidates.insert(n);

            if (n == root)
            {
                break;
            }

            if (is_choice(n)) // push its representative node
            {
                que.push(_choice_repr_map[n]);
                n = _choice_repr_map[n];
            }
            for (auto fo : _fanouts[n])
            {
                que.push(fo);
            }
        }

        return candidates;
    }

    /**
     * @brief checl whether a node is choice node
     *
     * @param n
     * @return true
     * @return false
     */
    bool is_choice(node const &n)
    {
        return _choice_repr_map.find(n) != _choice_repr_map.end();
    }

private:
    Ntk _ntk;
    std::map<node, std::set<node>> _fanouts;
    std::map<node, node> _choice_repr_map;

}; // end class checker_4_mapped_graph_parallel

iFPGA_NAMESPACE_HEADER_END