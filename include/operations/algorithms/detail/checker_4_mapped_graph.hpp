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
#include <map>
#include <unordered_map>
#include <set>

iFPGA_NAMESPACE_HEADER_START

struct params_sat
{
    int  max_conflit_limit{1000};
    int  max_sat_var{5000};
    bool polar_flip{true};
};

template<typename Ntk>
class checker_4_mapped_graph{
public:
    using node   = typename Ntk::node;
    using signal = typename Ntk::signal;
    static constexpr node AIG_NULL = Ntk::AIG_NULL;

    explicit checker_4_mapped_graph(Ntk const &ntk)
        : _ntk(ntk), _constant_max_sat_var(1u)
    {    }

public:

    /**
     * @brief the main run flow
     * 
     * @return true  equivalent
     * @return false not equivalent
     */
    bool run()
    {
        init();
        bool error = false;
        std::cout << "step1: checking each lut ing" << std::endl;
        if( !check_each_luts() )
        {
            error = true;
            std::cout << "\tthe first unequivalent lut at: " << _ue_node << std::endl;
        }
        
        std::cout << "\nstep2: checking each po ing" << std::endl;
        if( !check_each_po() )
        {
            error = true;
            std::cout << "step2.1: checking the unequivalent po (" << _ntk.node_to_index(_ntk.get_node(_ue_signal)) << ", " << _ue_signal.complement << ")" << std::endl;
            [[maybe_unused]] bool equal = check_signal_incrementally(_ue_signal);
            assert(equal == !error);
            std::cout << "\tthe first unequivalent node at: " << _ue_node << std::endl;
        }
        return !error;
    }

    /**
     * @brief check the equivalence of each lut and its covery on original aig
     * 
     * @return true 
     * @return false 
     */
    bool check_each_luts()
    {
        bool error = false;
        uint32_t count = 0, index = 0;

        _ntk.foreach_gate([&](auto const &n)
                        { 
                            if ( _ntk.is_cell_root(n) )
                            {
                                count++;
                            } 
                        });
        _ntk.foreach_gate([&](auto const &n)
                          {
            if( error )
            {
                return;
            }
            if ( _ntk.is_cell_root(n) )
            {
                printf(" [%d/%d]  ", ++index, count);
                auto equal = is_equal_cell(n);
                if( equal == -1 )
                {
                    error = true;
                    _ue_node = n;
                }
                else if(equal == 0)
                {
                    std::cerr << " timeout... " << std::endl;
                }
            } });
        return !error;
    }

    /**
     * @brief check the equivalence of each po
     *
     * @return true
     * @return false
     */
    bool check_each_po()
    {
        bool error = false;
        printf("checking po at: ");
        _ntk.foreach_po([&](auto const &p, int index)
                        {
            if(error)
            {
                return;
            }
            printf(" [%d/%d] ", index, _ntk.num_pos()-1);
            auto root = _ntk.get_node(p);
            if(_ntk.is_pi(root) || _ntk.is_constant(root) )
            {
                return;
            }
            auto equal = is_equal_root(root);
            if(equal == -1)
            {
                error = true;
                _ue_signal = p;
            }
            else if(equal == 0)
            {
                std::cerr << " timeout... " << std::endl;
            }
            });
        printf("\n");
        return !error;
    }

private:
    void init()
    {
        collect_fanouts();
        collect_choices();
        _ps_sat.max_conflit_limit = 1000000;    // conflict may cause timeout
        _ps_sat.max_sat_var       = 1000000;
        _ps_sat.polar_flip        = true;

        _sat_vars_map.assign(_ntk.size(), 0u);
        _solver.set_nr_vars(1000);
        _current_sat_var = _constant_max_sat_var;
        _sat_vars_map[0] = _current_sat_var++;
    }
    
    /**
     * @brief recycle the sat solver for the next solving
     * 
     */
    void recycle_sat()
    {       
        _solver.restart();

        _sat_vars_map.assign(_ntk.size(), 0u);
        _current_sat_var = 1u;
        _sat_vars_map[0] = _current_sat_var++;  // process constant-zero
    }

    /**
     * @brief collect the fanout view of all the node
     * 
     */
    void collect_fanouts()
    {
        _ntk.foreach_gate([&](auto const& n){
            _ntk.foreach_fanin(n, [&](auto const& c){
                _fanouts[ _ntk.get_node(c) ].insert(n);
            });
        });
    }

    /**
     * @brief collecn all the choice node and its representation
     * 
     */
    void collect_choices()
    {
        _ntk.foreach_gate([&](auto const& n){
            if ( _ntk.is_repr(n) ){
                auto next_node = _ntk.get_equiv_node(n);
                while( next_node != AIG_NULL ){
                    _choice_repr_map[next_node] = n;
                    next_node = _ntk.get_equiv_node(next_node);
                }
            }
        });
    }

