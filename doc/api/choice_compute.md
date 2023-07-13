# choice_computation

The class [aig_with_choice](fpga-map-tool/include/operations/algorithms/choice_computation.hpp) 

| Command                                 | Description                                                    |
| :-------------------------------------- | :------------------------------------------------------------- |
| [compute_choice](#compute_choice)       | create aig with choices                                        |
| [man_sweep](#man_sweep)                 | do sat-prove() to verify equivalent nodes                      |
| [sweep_node](#sweep_node)               | to verify a node is choice or not                              |
| [resimulate](#resimulate)               | resimulate 2 non-equivalent nodes and refine the equiv classes |
| [collect_tfo_cands](#collect_tfo_cands) | collect TFO cone to do resimulate                              |
| [derive_choice_aig](#derive_choice_aig) | derive the result network with choice                          |
| [man_dup_dfs](#man_dup_dfs)             | duplicate the network in DFS topo order                        |

---

## compute_choice

create aig with choices.

```cpp
        aig_with_choice compute_choice();
```

#### Parameters

none

#### Return Value

aig_with_choice.

#### Notes

none

---

## man_sweep

do sat-prove() to verify equivalent nodes.

```cpp
    void man_sweep();
```

#### Parameters

none

#### Return Value

none

#### Notes

do sat-prove() to verify equivalent nodes will not sweep them exactly, but mark them as choices in array _repr_proved

---

## sweep_node

to verify a node is choice or not.

```cpp
    void sweep_node(node const& n, node const& new_node);
```

#### Parameters

+ n : the node in original network 
+ new_node : the node in new network(with choices)

#### Return Value

none

#### Notes

none

---

## resimulate

resimulate 2 non-equivalent nodes and refine the equiv classes.

```cpp
    void sweep_node(node const& n, node const& new_node);
```

#### Parameters

+ repr : the disproved representative node 
+ n : the disproved choice node 

#### Return Value

none

#### Notes

none

---

## collect_tfo_cands

collect TFO cone to do resimulate.

```cpp
    void collect_tfo_cands(node const& repr, node const& n);
```

#### Parameters

+ repr : the disproved representative node 
+ n : the disproved choice node 

#### Return Value

none

#### Notes

none

---

## derive_choice_aig

derive the result network with choice.

```cpp
    aig_with_choice derive_choice_aig();
```

#### Parameters

none

#### Return Value

aig_with_choice created from orignal network with array _repr_proved.

#### Notes

none

---

## man_dup_dfs

duplicate the network in DFS topo order.

```cpp
    static aig_with_choice man_dup_dfs(aig_with_choice const& tmp);
```

#### Parameters

+ tmp : the unordered network 

#### Return Value

Ordered aig_with_choice.

#### Notes

none
 
* * *
