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

#include <random>
#include <vector>
#include <map>

#include "algorithms/aig_with_choice.hpp"
#include "algorithms/circuit_validator.hpp"
#include "percy/solvers.hpp"

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the parameters of choice computation
 * 
 */
struct choice_params
{
    unsigned nwords{8};           ///< the number of simulation words
    unsigned BT_limit{1000};      ///< confilict limit at a node
    unsigned max_sat_var{5000};   ///< the max number of SAT variables
    unsigned calls_recycle{100};  ///< calls to perform before recycling SAT solver
    bool polar_flip{true};        ///< uses polarity adjustment
};

/**
 * @brief do simulation to create candidate equivalence classes for choice computation
 * ref: abc/src/proof/dch/dch/dchClass.c: struct Dch_Cla_t_ 
 */
class cand_equiv_classes
{
public:
    using node      = aig_network::node;
    using signal    = aig_network::signal;
    static constexpr node AIG_NULL = aig_network::AIG_NULL;

public:
    /**
     * @brief Construct a new Cand Equiv Classes object
     * 
     * @param aig       the orignal AIG network to be computed 
     * @param nwords    the simulation word size 
     */
    cand_equiv_classes(aig_network const& aig, unsigned nwords) : _nwords(nwords), _aig(aig)
    {
        _id2class.assign(aig.size(), std::vector<node>());
        
        _reprs.assign(_aig.size(), 0);
        for(size_t i = 0; i < _aig.size(); ++i)
        {
            _reprs[i] = i;
        }
    }
    ~cand_equiv_classes() {}

    /// generate an random value
    unsigned get_random_value() { return _rng(); }

    /**
     * @brief compute candidat equiv classes by simulation
     * @return the representative array of this network
     */
    void compute_equiv_classes()
    {
       perform_random_simulation();
       prepare();
       for(auto i = 0; i < 7; ++i)
       {
           perform_random_simulation();
           refine_classes(true /* recursively */);
       }
    }

    /**
     * @brief get the simulation representative of one node
     * @param n the choice node 
     * @return node the representative of node n 
     */
    node get_sim_repr(node const& n) const
    {
        assert(n < _reprs.size());
        return _reprs[n];
    }

    /**
     * @brief Get the equiv classes of one node
     * 
     * @param n the repr node
     * @return all nodes in the equiv classes of node n if n is a representative, or empty vector 
     */
    const std::vector<node>& get_equiv_classes(node n) const
    {
        assert(n < _id2class.size());
        return _id2class[n];
    }

    /**
     * @brief refine one equiv classes after simulation/resimulation
     * 
     * @param repr the repr of this classes
     * @return int 1 if the splitting happened, or 0 
     */
    int refine_one_class(node const& repr, bool resimulation = false, bool recursively = false)
    {
        auto& equiv_class = _id2class[repr];
        std::vector<node> class_old;
        std::vector<node> class_new;
        for(auto& n : equiv_class)
        {
            bool verify = resimulation ? equal_cex(repr, n) : equal(repr, n);
            if(verify) 
                class_old.push_back(n);
            else
                class_new.push_back(n);
        }
        if(class_new.empty())
            return 0; //< no splitting happened
        
        _id2class[repr].swap(class_old);
        auto repr_new = class_new.front();
        for(auto& c : class_new)
        {
            _reprs[c] = repr_new;   //< FIXME, here ABC set _reprs[repr_new] = NULL
        }
        _id2class[repr_new].swap(class_new);

        if(recursively && _id2class[repr_new].size() > 1)
        {
            return 1 + refine_one_class(repr_new, resimulation, true);
        }
        return 1;
    }    

    /**
     * @brief print all equiv classes, for debug
     * 
     */
    void print_classes() const
    {
        for(auto& cls : _id2class)
        {
            if(cls.empty()) continue;
            std::cout << "class " << cls[0] << ":" << std::endl;
            for(auto c : cls) 
            {
                std::cout << c << " ";
            }
            std::cout << std::endl;
        }
    }
private:
    void perform_random_simulation()
    {
        // init simulation info
        _sim.assign(_aig.size(), std::vector<uint64_t>(_nwords, 0));
        //_sim[0].assign(_nwords, 0x0);

        // random PI sim info
        _aig.foreach_ci([&](node const& n){
            for(auto i = 0u; i < _nwords; ++i)
            {
                _sim[n][i] = get_random_value();
            }
            _sim[n][0] <<= 1;
        });
        // simulate AIG in topo order
        _aig.foreach_gate([&](node const& n){
            auto child0 = _aig.get_child0(n);
            auto child1 = _aig.get_child1(n);
            auto& sim0 = _sim[child0.index];
            auto& sim1 = _sim[child1.index];
            auto& sim = _sim[n];

            for(auto i = 0u; i < _nwords; ++i)
            {
                sim[i] = (child0.complement ? ~sim0[i] : sim0[i]) & (child1.complement ? ~sim1[i] : sim1[i]);
            }
        });

    }

