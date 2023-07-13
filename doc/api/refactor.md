# Refactor

The class [rewrite_impl](fpga-map-tool/include/operations/optimization/refactor.hpp) 

sop refactoring baesd on reconvergence-driven cut.

| Command                                                               | Description                                 |
| :-------------------------------------------------------------------- | :------------------------------------------ |
| [refactor](#refactor)                                                 | main api for user                           |
| [refactor_impl](#refactor_impl)                                       | init procedure                              |
| [run](#run)                                                           | the main algorithm procedure                |
| [compute_reconvergence_driven_cut](#compute_reconvergence_driven_cut) | compute reconvergence_driven_cut for a node |
| [collect_cone_nodes](#collect_cone_nodes)                             | collect nodes in cut                        |
| [compute_cone_tt](#compute_cone_tt)                                   | compute truth table of the cone             |
| [replace_sub_ntk](#replace_sub_ntk)                                   | run Isop refactor and replace sub ntk       |

---

## refactor

the main api to run refactor.

```cpp
template <typename Ntk = aig_network,
          class RefactoringFn = sop_factoring<Ntk>,
          typename NodeCostFn = unit_cost<Ntk>>
Ntk refactor(Ntk &ntk, refactor_params const &params);
```

#### Templates
+ Ntk : the type of input network, default is aig_network
+ RewritingFn : the refactoring function, default is sop_refactoring(fpga-map-tool/include/operations/optimization/detail/sop_refactoring.hpp)
+ NodeCostFn : the cost function compute cost of each node, defaoult cost of each node is 1u.  

#### Parameters

+ ntk: the input logic network, default is aig_network
+ rewrite_params: params of refactor

#### Return Value

Returns the logic network after refactor.

#### Notes

The returned network and the input network are not the same data structure.

---

## refactor_impl

the main init procedure.

```cpp
template <typename Ntk, class RefactoringFn, typename NodeCostFn>
refactor_impl(Ntk &ntk,
              RefactoringFn const &refactoring_fn,
              const refactor_params &params,
              const NodeCostFn &node_cost_fn)
    : _ntk(ntk),
      _refactoring_fn(refactoring_fn),
      _params(params),
      _node_cost_fn(node_cost_fn)
```

#### Templates
+ Ntk : the type of input network, default is aig_network
+ RewritingFn : the refactoring function, default is sop_refactoring(fpga-map-tool/include/operations/optimization/detail/sop_refactoring.hpp)
+ NodeCostFn : the cost function compute cost of each node, defaoult cost of each node is 1u.  

#### Parameters

+ ntk: the input logic network, default is aig_network
+ refactoring_fn : the refactoring function
+ rewrite_params: params of refactor
+ node_cost_fn : the cost function compute cost of each node

#### Return Value

none

#### Notes

none

---

## run

the main algorithm procedure.

```cpp
  Ntk run();
```

#### Templates
none

#### Parameters

none

#### Return Value

Returns the logic network after sop refactoring.

#### Notes

The returned network and the input network are not the same data structure.

---

## compute_reconvergence_driven_cut

Finds a fanin-limited, reconvergence-driven cut for the node.

```cpp
    void compute_reconvergence_driven_cut(node<Ntk> root);
```

#### Templates
none

#### Parameters

+ root : the node need to compute reconvergence-driven cut

#### Return Value

none

#### Notes

none

---

## collect_cone_nodes

collect the nodes contained in the cut

```cpp
    void collect_cone_nodes(node<Ntk> root);
```

#### Templates
none

#### Parameters

+ root : the node need to collect the nodes contained in the cut

#### Return Value

none

#### Notes

none

---

## compute_cone_tt

compute truth table of the cone

```cpp
    kitty::dynamic_truth_table compute_cone_tt(node<Ntk> root);
```

#### Templates
none

#### Parameters

+ root : the node need to compute truth table

#### Return Value

The truth table of this cone

#### Notes

none

---

## replace_sub_ntk

run Isop refactor and replace sub ntk

```cpp
        void replace_sub_ntk(node<Ntk> root, kitty::dynamic_truth_table const &tt);
```

#### Templates
none

#### Parameters

+ root : the node need to run local replacement
+ tt : the truth table of this cone

#### Return Value

none

#### Notes

none

* * *
