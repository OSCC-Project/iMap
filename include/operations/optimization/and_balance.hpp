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

#include <vector>

#include "algorithms/cleanup.hpp"
#include "utils/traits.hpp"
#include "views/depth_view.hpp"

iFPGA_NAMESPACE_HEADER_START

template<class Ntk>
class and_balance
{
    using node      = typename Ntk::node;
    using signal    = typename Ntk::signal;
    static constexpr node AIG_NULL = Ntk::AIG_NULL;
    #define SIGNAL_NULL (signal(AIG_NULL, 0))

public:
    and_balance(const Ntk& aig) : _aig(depth_view(aig)), _new_aig(depth_view<Ntk>()) 
    {
        _aig.update_levels();
        _old2new.assign(_aig.size(), SIGNAL_NULL);
        _old2new[0] = signal(0, false);
        // map PIs
        _aig.foreach_pi([&](node pi) {
            _old2new[pi] = _new_aig.create_pi();
        });
        _new_aig.update_levels(); //< this is necessary to init levels of PIs
    }
    ~and_balance() {}

public:
    /// return a balanced new aig network
    Ntk run()
    {
        _aig.foreach_po([&](signal o){
            auto driver = o.index;
            signal new_signal = balance_rec(driver);
            assert(new_signal != SIGNAL_NULL);
            _new_aig.create_po(new_signal ^ o.complement);
        });

        return cleanup_dangling(_new_aig);
    }

private:
    signal balance_rec(node driver)
    {
        // return if the result is known
        if(_old2new[driver] != SIGNAL_NULL)
        {
            return _old2new[driver];
        }
        
        // get the implication supergate
        std::vector<signal> super = get_balance_cone(driver);
        // check if supergate contains two nodes in the opposite polarity
        if (super.empty())
        {
            _old2new[driver] = _new_aig.get_constant(false);
            return _old2new[driver];
        }
        
        signal new_node(SIGNAL_NULL);
        // for each old node, derive the new well-balanced node
        for(size_t i = 0; i < super.size(); ++i)
        {
            new_node = balance_rec(super[i].index);
            assert(new_node != SIGNAL_NULL);
            super[i] = new_node ^ super[i].complement;
        }

        // check for exactly one node
        if( super.size() == 1)
        {
            return super[0];
        }

        // build the supergate
        new_node = build_super(super, _aig.is_xor(driver));
        _old2new[driver] = new_node;

        return new_node;
    }

    std::vector<signal> get_balance_cone(node n)
    {
        assert(_aig.is_and(n));

        std::vector<signal> nodes;
        // collect the nodes in the implication supergate
        get_balance_cone_rec(signal(n, false), signal(n, false), nodes);

        // remove duplicates
        balance_unify(n, nodes);

        return nodes;
    }

    void balance_unify(node n, std::vector<signal>& nodes)
    {
        bool isXor = _aig.is_xor(n);

        // sort the nodes by their literal
        std::sort(nodes.begin(), nodes.end());

        // remove duplicates
        int k = 0;
        for(size_t i = 0; i < nodes.size(); ++i)
        {
            if ( i + 1 == nodes.size() )
            {
                nodes[k++] = nodes[i];
                break;
            }
            if (!isXor && nodes[i] == !nodes[i+1])
            {
                nodes.clear();
                return;
            }
            if ( nodes[i] != nodes[i + 1] )
            {
                nodes[k++] = nodes[i];
            }
            else if (isXor)
            {
                ++i;
            }
        }
        nodes.resize(k);
    }

    void get_balance_cone_rec(signal root, signal s, std::vector<signal>& super)
    {
        if( s != root &&
            (s.complement || 
            // type different
            ((_aig.is_xor(root.index) && !_aig.is_xor(s.index)) ||
             (_aig.is_and(root.index) && (_aig.is_xor(s.index) || _aig.is_pi(s.index)))
             ) ||
            _aig.fanout_size(s.index) > 1 || 
            super.size() > 10000 // ???
            )
        )
        {
            super.push_back(s);
        }
        else
        {
            assert(!s.complement);
            assert(_aig.is_and(s.index));
            get_balance_cone_rec(root, _aig.get_child0(s.index), super);
            get_balance_cone_rec(root, _aig.get_child1(s.index), super);
        }
    }