    void prepare()
    {
        // init hash table
        int hash_size = Abc_PrimeCudd(_aig.size() / 4);
        std::vector<uint64_t> table(hash_size, AIG_NULL);

        // add all nodes to hash table
        _aig.foreach_node([&](node const& n){
            // check if the node belongs to the constant class
            //if(is_const(n))
            //{
            //    _reprs[n] = 0;
            //    return;
            //}
            // hash 
            unsigned hash_code = node_hash(n) % hash_size;
            if(table[hash_code] == AIG_NULL)
            {
                table[hash_code] = n;
                _id2class[n].push_back(n);
            }
            else
            {
                // here not check if nexts are real equal(same sim info), refine_class() will do it.
                auto repr = table[hash_code];
                _reprs[n] = repr;
                _id2class[repr].push_back(n);
            }
        });

        // refine the classes
        refine_classes();
    }

    /**
     * @brief refine all equiv classes
     * @return int the count of splitting happend
     */
    int refine_classes(bool recursively = false)
    {
        int count = 0;
        for(size_t i = 0; i < _id2class.size(); ++i)
        {
            auto& equiv_class = _id2class[i];
            if(!equiv_class.empty())
            {
                count += refine_one_class(i, false/* resimulation */, recursively);
            }
        }
        return count;
    }

    /// compute hash value of the node using its simulation info
    unsigned node_hash(node const& n)
    {
        static int s_FPrimes[128] = { 
            1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
            1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
            2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
            2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
            3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
            3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
            4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
            4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
            5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
            6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
            6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
            7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
            8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
        };
        unsigned uHash = 0;
        auto& sim = _sim[n];
        if(_aig.phase(n))
        {
            for (auto k = 0u; k < _nwords; k++)
                uHash ^= ~sim[k] * s_FPrimes[k & 0x7F];
        }
        else
        {
            for (auto k = 0u; k < _nwords; k++)
                uHash ^= sim[k] * s_FPrimes[k & 0x7F];
        }
        return uHash;
    }

    /// check if simulation info is composed of all zeros
    bool is_const(node const& n)
    {
        auto& sim = _sim[n];
        if(_aig.phase(n))
        {
            for(auto k = 0u; k < _nwords; k++)
                if(~sim[k])
                    return false;
        }
        else
        {
            for(auto k = 0u; k < _nwords; k++)
                if(sim[k])
                    return false;
        }
        return true;
    }

    /// check if the 2 nodes are equal(simulation infos are same)
    bool equal(node const& n1, node const& n2)
    {
        auto& sim0 = _sim[n1];
        auto& sim1 = _sim[n2];
        if(_aig.phase(n1) != _aig.phase(n2))
        {
            for(auto k = 0u; k < _nwords; k++)
                if(sim0[k] != ~sim1[k])
                    return false;
        }
        else
        {
            for(auto k = 0u; k < _nwords; k++)
                if(sim0[k] != sim1[k])
                    return false;
        }
        return true;
    }

    /// check if the 2 nodes are equal(for resimulation)
    bool equal_cex(node const& n1, node const& n2)
    {
        return (_aig.phase(n1) == _aig.phase(n2)) == 
                (_aig.mark_b(n1) == _aig.mark_b(n2)); // MarkB
    }

    /**
     * @brief get the next prime >= p
     * ref:   abc/misc/util/abc_global.h
     */
    int Abc_PrimeCudd( unsigned p)
    {
        int i, pn;
        p--;
        do
        {
            p++;
            if (p & 1)
            {
                pn = 1;
                i = 3;
                while ((unsigned)(i * i) <= p)
                {
                    if (p % (unsigned)i == 0)
                    {
                        pn = 0;
                        break;
                    }
                    i += 2;
                }
            }
            else
                pn = 0;
        } while (!pn);
        return (int)(p);
    }

private:
    unsigned _nwords;                            ///< the simulation word size
    aig_network _aig;                            ///< orignal AIG network
    std::vector<std::vector<node>> _id2class;    ///< equiv classes by ID of repr node 
    std::vector<node> _reprs;                    ///< representatives of each node, the array 'simrep' in the paper

    std::vector<std::vector<uint64_t>> _sim;     ///< simulation info
    std::minstd_rand0 _rng;                      ///< random number generator from C++ std

};

/**
 * @brief the wrapper of SAT-solver to do sat-prove() in choice compuation
 * 
 */
