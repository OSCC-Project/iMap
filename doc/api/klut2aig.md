# convert_to_aig

The class [convert_to_aig](fpga-map-tool/include/operations/algorithms/detail/convert_to_aig.hpp) 

| Command                                                     | Description                             |
| :---------------------------------------------------------- | :-------------------------------------- |
| [convert_klut_to_aig](#convert_klut_to_aig)                 | convert a klut network to a aig network |
| [convert_mapped_graph_to_aig](#convert_mapped_graph_to_aig) | convert a network to a aig network      |

---

## convert_klut_to_aig

convert a klut network to a aig network.

```cpp
iFPGA_NAMESPACE::aig_network convert_klut_to_aig(iFPGA_NAMESPACE::klut_network const &klut);
```

#### Parameters

+ klut : the input klut network

#### Return Value

The aig network converted from input klut network.

#### Notes

none

---

## convert_mapped_graph_to_aig

convert a network to a aig network.

```cpp
template<typename Ntk>
iFPGA_NAMESPACE::aig_network convert_mapped_graph_to_aig(Ntk const &lutnet);
```

#### Templates
+ Ntk : type of input network， should be klut network

#### Parameters

+ lutnet : the input network

#### Return Value

The aig network converted from input network.

#### Notes

none

* * *
