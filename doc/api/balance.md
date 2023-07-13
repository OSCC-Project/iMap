# Balance

The class [and_balance](fpga-map-tool/include/operations/optimization/and_balance.hpp) 

balance based on and-tree balancing.

| Command                     | Description                  |
| :-------------------------- | :--------------------------- |
| [balance_and](#balance_and) | main api for user            |
| [run](#run)                 | the main algorithm procedure |


---

## and_balance

the main api to run and_balance.

```cpp
template<typename Ntk = iFPGA_NAMESPACE::aig_network>
Ntk balance_and(Ntk const& ntk);
```

#### Templates
+ Ntk : the type of input network, default is aig_network

#### Parameters

+ ntk: the input logic network, default is aig_network

#### Return Value

Returns the logic network after and-tree balancing.

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

Returns the logic network after and-tree balancing.

#### Notes

The returned network and the input network are not the same data structure.

* * *