class sat_prover
{
public:
    using node      = aig_network::node;
    using signal    = aig_network::signal;
    static constexpr node AIG_NULL = aig_network::AIG_NULL;

public:
    sat_prover(choice_params params, aig_with_choice& fraig, uint32_t size) : 
        _params(params), _fraig(fraig), _sat_vars(1), _recycle_num(0), _calls_since(0)
    {
        _sat_vars_map.assign(size, 0);
        // for const0
        _solver.set_nr_vars(1000);
        _sat_vars = 1; // for const node

        uint32_t lit = make_lit(_sat_vars, true);
        //if(_params.polar_flip)
        //{
        //    lit = lit_not(lit);
        //}
        _solver.add_clause(std::vector<uint32_t>(1, lit));
        _sat_vars_map[0] = _sat_vars++;
    }

    /// get counter example of this node from sat solver 
    int get_var_value(node const& n)
    {
        assert(n < _sat_vars_map.size());
        uint32_t var = _sat_vars_map[n];
        // get the value from the SAT solver
        // (account for the fact that some vars may be minimized away)
        return var ? _solver.var_value(var) : 0;
    }

    /**
     * @brief use sat solver to verify 2 nodes are real equivalent or not
     * @param repr the candidate reprentative node 
     * @param n the candidate choice node 
     * @return percy::success if solved(which means not equivalent) 
     *         or percy::failure if unsolved(which means equivalent)
     *         or percy::timeout if meet time limitation
     */
    percy::synth_result sat_prove(node const& repr, node const& n)
    {
        assert( repr != n);

        ++_calls_since;
        //check if SAT solver needs recycling
        if(_params.max_sat_var && _sat_vars > _params.max_sat_var &&
            _calls_since > _params.calls_recycle) 
            {
                recycle_sat_solver();
            }
        
        // if the nodes do not have SAT variables, allocate them
        cnf_node_add_to_solver(repr);
        cnf_node_add_to_solver(n);

        // propagate unit clauses
        _solver.propaget_unit_clauses();

        // solve under assumptions
        // A = 0; B = 1        OR  A = 0; B = 0
        std::vector<uint32_t> lits;
        lits.push_back(make_lit(_sat_vars_map[repr], true));
        lits.push_back(make_lit(_sat_vars_map[n], _fraig.phase(repr) ^ _fraig.phase(n)));
        if(_params.polar_flip)
        {
            if(_fraig.phase(repr)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(n)) lits[1] = lit_not(lits[1]);
        }

        percy::synth_result ret = _solver.solve(lits, static_cast<int>(_params.BT_limit));
        if(ret == percy::failure)
        {
            lits[0] = lit_not(lits[0]);
            lits[1] = lit_not(lits[1]);
            [[maybe_unused]] int ret1 = _solver.add_clause(lits);
            assert(ret1);
        }
        // TODO, stats
        else if(ret == percy::success)
        {
            return ret;
        }
        else
        {
            return ret;
        }

        // if the old node was constant 0, we already know the answer
        if(repr == _fraig.get_constant(false).index)
        {
            return percy::failure;
        }

        // solve under assumptions
        // A = 1; B = 0    OR      A = 1; B = 1
        lits[0] = make_lit(_sat_vars_map[repr], false);
        lits[1] = make_lit(_sat_vars_map[n], _fraig.phase(repr) == _fraig.phase(n));
        if(_params.polar_flip)
        {
            if(_fraig.phase(repr)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(n)) lits[1] = lit_not(lits[1]);
        }
        ret = _solver.solve(lits, static_cast<int>(_params.BT_limit)); 
        if(ret == percy::failure)
        {
            lits[0] = lit_not(lits[0]);
            lits[1] = lit_not(lits[1]);
            [[maybe_unused]] int ret1 = _solver.add_clause(lits);
            assert(ret1);
        }
        // TODO, stats
        return ret;
    }

    /**
     * @brief recycle the sat solver
     */
    void recycle_sat_solver()
    {
        for(auto n : _used_nodes)
        {
            _sat_vars_map[n] = 0;
        }
        _used_nodes.clear();

        if(_recycle_num)
        {
            //std::cout << "delete last solver" << _recycle_num << std::endl;
            //delete &_solver;
            // FIXME, ABC has specail memory management
        }
        //std::cout << "create new solver" << (_recycle_num + 1) << std::endl;

        // auto new_solver = new percy::bsat_wrapper();
        // _solver = *new_solver;

        _solver.restart();

        _solver.set_nr_vars(1000);
        _sat_vars = 1; // for const node

        uint32_t lit = make_lit(_sat_vars, true);
        //if(_params.polar_flip)
        //{
        //    lit = lit_not(lit);
        //}
        _solver.add_clause(std::vector<uint32_t>(1, lit));
        _sat_vars_map[0] = _sat_vars++;

        ++_recycle_num;
        _calls_since = 0;
    }