    /**
     * @brief
     *      TODO: check again please!
     * @param root
     * @param min_limit
     * @return std::set<node>
     */
    std::set<node> collect_limited_fanin_cone_nodes(node const& root, node const& min_limit)
    {
        std::set<node> nodes;
        _ntk.clear_visited();
        std::queue<node> que;
        que.push(root);
        while (!que.empty())
        {
            auto n = que.front();
            que.pop();

            if(_ntk.visited(n))
            {
                continue;
            }

            _ntk.set_visited(n, 1);
            nodes.insert(n);

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
                                }
                               });
            if(conti)
            {
                _ntk.foreach_fanin(n, [&](auto const &c)
                                   { que.push(_ntk.get_node(c)); });
            }
        }
        _ntk.clear_visited();
        return nodes;
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

    /**
     * @brief get the sat_vars of a node
     * 
     * @param n 
     * @return uint32_t 
     */
    uint32_t get_sat_vars_of_node(node const& n)
    {
        return _sat_vars_map[n];
    }

    /**
     * @brief auto set the vars of a node
     *
     * @param n
     * @return uint32_t
     */
    uint32_t set_sat_vars_of_node(node const &n)
    {
        assert(_sat_vars_map[n] <= _constant_max_sat_var);
        _sat_vars_map[n] = _current_sat_var++;
        return _sat_vars_map[n];
    }

    /**
     * @brief prepare the node-variables mapping
     *
     * @param n
     * @return uint32_t
     */
    uint32_t prepare_node_for_sat(node const& n)
    {
        if (get_sat_vars_of_node(_ntk.node_to_index(n)) <= _constant_max_sat_var)
        {
            set_sat_vars_of_node(_ntk.node_to_index(n));
        }
        return get_sat_vars_of_node(_ntk.node_to_index(n));
    }

    /**
     * @brief add clauses generated by the cell's function
     * 
     * @param cell_func 
     * @param root 
     * @param leaves 
     * @return uint32_t 
     */
    uint32_t add_cell_func_to_solver(kitty::dynamic_truth_table const &cell_func, node const &root, std::vector<node> const &leaves)
    {
        uint32_t num_vars = cell_func.num_vars();
        auto clauses = kitty::cnf_characteristic(cell_func);

        auto vars_root = _current_sat_var++; // make different with _sat_vars_map[root], then do xor checker
        for (auto leaf : leaves)
        {
            prepare_node_for_sat(leaf);
        }

        // add the current clauses into solver
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
                        lits.push_back(make_lit(get_sat_vars_of_node(leaves[i]), false));
                    }
                    else
                    {
                        lits.push_back(make_lit(get_sat_vars_of_node(leaves[i]), true));
                    }
                }
            }
            if ((mask >> num_vars) & 1) // output bit
            {
                if ((bits >> num_vars) & 1)
                {
                    lits.push_back(make_lit(vars_root, false));
                }
                else
                {
                    lits.push_back(make_lit(vars_root, true));
                }
            }
            _solver.add_clause(lits);
        }
        return vars_root;
    }

    /**
     * @brief add the cone of a cell into the sat solver
     *
     * @param root
     * @param leaves
     * @return uint32_t
     */
    uint32_t add_cell_cone_to_solver(node const &root, std::vector<node> const &leaves)
    {
        prepare_node_for_sat(root);
        for (auto leaf : leaves)
        {
            prepare_node_for_sat(leaf);
        }

        auto candicates = collect_limited_fanin_cone_nodes(root, *min_element(leaves.begin(), leaves.end()));

        for(auto n : candicates)
        {
            if(_ntk.is_pi(n) || _ntk.is_constant(n))
            {
                continue;
            }

            std::vector<signal> children(2);
            _ntk.foreach_fanin(n, [&](auto const &c, int index)
                               { children[index] = c; });

            add_and2_to_solver(n, children[0], children[1]);

            if (is_choice(n))
            {
                // add constraint
                auto phase = _ntk.phase(n) ^ _ntk.phase(_choice_repr_map[n]);

                if (phase)
                {
                    add_compl_equal_to_solver(prepare_node_for_sat(n), prepare_node_for_sat(_choice_repr_map[n]));
                }
                else
                {
                    add_equal_to_solver(prepare_node_for_sat(n), prepare_node_for_sat(_choice_repr_map[n]));
                }

                n = _choice_repr_map[n]; // propagate through repr node to the fanout cone
            }
        }

        assert(_sat_vars_map[root] != 0u);

        return _sat_vars_map[root];
    }

    /**
     * @brief add the and2 gate to the solver using tseytin transformation
     *
     * @param n
     * @param child0
     * @param child1
     * @return uint32_t
     */
    uint32_t add_and2_to_solver(node const &n, signal child0, signal child1)
    {
        uint32_t var_node = prepare_node_for_sat(n);
        uint32_t var_child0 = prepare_node_for_sat(_ntk.get_node(child0));
        uint32_t var_child1 = prepare_node_for_sat(_ntk.get_node(child1));

        // add clause on and node
        std::vector<uint32_t> lits1(2, 0);
        std::vector<uint32_t> lits2(2, 0);
        std::vector<uint32_t> lits3(3, 0);

        /**
         * c = a && b
         * (a + !c)(b + !c)(!a + !b + c)
         */
        lits1[0] = make_lit(var_child0, false ^ child0.complement);
        lits1[1] = make_lit(var_node, true);

        lits2[0] = make_lit(var_child1, false ^ child1.complement);
        lits2[1] = make_lit(var_node, true);

        lits3[0] = make_lit(var_child0, true ^ child0.complement);
        lits3[1] = make_lit(var_child1, true ^ child1.complement);
        lits3[2] = make_lit(var_node, false);

        _solver.add_clause(lits1);
        _solver.add_clause(lits2);
        _solver.add_clause(lits3);

        return var_node;
    }

    /**
     * @brief add complementary equal constraint to child0 and child1
     * 
     * @param child0 
     * @param child1 
     */
    void add_compl_equal_to_solver(uint32_t const &var1, uint32_t const &var2)
    {
        [[maybe_unused]] uint32_t var_node = _current_sat_var++; // the dummy node, because of the 2 choice nodes do not have an ancestor

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

    /**
     * @brief add equal constraint to child0 and child1
     *
     * @param child0
     * @param child1
     */
    void add_equal_to_solver(uint32_t const &var1, uint32_t const &var2)
    {
        [[maybe_unused]] uint32_t var_node = _current_sat_var++; // the dummy node, because of the 2 choice nodes do not have an ancestor
        std::vector<uint32_t> lits(2, 0);
        lits[0] = make_lit(var1, true );
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


    /**
     * @brief check whether the mapped cone is equal to its truth table 
     * 
     * @param root      the root fo the cone
     * @return true     equal
     * @return false    not equal
     */
    int is_equal_cell(node root)
    {
        recycle_sat();
        kitty::dynamic_truth_table cell_func = _ntk.cell_function(root);
        std::vector<node> leaves;
        _ntk.foreach_cell_fanin(root, [&leaves](auto const &l)
                                { leaves.emplace_back(l); });

        for(auto leaf : leaves)
        {
            prepare_node_for_sat(leaf);
        }

        uint32_t gold_var = add_cell_func_to_solver(cell_func, root, leaves);
        uint32_t gate_var = add_cell_cone_to_solver(root, leaves);

        assert(get_sat_vars_of_node(root) != 0u);

        // add xor to check
        add_compl_equal_to_solver(gold_var, gate_var);
        auto ret = _solver.solve(_ps_sat.max_conflit_limit);

        if(ret == percy::failure)
        {
            return 1;
        }
        else if(ret == percy::success)
        {
            std::string path = "debug_cone_" + std::to_string(_ntk.node_to_index(root)) + "_";
            for(auto leaf : leaves)
            {
                path += "_" + std::to_string(_ntk.node_to_index(leaf));
            }
            generate_subgraph_dot(collect_limited_fanin_cone_nodes(root, *min_element(leaves.begin(), leaves.end())), path + ".dot");
            return -1;
        }
        else    // timeout
        {
            return 0;
        }
    }

    /**
     * @brief check the equivelence of the cone of s
     *
     * @param s the root of a signal
     * @return int
     */
    int is_equal_root(node root)
    {
        assert(_ntk.is_cell_root(root));
        recycle_sat();
        _ntk.foreach_pi([&](auto const n)
                        { 
                            prepare_node_for_sat(n);
                        });

        std::queue<node> que;
        que.push(root);
        while (!que.empty())
        {
            auto tn = que.front();
            que.pop();
            if (_ntk.is_pi(tn) || _ntk.is_constant(tn))
            {
                continue;
            }

            assert(_ntk.is_cell_root(tn));
            kitty::dynamic_truth_table cell_func = _ntk.cell_function(tn);
            std::vector<node> leaves;
            _ntk.foreach_cell_fanin(tn, [&leaves](auto const &leaf)
                                    { leaves.emplace_back(leaf); });
            for (auto leaf : leaves)
            {
                prepare_node_for_sat(leaf);
            }

            uint32_t gate_var = add_cell_cone_to_solver(tn, leaves);
            uint32_t gold_var = add_cell_func_to_solver(cell_func, tn, leaves);

            // add to check
            add_compl_equal_to_solver(gate_var, gold_var);

            for (auto leaf : leaves)
            {
                que.push(leaf);
            }
        }

        auto ret = _solver.solve(_ps_sat.max_conflit_limit);
        if (ret == percy::failure)
        {
            return 1;
        }
        else if (ret == percy::success)
        {
            return -1;
        }
        else // timeout
        {
            return 0;
        }
    }

    /**
     * @brief check the cone of a signal incrementally
     *
     * @param s
     */
    bool check_signal_incrementally(signal const &s)
    {
        assert(_ntk.is_cell_root(_ntk.get_node(s)));
        _ntk.foreach_pi([&](auto const n)
                        { prepare_node_for_sat(n); });
        auto max_node = _ntk.get_node(s);
        bool error = false;

        _ntk.foreach_gate([&](auto const &n)
                          {
            if(error || (n > max_node))
            {
                return;
            }   
            if( _ntk.is_cell_root(n) )
            {
                auto equal = is_equal_root(n);
                if (equal == -1)
                {
                    error = true;
                    _ue_node = n;
                }
                else if (equal == 0)
                {
                    std::cerr << " timeout... " << std::endl;
                }
            }   
            });

        return !error;
    }

    /**
     * @brief generate the subgraph's dot file for visualization
     * 
     * @param cone 
     * @param path 
     */
    void generate_subgraph_dot(std::set<node> cone, std::string path)
    {
        std::ofstream os(path.c_str(), std::ofstream::out);
        std::stringstream nodes, edges, levels;
        std::unordered_map<node, int> node_level_map;
        int level_max = -1;
        for(auto n : cone)
        {  
            node_level_map[n] = 0;
        }
        
        auto node_label_func = [&](node const &n) -> std::string
        {
            if (is_choice(n))
            {
                return std::to_string(_ntk.node_to_index(n)) + "->" + std::to_string(_ntk.node_to_index(_choice_repr_map[n]));
            }
            else
            {
                return std::to_string(_ntk.node_to_index(n));
            }
        };

        auto node_shape_func = [&](node const &n) -> std::string
        {
            if (_ntk.is_constant(n))
            {
                return "box";
            }
            else if (_ntk.is_pi(n))
            {
                return "triangle";
            }
            else
            {
                return "ellipse";
            }
        };

        auto node_color_func = [&](node const &n) -> std::string
        {
            if (_ntk.is_constant(n) || _ntk.is_pi(n))
            {
                return "snow2";
            }
            else if (_ntk.is_repr(n) || is_choice(n))
            {
                return "lightskyblue";
            }
            else
            {
                return "white";
            }
        };

        auto edge_style_func = [&](node const &n, signal const &c) -> std::string
        {
            if (_ntk.is_complemented(c))
            {
                return "dashed";
            }
            else
            {
                return "solid";
            }
        };

        for(auto n : cone)
        {
            // compute level
            _ntk.foreach_fanin(n, [&](auto const& c){
                auto cn = _ntk.get_node(c);
                if( node_level_map.find(cn) != node_level_map.end() )
                {
                    node_level_map[n] = std::max(node_level_map[n], node_level_map[cn] + 1);
                    level_max = std::max(level_max, node_level_map[n]);
                    // add edges
                    edges << fmt::format("{} -> {} [style={}]\n",
                                         _ntk.node_to_index(_ntk.get_node(c)), _ntk.node_to_index(n), edge_style_func(n, c)
                                         );
                }
            });
            // add nodes
            auto index = _ntk.node_to_index(n);
            nodes << fmt::format("{} [label=\"{}\",shape={},style=filled,fillcolor={}]\n",
                                index, node_label_func(n), node_shape_func(n), node_color_func(n)
                                );
        }   

        // add ranks
        std::map<int, std::set<node>> level_nodes_map;

        for(auto nl : node_level_map)
        {
            level_nodes_map[nl.second].insert(nl.first);
        }

        for(auto ln : level_nodes_map)
        {
            levels << "{rank = same; ";
            for(auto n : ln.second)
            {
                levels << std::to_string(_ntk.node_to_index(n)) + "; ";
            }
            levels << "}\n";
        }

        os << "digraph {\n"
           << "rankdir=BT;\n"
           << nodes.str() << edges.str() << levels.str() << "}\n";
    }

private: 
    Ntk                              _ntk;
    std::map< node, std::set<node> > _fanouts;
    std::map< node, node >           _choice_repr_map;
    percy::bsat_wrapper     _solver;

    std::vector<uint32_t>   _sat_vars_map;
    uint32_t                _constant_max_sat_var;
    uint32_t                _current_sat_var;
    params_sat              _ps_sat;

    // record the first unequivalent node and signal
    node    _ue_node;
    signal  _ue_signal;
}; // end class checker_4_mapped_graph

iFPGA_NAMESPACE_HEADER_END