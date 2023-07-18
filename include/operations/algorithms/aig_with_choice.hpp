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

#include "utils/ifpga_namespaces.hpp"
#include "network/aig_network.hpp"

iFPGA_NAMESPACE_HEADER_START

using base_aig_network = aig_network;
/*! \brief AIG storage container with choice nodes

  AIGs have nodes with fan-in 2.  We split of one bit of the index pointer to
  store a complemented attribute.  Every node has 64-bit of additional data
  used for the following purposes:

  `data[0].h1`: Fan-out size (we use MSB to indicate whether a node is dead)
  `data[0].h2`: Application-specific value
  `data[1].h1`: Visited flag
  `data[1].h2`: flags
*/
class aig_with_choice : public base_aig_network
{
public:
    using base_type = aig_with_choice;
    using storage   = base_aig_network::storage;
    using node      = base_aig_network::node;
    using signal    = base_aig_network::signal;
    static constexpr node AIG_NULL = base_aig_network::AIG_NULL;
    static constexpr uint8_t min_fanin_size{2u};
    static constexpr uint8_t max_fanin_size{2u};


public:
    aig_with_choice(uint64_t size)
        : base_aig_network() 
    {
        init_choices(size);
    }

    aig_with_choice(base_aig_network const& aig) : aig_with_choice(aig.size()) 
    {
        base_aig_network::_storage = aig._storage;
        base_aig_network::_events = aig._events;
    }
    /**
     * @brief check the node is a representative
     * @param n the node n 
     * @return true if node n is a representative node and has one or more choice nodes 
     * @note if node n has no choices, also return false for it doesn't need extra processing in mapping
     */
    bool is_repr(node const& n) const
    {
        return get_equiv_node(n) != AIG_NULL && fanout_size(n) > 0;
    }

    /// Get the next equiv node of node n 
    node get_equiv_node(node const& n) const
    {
        return _equivs[n];
    }

    /// get the repr of this node
    node get_repr(node const& n) const
    {
        return _reprs[n];
    }

    /**
     * @brief alloc the memory for choices
     * @note this should be done before computing choice 
     */
    void init_choices(uint64_t size)
    {
        _equivs.assign(size, AIG_NULL);

        _reprs.clear();
        _reprs.reserve(size);
        for(uint64_t n = 0; n < size; ++n)
        {
            _reprs.push_back(n);
        }
    }

    /**
     * @brief add the node n to the equivalance class of signal s
     * @param n the choice node 
     * @param r a representative node r 
     * @return true if add successfully, else false because of loop
     */
    bool set_choice(node const& n, node const& r)
    {
        if(r >= n) return false;

        if(check_tfi(n, r))
        {
            return false;
        }
        // set representative
        _reprs[n] = r;
        // add n to the tail of the linked list
        node tmp = r;
        while(_equivs[tmp] != AIG_NULL)
        {
            tmp = _equivs[tmp];
        }
        assert(_equivs[tmp] == AIG_NULL);
        _equivs[tmp] = n;
        assert(_equivs[n] == AIG_NULL);
        return true;
    }


    /**
     * @brief set choice directly, when do DFS duplicate
     * @param n the node 
     * @param equiv the next equiv node of node n
     */
    void set_equiv(node const& n, node const& equiv)
    {
        _equivs[n] = equiv;
    }
    /**
     * @brief check this node is the root of MUX or EXOR/NEXOR
     * ref: abc/src/aig/aig/aigUtil.c:Aig_ObjIsMuxType() 
     */
    bool is_mux(node const& n) const
    {
        if(!is_and(n)) return false;

        // get children
        auto child0 = get_child0(n);
        auto child1 = get_child1(n);
        
        // if the children are not complemented, this is not MUX
        if(!child0.complement || !child1.complement) return false;
        
        // if the children are not ANDs, this is not MUX
        if(!is_and(child0.index) || !is_and(child1.index)) return false;

        auto child00 = get_child0(child0.index);
        auto child01 = get_child1(child0.index);
        auto child10 = get_child0(child1.index);
        auto child11 = get_child1(child1.index);

        // otherwise the node is MUX iff it has a pair of equal grandchildren
        return child00 == !child10 || child00 == !child11 ||
               child01 == !child10 || child01 == !child11;
    }

    /**
     * @brief reconize what nodes are control and data inputs of a MUX
     *  If the node is a MUX, returns the control variable C.
        Assigns nodes T and E to be the then and else variables of the MUX. 
        Node C is never complemented. Nodes T and E can be complemented.
        This function also recognizes EXOR/NEXOR gates as MUXes. 
        ref: abc/src/aig/aig/aigUtil.c:Aig_ObjRecognizeMux
     */
    void recognize_mux(node const& n, signal& i, signal& t, signal& e)
    {
        assert(is_mux(n));
        // get children
        auto child0 = get_child0(n);
        auto child1 = get_child1(n);

        auto child00 = get_child0(child0.index);
        auto child01 = get_child1(child0.index);
        auto child10 = get_child0(child1.index);
        auto child11 = get_child1(child1.index);
        // find the control variable
        if(child01 == !child11)
        {
            if(child01.complement)
            {
                i = child11;
                t = !child10;
                e = !child00; 
                return;
            }
            else
            {
                i = child01;
                t = !child00;
                e = !child10;
                return;
            }
        }
        else if(child00 == !child10)
        {
            if(child00.complement)
            {
                i = child10;
                t = !child11;
                e = !child01;
                return;
            }
            else
            {
                i = child00;
                t = !child01;
                e = !child11;
                return;
            }
        }
        else if(child00 == !child11)
        {
            if(child00.complement)
            {
                i = child11;
                t = !child10;
                e = !child01;
                return;
            }
            else
            {
                i = child00;
                t = !child01;
                e = !child10;
                return;
            }
        }
        else if(child01 == !child10)
        {
            if(child01.complement)
            {
                i = child10;
                t = !child11;
                e = !child00; 
                return;
            }
            else
            {
                i = child01;
                t = !child00;
                e = !child11;
                return;
            }
        }
        assert(0);
    }
private:
    /**
     * @brief check if r is in the TFI of n
     * @param n the choice node
     * @param r the representative node
     * @return true if r is in the TFI of n, else false
     */
    bool check_tfi(node const& n, node const& r)
    {
        bool inTFI = false;
        for(node tmp = r; tmp != AIG_NULL; tmp = _equivs[tmp])
        {
            set_value(tmp, 1); // mark as 1
        }
        // will traverse the TFI of n
        incr_trav_id();
        inTFI = check_tfi_rec(n);
        for(node tmp = r; tmp != AIG_NULL; tmp = _equivs[tmp])
        {
            set_value(tmp, 0); // unmark
        }
        return inTFI;
    }

    bool check_tfi_rec(node const& n)
    {
        if(n == AIG_NULL) return false;
        if(is_ci(n)) return false;

        // find it!
        if(value(n) == 1) return true;

        // skip visited node
        if(visited(n) == trav_id()) return false;
        set_visited(n, trav_id());

        // check children
        if(check_tfi_rec(get_child0(n).index)) return true;
        if(check_tfi_rec(get_child1(n).index)) return true;

        // check equiv nodes
        return check_tfi_rec(_equivs[n]);
    }
    
private:
    std::vector<node> _equivs;   ///< linked list of equivalent nodes
    std::vector<node> _reprs;    ///< representatives of each nodes
};

iFPGA_NAMESPACE_HEADER_END