    /**
     * @brief add the node into sat solver
     * @param n the node to be verified
     */
    void cnf_node_add_to_solver(node const& n)
    {
        // quit if CNF is ready
        if(_sat_vars_map[n]) return;

        std::vector<node> frontier;
        // start the frontier
        add_to_frontier(n, frontier);
        // explore nodes in frontier
        for(size_t i = 0; i < frontier.size(); ++i)
        {
            // create the supergate
            node cur_node = frontier[i];
            assert(_sat_vars_map[cur_node] != 0);
            if(_fraig.is_mux(cur_node))
            {
                std::vector<signal> fanins;
                auto child0 = _fraig.get_child0(cur_node).index;
                auto child1 = _fraig.get_child1(cur_node).index;
                vector_push_unique(fanins, +_fraig.get_child0(child0));
                vector_push_unique(fanins, +_fraig.get_child0(child1));
                vector_push_unique(fanins, +_fraig.get_child1(child0));
                vector_push_unique(fanins, +_fraig.get_child1(child1));

                for(auto child : fanins)
                {
                    add_to_frontier(child.index, frontier);
                }
                add_clauses_mux(cur_node);
            }
            else
            {
                std::vector<signal> fanins;
                assert(!_fraig.is_ci(cur_node));
                collect_super(signal(cur_node, 0), fanins, true);
                for(auto child : fanins)
                {
                    add_to_frontier(child.index, frontier);
                }
                add_clauses_super(cur_node, fanins);
            }
        }
    }

    /// add the node to the sat solver
    void add_to_frontier(node const& n, std::vector<node>& frontier)
    {
        if( _sat_vars_map[n]) return;
        if(n == 0) return; 
        _used_nodes.push_back(n);
        _sat_vars_map[n] = _sat_vars++;
        if(_fraig.is_and(n))
        {
            frontier.push_back(n);
        }
    }