    signal build_super(std::vector<signal>& super, bool isXor)
    {
        assert(super.size() > 1);

        // sort the new nodes by level in th decreasing order
        std::sort(super.begin(), super.end(), [&](signal s0, signal s1)
        {
             int diff = _new_aig.level(s0.index) - _new_aig.level(s1.index);
             if(diff > 0)
             {
                return true;
             }
             else if( diff < 0)
             {
                return false;
             }
             else
             {
                return s0.index > s1.index;
             }
        });

        // balance the nodes
        while (super.size() > 1)
        {
            // find the left bound on the node to be paired
            int leftBound = balance_find_left(super);
            // find the node that can be shared ( if no such node, randomize choice)
            balance_permute(super, leftBound, isXor);
            // pull out the last two nodes
            auto node0 = super.back();
            super.pop_back();
            auto node1 = super.back();
            super.pop_back();
            if(isXor)
            {
                auto new_node = _new_aig.create_xor(node0, node1);
                push_unique_by_level(super, new_node, true);
            }
            else
            {
                auto new_node = _new_aig.create_and(node0, node1);
                push_unique_by_level(super, new_node, false);
            }
        }
        
        return super.empty() ? _aig.get_constant(false) : super[0];
    }

    int balance_find_left(const std::vector<signal>& super)
    {
        // if two or less nodes, pair with the first
        if( super.size() < 3)
        {
            return 0;
        }

        // set the pointer to the one before the last
        int current = super.size() - 2;
        signal right = super[current];

        signal left(SIGNAL_NULL);
        // go through the nodes to the left of this one
        for (--current; current >= 0; --current)
        {
            // get the next node on the left
            left = super[current];
            // if the level of this node is different, quit the loop
            if(_new_aig.level(left.index) != _new_aig.level(right.index))
            {
                break;
            }
        }
        ++current;
        // get the node, for which the equality holds
        left = super[current];
        assert(_new_aig.level(left.index) == _new_aig.level(right.index));
        return current;
    }

    void balance_permute(std::vector<signal>& super, int left, bool isXor)
    {
        // get the right bound
        int right = super.size() - 2;
        assert( left <= right );
        if ( left == right ) return;

        // get the two last nodes
        signal node1 = super[right + 1];
        signal node2 = super[right];
        if( _new_aig.is_constant(node1.index) || _new_aig.is_constant(node2.index) || 
           node1.index == node2.index )
        {
            return;
        }

        // find the first node that can be shared
        for (int i = right; i >= left; --i)
        {
            signal node3 = super[i];
            if(_new_aig.is_constant(node3.index))
            {
                super[i]     = node2;
                super[right] = node3;
                return;
            }

            if(node1.index == node3.index)
            {
                if(node2.index == node3.index) return;
                super[i]     = node2;
                super[right] = node3;
                return;
            }

            if(isXor)
            {
                auto old_size = _new_aig.size();
                _new_aig.create_xor(node1, node3);
                // already created
                if(_new_aig.size() == old_size)
                {
                    if(node2.index == node3.index) return;
                    super[i] = node2;
                    super[right] = node3;
                    return;
                }
            }
            else
            {
                if(_new_aig.find_and(node1, node3))
                {
                    if(node2.index == node3.index) return;
                    super[i] = node2;
                    super[right] = node3;
                    return;
                }
            }
        }
    }

    void push_unique_by_level(std::vector<signal>& super, signal new_node, bool isXor)
    {
        bool found = false;
        for(signal s : super)
        {
            if(s == new_node)
            {
                found = true;
            }
        }
        if(found)
        {
            if(isXor)
            {
                super.erase(std::remove(super.begin(), super.end(), new_node));
            }
        }
        else
        {
            super.push_back(new_node);

            // find the p of the node
            for (int i = super.size() - 1; i > 0; --i)
            {
                signal node0 = super[i];
                signal node1 = super[i - 1];
                if (_new_aig.level(node0.index) <= _new_aig.level(node1.index))
                {
                    break;
                }
                super[i]     = node1;
                super[i - 1] = node0;
            }
        }
        return;
    }
private:
    depth_view<Ntk> _aig;
    depth_view<Ntk> _new_aig;

    std::vector<signal> _old2new;
};  // end class and_balance


template<typename Ntk = iFPGA_NAMESPACE::aig_network>
Ntk balance_and(Ntk const& ntk)
{
    iFPGA_NAMESPACE::and_balance<iFPGA_NAMESPACE::aig_network> ab(ntk);
    return ab.run()._storage;
}

iFPGA_NAMESPACE_HEADER_END