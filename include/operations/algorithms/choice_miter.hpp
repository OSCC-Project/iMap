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

#include <map>
#include <vector>

#include "utils/ifpga_namespaces.hpp"
#include "network/aig_network.hpp"

iFPGA_NAMESPACE_HEADER_START

/**
 * @brief the pointer of each aig is stored with its corresponding ID map
 */
struct aig_with_id_map
{
    std::shared_ptr<aig_network> aig;
    std::map<uint64_t, uint64_t> id_map;
};


class choice_miter
{
public:
    using id_map     = std::map<uint64_t, uint64_t>;
    using id_pair    = std::pair<uint64_t, uint64_t>;
    using node       = iFPGA::aig_network::node;
    using signal     = iFPGA::aig_network::signal;

    choice_miter()
    {
    }

    choice_miter(std::vector<std::shared_ptr<aig_network> > const& aigs) 
    {
        for (std::shared_ptr<aig_network> aig : aigs)
        {
            add_aig(aig);
        }
    }
public:
     /**
     * @brief add aig network pointer into the list to be merged
     * @param aig the aig network pointer to be added
     * @note add the structure composed of the aig network pointer and the id map in vector(_aigs)
     */
    void add_aig(std::shared_ptr<aig_network> aig)
    {
        id_map map;
        aig_with_id_map aig_with_id_map = {aig, map};
        _aigs.push_back(aig_with_id_map);
    }

    /**
     * @brief mian function of miter class
     * @return the of final generated miter
     * @note main process of corresponding algorithm, merge aigs
     */
    aig_network merge_aigs_to_miter ()
    {
        assert(_aigs.size() > 0);
        // if only input one aig_network
        if(_aigs.size() == 1)
        {
            return *_aigs[0].aig;
        }
        
        // make sure they have equal parameters
        for(uint64_t i = 0; i < _aigs.size(); i++)
        {
            assert( _aigs[0].aig->num_pis() == _aigs[i].aig->num_pis() );
            assert( _aigs[0].aig->num_pos() == _aigs[i].aig->num_pos() );
            _aigs[i].aig->clear_visited();
        }

        // copy PIs to new aig
        for (uint64_t i = 0; i < _aigs[0].aig->num_pis(); i++)
        {
            _aig_new.create_pi();

            for (uint8_t j = 0; j < _aigs.size(); j++)
            {
                _aigs[j].id_map.insert( id_pair(i + 1, i + 1) );
            }
        }
        
        // create internal nodes
        create_internal_nodes(); 

        // delete redundant POs
        uint64_t count = 0;
        for (auto output = _aig_new._storage->outputs.begin(); output != _aig_new._storage->outputs.end(); ++output)
        {
            // only the nodes of integer multiple sequence number of _aigs.size() are reserved
            if(count % _aigs.size() != 0)
            {
                _aig_new._storage->outputs.erase(output);
                --output;
                --_aig_new._storage->data.num_pos;
            }

            ++count;
        }

        for(uint64_t i = 0; i < _aigs.size(); i++)
        {
            _aigs[i].aig->clear_visited();
        }

        // printf("id-maps:\n");
        // for(uint64_t i = 0; i < _aigs.size(); i++)
        // {
        //     printf("id-map%d : \n", i);
        //     for(auto m : _aigs[i].id_map)
        //     {
        //         printf("%d-%d   ", m.first, m.second);
        //     }
        //     printf("\n");
        // }

        return _aig_new;
    }

    /**
     * @brief clear the data of the miter object
     * @note when the miter needs to be reused, the previous data must be cleared first
     */
    void clear()
    {
        _aigs.clear();
    }
private:
    /**
     * @brief query the index of the corresponding node in miter of the node in aig
     * @param aig pointer of the aig to be query
     * @param node_index the index of the node in aig to be queried
     * @return the index of the corresponding node in miter
     */
    uint64_t get_id_in_miter(std::shared_ptr<aig_network> aig, uint64_t node_index)
    {
        for (uint64_t i = 0; i < _aigs.size(); i++)
        {
            if(aig == _aigs[i].aig)
            {
                return ( _aigs[i].id_map.find(node_index) )->second;
            }
        }

        return -1;
    }

    /**
     * @brief add the information corresponding to the node of aig and miter
     * @param aig pointer of the aig
     * @param id_in_old_aig index of corresponding node in aig
     * @param id_in_new_aig index of corresponding node in miter
     */
    void update_id_map(std::shared_ptr<aig_network> aig, uint64_t id_in_old_aig, uint64_t id_in_new_aig)
    {
       for (uint64_t i = 0; i < _aigs.size(); i++)
       {
           if(aig == _aigs[i].aig)
           {
               _aigs[i].id_map.insert( id_pair(id_in_old_aig, id_in_new_aig) );
               break;
           }
       }
    }

    /**
     * @brief add a new node corresponding to the node in aig in miter
     * @param aig pointer of the aig
     * @param index index of corresponding node in aig
     * @note according to the information of this node in aig, create a corresponding new node in miter,then update the ID map
     */
    void create_node(std::shared_ptr<aig_network> aig, uint64_t index)
    {
        // create_node
        auto node = aig->_storage->nodes[index];
        uint64_t weight0 = node.children[0].weight;
        uint64_t weight1 = node.children[1].weight;
        uint64_t child0_id = get_id_in_miter(aig, node.children[0].index);
        uint64_t child1_id = get_id_in_miter(aig, node.children[1].index);
        signal child0 = {child0_id, weight0};
        signal child1 = {child1_id, weight1};

        signal new_node = _aig_new.create_and(child0, child1);

        // update id map
        update_id_map(aig, index, new_node.index);
    }

    /**
     * @brief depth first traverses each po of the aig to build the miter
     * @param aig pointer of the aig
     * @param index index of po in aig
     * @return the returned index is used to determine the index of the po to be built later
     */
    void dfs_recursion_po (std::shared_ptr<aig_network> aig, uint64_t index)
    {
        // traversed to the PIs
        if( aig->is_pi(index) )
        {
            return;
        }

        // traversed to visited node
        if( aig->visited( aig->index_to_node(index) ) )
        {
            return;
        }

        // traverse left child
        dfs_recursion_po( aig, ( aig->get_child0(index) ).index );

        // traverse right child
        dfs_recursion_po( aig, ( aig->get_child1(index) ).index );

        // wether add this node to new_aig
        create_node(aig, index);
        aig->set_visited(aig->index_to_node(index), 1u);
        return;
    }

    /**
     * @brief building and-node and pos in miter
     * @note the outer loop is each po of aig, and the inner loop is each aig
     */
    void create_internal_nodes ()
    {
        for (uint64_t i = 0; i < _aigs[0].aig->num_pos(); i++)
        {
            for (uint64_t j = 0; j < _aigs.size(); j++)
            {
                dfs_recursion_po( _aigs[j].aig, _aigs[j].aig->po_at(i).index );
                _aig_new.create_po( signal( _aigs[j].id_map[_aigs[j].aig->po_at(i).index], _aigs[j].aig->po_at(i).complement ) );
            }
        }
    }

private: 
    std::vector<aig_with_id_map> _aigs;     ///< the vector storages the pointer of aig network to be merged and the Corresponding id map 
    aig_network _aig_new{aig_network()};   ///< the miter
};

iFPGA_NAMESPACE_HEADER_END