    /**
     * @brief add mux clauses
     * @param n the root node of the mux 
     */
    void add_clauses_mux(node const& n)
    {
        assert(_fraig.is_mux(n));
        // get nodes (I = if, T = then, E = else)
        signal i, t, e; 
        _fraig.recognize_mux(n, i, t, e);

        // get the variable numbers
        auto var_f = _sat_vars_map[n];
        auto var_i = _sat_vars_map[i.index];
        auto var_t = _sat_vars_map[t.index];
        auto var_e = _sat_vars_map[e.index];

        // f = ITE(i, t, e);
        // i' + t' + f
        // i' + t + f'
        // i + e' + f
        // i + e + f'

        // create four clauses
        std::vector<uint32_t> lits(3, 0);
        lits[0] = make_lit(var_i, true);
        lits[1] = make_lit(var_t, true ^ t.complement);
        lits[2] = make_lit(var_f, false);
        if(_params.polar_flip)
        {
            if(_fraig.phase(i.index)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(t.index)) lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n)) lits[2] = lit_not(lits[2]);
        }
        [[maybe_unused]] int ret = _solver.add_clause(lits);
        assert(ret);

        lits[0] = make_lit(var_i, true);
        lits[1] = make_lit(var_t, false ^ t.complement);
        lits[2] = make_lit(var_f, true);
        if(_params.polar_flip)
        {
            if(_fraig.phase(i.index)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(t.index)) lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n)) lits[2] = lit_not(lits[2]);
        }
        ret = _solver.add_clause(lits);
        assert(ret);

        lits[0] = make_lit(var_i, false);
        lits[1] = make_lit(var_e, true ^ e.complement);
        lits[2] = make_lit(var_f, false);
        if(_params.polar_flip)
        {
            if(_fraig.phase(i.index)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(e.index)) lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n)) lits[2] = lit_not(lits[2]);
        }
        ret = _solver.add_clause(lits);
        assert(ret);

        lits[0] = make_lit(var_i, false);
        lits[1] = make_lit(var_e, false ^ e.complement);
        lits[2] = make_lit(var_f, true);
        if(_params.polar_flip)
        {
            if(_fraig.phase(i.index)) lits[0] = lit_not(lits[0]);
            if(_fraig.phase(e.index)) lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n)) lits[2] = lit_not(lits[2]);
        }
        ret = _solver.add_clause(lits);
        assert(ret);

        // two additional clauses
        // t' & e' -> f'
        // t & e -> f
        // t + e + f'
        // t' + e' + f
        if( var_t == var_e) return;

        lits[0] = make_lit(var_t, false ^ t.complement);
        lits[1] = make_lit(var_e, false ^ e.complement);
        lits[2] = make_lit(var_f, true);
        if(_params.polar_flip)
        {
            if(_fraig.phase(t.index))   lits[0] = lit_not(lits[0]);
            if(_fraig.phase(e.index))   lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n))         lits[2] = lit_not(lits[2]);
        }
        ret = _solver.add_clause(lits);
        assert(ret);

        lits[0] = make_lit(var_t, true ^ t.complement);
        lits[1] = make_lit(var_e, true ^ e.complement);
        lits[2] = make_lit(var_f, false);
        if(_params.polar_flip)
        {
            if(_fraig.phase(t.index))   lits[0] = lit_not(lits[0]);
            if(_fraig.phase(e.index))   lits[1] = lit_not(lits[1]);
            if(_fraig.phase(n))         lits[2] = lit_not(lits[2]);
        }
        ret = _solver.add_clause(lits);
        assert(ret);
    }

    /**
     * @brief collect fanins recursively
     * @param s the root signal 
     * @param fanins fanins vector 
     * @param first 
     */
    void collect_super(signal const& s, std::vector<signal>& fanins, bool first)
    {
        if(s.complement || _fraig.is_ci(s.index) ||
            (!first && _fraig.fanout_size(s.index) > 1) ||
            _fraig.is_mux(s.index) )
            //_fraig.is_mux(s.index) && false ) // FIXME, debuging
            {
                vector_push_unique(fanins, s);
                return;
            }
        
        collect_super(_fraig.get_child0(s.index), fanins, false);
        collect_super(_fraig.get_child1(s.index), fanins, false);
    }

    /**
     * @brief push if the element not in vector already
     * @param vec 
     * @param n 
     */
    void vector_push_unique(std::vector<signal>& vec, signal const& n)
    {
        for(auto i : vec)
        {
            if(i == n) return;
        }
        vec.push_back(n);
    }

    /**
     * @brief add super gate clauses
     * @param n the root node 
     * @param fanins the literal fanins 
     */
    void add_clauses_super(node const& n, std::vector<signal> const& fanins)
    {
        assert(_fraig.is_and(n));
        // suppose AND-gate is A & B = C
        // add !A => !C or A + !C
        for(auto fanin : fanins)
        {
            std::vector<uint32_t> lits;
            lits.push_back(make_lit(_sat_vars_map[fanin.index], fanin.complement));
            lits.push_back(make_lit(_sat_vars_map[n], true));
            if(_params.polar_flip)
            {
                if(_fraig.phase(fanin.index)) lits[0] = lit_not(lits[0]);
                if(_fraig.phase(n))     lits[1] = lit_not(lits[1]);
            }
            [[maybe_unused]] int ret = _solver.add_clause(lits);
            assert(ret);
        }
        // add A & B => C  or !A +!B + C
        std::vector<uint32_t> lits;
        for(size_t i = 0; i < fanins.size(); ++i)
        {
            auto fanin = fanins[i];
            lits.push_back(make_lit(_sat_vars_map[fanin.index], !fanin.complement));
            if(_params.polar_flip)
            {
                if(_fraig.phase(fanin.index)) lits[i] = lit_not(lits[i]);
            }
        }
        lits.push_back(make_lit(_sat_vars_map[n], false));
        if (_params.polar_flip)
        {
            if (_fraig.phase(n)) lits.back() = lit_not(lits.back());
        }
        [[maybe_unused]] int ret = _solver.add_clause(lits);
        assert(ret);
    }

private:
    choice_params _params;                       ///< parameters of choice computation
    aig_with_choice& _fraig;                     ///< final AIG network

    // sat solving
    percy::bsat_wrapper _solver;                 ///< recyclable SAT solver
    uint32_t _sat_vars;                          ///< the counter of SAT variables
    std::vector<uint32_t> _sat_vars_map;         ///< mapping of each node into its SAT var
    std::vector<uint32_t> _used_nodes;           ///< nodes whose SAT vars are assigned
    unsigned _recycle_num;                       ///< the number of times SAT solvers was recycled
    unsigned _calls_since;                       ///< the number of calls since the last recycle
    // solver cone size
    //unsigned _this_cone_size;
    //unsigned _max_cone_size;
};

/**
 * @brief the manager to build an aig network with choices
 * 
 */
class choice_computation
{
public:
    using node      = aig_network::node;
    using signal    = aig_network::signal;
    static constexpr node AIG_NULL = aig_network::AIG_NULL;
    #define SIGNAL_NULL (signal(AIG_NULL, 0))

public:
    /**
     * @brief Construct a new choice computation object
     * @param params the choice parameters 
     * @param aig the original network
     */
    choice_computation(choice_params params, aig_network aig) : 
    _params(params), _aig(aig), _fraig(aig.size()), _simulator(_aig, params.nwords), _prover(params, _fraig, aig.size())
    {
        _repr_proved.assign(_aig.size(), 0);
        for(size_t i = 0; i < _aig.size(); ++i)
        {
            _repr_proved[i] = i;
        }

        _fanouts.assign(_aig.size(), std::vector<node>());
        // calculate fanout for each node once
        _aig.foreach_gate( [&]( auto const& n ){
            _aig.foreach_fanin( n, [&]( auto const& c ){
                auto& fanout = _fanouts[c.index];
                if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
                {
                    fanout.push_back( n );
                }
          });
      });
    }

