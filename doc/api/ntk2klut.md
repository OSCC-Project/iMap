# network_to_klut

The class [network_to_klut](fpga-map-tool/include/operations/algorithms/network_to_klut.hpp) 

| Command                             | Description                                           |
| :---------------------------------- | :---------------------------------------------------- |
| [network_to_klut](#network_to_klut) | transformed a network into klut network merged        |
| [choice_to_klut](#choice_to_klut)   | transformed a choice-network into klut network merged |

---

## network_to_klut

transformed a network into klut network merged.

```cpp
template<class NtkDest, class NtkSource>
std::optional<NtkDest> network_to_klut( NtkSource const& ntk )
```

#### Templates
+ NtkDest : type of destination-network, should be aig network
+ NtkSource : type of source-network, should be klut network

#### Parameters

+ ntk : the source-network need transformed into klut network merged

#### Return Value

klut network merged

#### Notes

none

---

## network_to_klut

transformed a network into klut network merged.

```cpp
template<class NtkDest, class NtkSource>
bool network_to_klut( NtkDest& dest, NtkSource const& ntk )
```

#### Templates
+ NtkDest : type of destination-network, should be aig network
+ NtkSource : type of source-network, should be klut network

#### Parameters

+ dest : the destination-network
+ ntk : the source-network need transformed into klut network merged

#### Return Value

wether merged successful

#### Notes

none

---

## choice_to_klut

transformed a choice-network into klut network merged.

```cpp
template<class NtkDest, class NtkSource>
std::optional<NtkDest> choice_to_klut( NtkSource const& ntk )
```

#### Templates
+ NtkDest : type of destination-network, should be choice network
+ NtkSource : type of source-network, , should be klut network

#### Parameters

+ ntk : the source-network need transformed into klut network merged

#### Return Value

klut network merged

#### Notes

none

---

## choice_to_klut

transformed a choice-network into klut network merged.

```cpp
template<class NtkDest, class NtkSource>
bool choice_to_klut( NtkDest& dest, NtkSource const& ntk );
```

#### Templates
+ NtkDest : type of destination-network, should be choice network
+ NtkSource : type of source-network, should be klut network

#### Parameters

+ dest : the destination-network
+ ntk : the source-network need transformed into klut network merged

#### Return Value

wether merged successful

#### Notes

none

* * *
