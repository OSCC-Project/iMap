# Choice_miter

The class [aig_with_choice](fpga-map-tool/include/operations/algorithms/choice_miter.hpp) 

| Command                                         | Description                                                    |
| :---------------------------------------------- | :------------------------------------------------------------- |
| [add_aig](#add_aig)                             | add aig network pointer into the list to be merged             |
| [merge_aigs_to_miter](#merge_aigs_to_miter)     | merge aigs to a miter aig network                              |
| [clear](#clear)                                 | clear the data of the miter object                             |
| [update_id_map](#update_id_map)                 | add the information corresponding to the node of aig and miter |
| [create_node](#create_node)                     | add a new node corresponding to the node in aig in miter       |
| [dfs_recursion_po](#dfs_recursion_po)           | depth first traverses each po of the aig to build the miter    |
| [create_internal_nodes](#create_internal_nodes) | building and-node and pos in miter                             |

---

## add_aig

add aig network pointer into the list to be merged.

```cpp
    void add_aig(std::shared_ptr<aig_network> aig);
```

#### Parameters

+ aig : the aig network pointer to be added

#### Return Value

none

#### Notes

Add the structure composed of the aig network pointer and the id map in vector(_aigs).

---

## merge_aigs_to_miter

merge aigs to a miter aig network.

```cpp
aig_network merge_aigs_to_miter ();
```

#### Parameters

none

#### Return Value

the of final generated miter.

#### Notes

main process of corresponding algorithm, merge aigs.

---

## clear

clear the data of the miter object.

```cpp
    void clear();
```

#### Parameters

none

#### Return Value

none

#### Notes

When the miter needs to be reused, the previous data must be cleared first.

---

## update_id_map

add the information corresponding to the node of aig and miter.

```cpp
    void update_id_map(std::shared_ptr<aig_network> aig, uint64_t id_in_old_aig, uint64_t id_in_new_aig);
```

#### Parameters

 + aig : pointer of the aig
 + id_in_old_aig : index of corresponding node in aig
 + id_in_new_aig : index of corresponding node in miter

#### Return Value

none

#### Notes

none

---

## create_node

add a new node corresponding to the node in aig in miter.

```cpp
    void create_node(std::shared_ptr<aig_network> aig, uint64_t index);
```

#### Parameters

 + aig : pointer of the aig
 + index : index of corresponding node in aig

#### Return Value

none

#### Notes

According to the information of this node in aig, create a corresponding new node in miter,then update the ID map.

---

## dfs_recursion_po

depth first traverses each po of the aig to build the miter.

```cpp
    void dfs_recursion_po (std::shared_ptr<aig_network> aig, uint64_t index);
```

#### Parameters

 + aig : pointer of the aig
 + index : index of po in aig

#### Return Value

The returned index is used to determine the index of the po to be built later.

#### Notes

none

---

## create_internal_nodes

building and-node and pos in miter.

```cpp
    void create_internal_nodes ();
```

#### Parameters

none
#### Return Value

none

#### Notes

The outer loop is each po of aig, and the inner loop is each aig.

* * *