    /**
     * @brief create aig with choices
     * @return aig_with_choice 
     */
    aig_with_choice compute_choice()
    {
        // compute candidate equivalence classes
        _simulator.compute_equiv_classes();
        // perform sat-prove()
        man_sweep();

        // create aig with choices
        auto awc = derive_choice_aig();
//#ifndef NDEBUG
//        circuit_validator fv(awc);
//        awc.foreach_gate([&](auto n) {
//            if(awc.is_repr(n))
//            {
//                auto equiv = awc.get_equiv_node(n);
//                while(equiv != AIG_NULL)
//                {
//                    auto result = fv.validate(signal(n, awc.phase(n)), signal(equiv, awc.phase(equiv)));
//                    if(result)
//                    {
//                        assert(*result);
//                    }
//                    equiv = awc.get_equiv_node(equiv);
//                }
//            }
//        });
//#endif
        return awc;
    }

private:
    /**
     * @brief do sat-prove() to verify equivalent nodes
     * will not sweep them exactly, but mark them as choices in array _repr_proved
     */
    void man_sweep()
    {
        _old2new.assign(_aig.size(), SIGNAL_NULL); //< the array final in the paper
        // map const and PIs
        _old2new[0] = _fraig.get_constant(false);
        _aig.foreach_ci([&](node const& n){
            _old2new[n] = _fraig.create_pi();
        });
        // sweep internal nodes
        _aig.foreach_gate([&](node const& n){
            if(_old2new[_aig.get_child0(n).index] == SIGNAL_NULL ||
               _old2new[_aig.get_child1(n).index] == SIGNAL_NULL)
               return;
            auto old_child0 = _aig.get_child0(n);
            auto old_child1 = _aig.get_child1(n);
            auto new_node = _fraig.create_and(_old2new[old_child0.index] ^ old_child0.complement,
                                              _old2new[old_child1.index] ^ old_child1.complement);
            _old2new[n] = new_node;
            sweep_node(n, new_node.index);
        });

        // clean MarkB
        _aig.foreach_node([&](node const& n)
        {
            _aig.mark_b(n, false);
        });
    }

    /**
     * @brief to verify a node is choice or not
     * @param n the node in original network 
     * @param new_node the node in new network(with choices)
     */
    void sweep_node(node const& n, node const& new_node)
    {
        auto old_repr = _simulator.get_sim_repr(n);
        if(old_repr == n) return;
        node new_repr = _old2new[old_repr].index;
        if(new_repr == AIG_NULL) return;

        if(new_repr == new_node)
        {
            _repr_proved[n] = old_repr;
            return;
        }

        percy::synth_result sat_result = _prover.sat_prove(new_repr, new_node);
        if(sat_result == percy::timeout) // timed out
        {
            _old2new[n] = SIGNAL_NULL;  // dropped? 
            return;
        }
        if(sat_result == percy::failure) // proved equivalent
        {
            _old2new[n] = _old2new[old_repr] ^ (_aig.phase(n) ^ _aig.phase(old_repr));
            _repr_proved[n] = old_repr;
            return;
        }

        // disproved equivalent
        //_simulator.print_classes();
        resimulate(old_repr, n);
        //_simulator.print_classes();
    }

    /**
     * @brief resimulate 2 non-equivalent nodes and refine the equiv classes
     * @param repr the disproved representative node 
     * @param n the disproved choice node 
     */
    void resimulate(node const& repr, node const& n)
    {
        // get the equivalence classes
        collect_tfo_cands(repr, n);
        // resimulate the cone of influence of the solved nodes
        //_this_cone_size = 0; 
        _aig.incr_trav_id();
        _aig.set_visited(0, _aig.trav_id());
        resimulate_solved_rec(n);
        resimulate_solved_rec(repr);
        //_max_cone_size = std::max(_max_cone_size, _this_cone_size);

        // resimulate the cone of influence of the cand classes
        int ret = 0;
        for(auto root : _sim_classes)
        {
            auto& classes = _simulator.get_equiv_classes(root);
            for(auto c : classes)
            {
                resimulate_other_rec(c);
            }
            ret += _simulator.refine_one_class(root, true/* resimulation */);
        }
        assert(ret);
    }

    /**
     * @brief collect TFO cone to do resimulate
     * @param repr the disproved representative node 
     * @param n the disproved choice node 
     */
    void collect_tfo_cands(node const& repr, node const& n)
    {
        _aig.incr_trav_id();
        _aig.set_visited(0, _aig.trav_id());
        _sim_classes.clear();
        collect_tfo_cands_rec(n);
        collect_tfo_cands_rec(repr);
        std::sort(_sim_classes.begin(), _sim_classes.end());
        // clear MarkA
        for(auto equiv : _sim_classes)
        {
            _aig.mark_a(equiv, false);
        }
    }

    /**
     * @brief collect TFO cone recursively 
     * @param n the root node 
     */
    void collect_tfo_cands_rec(node const& n)
    {
        if(_aig.visited(n) == _aig.trav_id()) return;
        _aig.set_visited(n, _aig.trav_id());

        // traverse the fanouts
        auto& fanouts = _fanouts[n];
        for(auto fo : fanouts)
        {
            collect_tfo_cands_rec(fo);
        }

        // check if the given node has a representive
        auto repr = _simulator.get_sim_repr(n);
        if(repr == n) return;

        // repr is the representive of an equivalence class
        if(_aig.mark_a(repr)) return;
        _aig.mark_a(repr, true); // MarkA
        _sim_classes.push_back(repr);
    }

    /**
     * @brief resimulate disproved node 
     * @param n the disproved node 
     */
    void resimulate_solved_rec(node const& n)
    {
        if(_aig.visited(n) == _aig.trav_id()) return;
        _aig.set_visited(n, _aig.trav_id());
        
        if(_aig.is_ci(n))
        {
            auto new_node = _old2new[n];
            int value = _prover.get_var_value(new_node.index);
            //int value = _cex[n - 1];
            _aig.mark_b(n, value); //MarkB
            return;
        }
        auto child0 = _aig.get_child0(n);
        auto child1 = _aig.get_child1(n);
        resimulate_solved_rec(child0.index);
        resimulate_solved_rec(child1.index);
        int value = (_aig.mark_b(child0.index) ^ child0.complement) & (_aig.mark_b(child1.index) ^ child1.complement);
        _aig.mark_b(n, value); // MarkB 
        // count the cone size
        //if(_sat_vars_map[_old2new[n]] > 0)
        //{
        //    ++_this_cone_size;
        //}
    }

    /**
     * @brief resimulate disproved node 
     * @param n the disproved node 
     */
    void resimulate_other_rec(node const& n)
    {
        if(_aig.visited(n) == _aig.trav_id()) return;
        _aig.set_visited(n, _aig.trav_id());

        if(_aig.is_ci(n))
        {
            //set random value
            _aig.mark_b(n, _simulator.get_random_value() & 1); // MarkB
            return;
        }
        auto child0 = _aig.get_child0(n);
        auto child1 = _aig.get_child1(n);
        resimulate_other_rec(child0.index);
        resimulate_other_rec(child0.index);
        int value = (_aig.mark_b(child0.index) ^ child0.complement) & (_aig.mark_b(child1.index) ^ child1.complement);
        _aig.mark_b(n, value); // MarkB 
    }

    /**
     * @brief derive the result network with choice
     * @return aig_with_choice created from orignal network with array _repr_proved 
     */
    aig_with_choice derive_choice_aig()
    {
        aig_with_choice awc(_aig.size());
        // compute choices
        std::vector<signal> old2new(_aig.size(), SIGNAL_NULL);
        old2new[0] = _fraig.get_constant(false);
        _aig.foreach_ci([&](node const& i){
            awc.create_pi();
            old2new[i] = signal(i, 0);
        });
        _aig.foreach_gate([&](node const& n){
            auto repr = _repr_proved[n];

            // for const and PI
            if(_aig.is_constant(repr) || _aig.is_ci(repr))
            {
                assert(old2new[repr] != SIGNAL_NULL);
                old2new[n] = old2new[repr] ^ (_aig.phase(n) ^ _aig.phase(repr));
                return;
            }

            auto get_repr_signal = [&](signal const& s)
            {
                auto repr = awc.get_repr(s.index);
                return signal(repr, awc.phase(repr) ^ awc.phase(s.index) ^ s.complement);
            };
            // get the new node
            auto old_child0 = _aig.get_child0(n);
            auto old_child1 = _aig.get_child1(n);
            auto new_child0 = old2new[old_child0.index] ^ old_child0.complement;
            auto new_child1 = old2new[old_child1.index] ^ old_child1.complement;
            auto new_node = awc.create_and(get_repr_signal(new_child0), get_repr_signal(new_child1));
            while(1)
            {
                auto new2 = new_node;
                new_node = get_repr_signal(new2);
                if(new_node == new2) break;
            }
            assert(old2new[n] == SIGNAL_NULL);
            old2new[n] = new_node;
            // skip those without reprs
            if(repr == n)
            {
                return;
            }
            assert(repr < n);
            // get the corresponding new nodes
            auto new_repr = old2new[repr];
            // skip earlier nodes
            if(new_repr.index >= new_node.index) return;
            //assert(awc.fanout_size(new_node.index) == 0);
            // log2, div
            // skip used nodes, FIXME: here ABC set _reprs but not equivs_ 
            if(awc.fanout_size(new_node.index) != 0) return;
            awc.set_choice(new_node.index, new_repr.index);
        });
        _aig.foreach_co([&](signal const& o){
            auto new_signal = old2new[o.index] ^ o.complement;
            auto new_repr = awc.get_repr(new_signal.index);
            awc.create_po(signal(new_repr, awc.phase(new_signal.index) ^ awc.phase(new_repr) ^ new_signal.complement));
        });

        // check_choices()
        //{
        //circuit_validator fv(awc);
        //awc.foreach_gate([&](auto n) {
        //    if(awc.is_repr(n))
        //    {
        //        auto equiv = awc.get_equiv_node(n);
        //        while(equiv != AIG_NULL)
        //        {
        //            auto result = fv.validate(signal(n, awc.phase(n)), signal(equiv, awc.phase(equiv)));
        //            printf("check node %ld with choice %ld: ", n, equiv);
        //            if(result)
        //            {
        //                assert(*result);
        //                if(*result)
        //                    printf("success\n");
        //                else
        //                    printf("NON-EQUIVALENT!!!\n");
        //            }
        //            else
        //            {
        //                printf("timeout\n");
        //            }
        //            equiv = awc.get_equiv_node(equiv);
        //        }
        //    }
        //});
        //}
        
        // find correct topo order with choices
        return man_dup_dfs(awc);
    }

    /**
     * @brief duplicate the network in DFS topo order
     * @param tmp the unordered network 
     * @return ordered aig_with_choice 
     */
    static aig_with_choice man_dup_dfs(aig_with_choice const& tmp)
    {
        aig_with_choice  awc(tmp.size());
        std::vector<signal> old2new(tmp.size(), SIGNAL_NULL);
        old2new[0] = awc.get_constant(false);
        tmp.foreach_ci([&](node const& i){
            awc.create_pi();
            old2new[i] = signal(i, 0);
        });
        tmp.foreach_co([&](signal const& o){
            man_dup_dfs_rec(tmp, awc, old2new, o.index);
            awc.create_po(old2new[o.index] ^ o.complement);
        });
        return awc;
    }

    /**
     * @brief duplicate the aig_with_choice in DFS topo order
     * @note here ensure that the representative node ID > the choices ID
     * @param tmp the old aig_with_choice network
     * @param awc the new aig_with_choice network
     * @param old2new a map from old node to new node
     * @param n the old node
     * @return node new created node
     */
    static signal man_dup_dfs_rec(aig_with_choice const& tmp, aig_with_choice& awc, std::vector<signal>& old2new, node const& n)
    {
        if(old2new[n] != SIGNAL_NULL) return old2new[n];
        signal new_equiv = signal(AIG_NULL, 0);
        if(tmp.get_equiv_node(n) != AIG_NULL)
        {
            new_equiv = man_dup_dfs_rec(tmp, awc, old2new, tmp.get_equiv_node(n));
        }
        auto old_child0 = tmp.get_child0(n);
        auto old_child1 = tmp.get_child1(n);
        man_dup_dfs_rec(tmp, awc, old2new, old_child0.index);
        man_dup_dfs_rec(tmp, awc, old2new, old_child1.index);
        auto new_child0 = old2new[old_child0.index] ^ old_child0.complement;
        auto new_child1 = old2new[old_child1.index] ^ old_child1.complement;
        signal new_node = awc.create_and(new_child0, new_child1);
        if(new_equiv != SIGNAL_NULL)
        {
            //assert(new_equiv.index < new_node.index);
            if(new_equiv.index < new_node.index)
            awc.set_equiv(new_node.index, new_equiv.index);
        }
        old2new[n] = new_node;
        return new_node;
    }

private:
    choice_params _params;                       ///< parameters of choice computation
    aig_network _aig;                            ///< orignal AIG network
    aig_with_choice _fraig;                      ///< final AIG network
    std::vector<signal> _old2new;                ///< the array 'final' in the paper, a map from _aig to _fraig

    cand_equiv_classes _simulator;               ///< simulator to create candidate equiv classes
    std::vector<std::vector<node>> _fanouts;     ///< fanouts for each node
    sat_prover _prover;                          ///< sat solver to do sat-prove()
    std::vector<node> _repr_proved;              ///< representatives of each node, proved by SAT

    // for resimulation
    std::vector<bool> _cex;
    std::vector<node> _sim_classes;              ///< the roots of cand equiv classes to simulate
};

iFPGA_NAMESPACE_HEADER_END
