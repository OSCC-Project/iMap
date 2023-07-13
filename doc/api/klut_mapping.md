# klut_mapping

The class [klut_mapping](fpga-map-tool/include/operations/algorithms/klut_mapping.hpp) 

| Command                                         | Description                                                                              |
| :---------------------------------------------- | :--------------------------------------------------------------------------------------- |
| [klut_mapping](#klut_mapping)                   | create aig with choices                                                                  |
| [init_parameters](#init_parameters)             | nitialize the parameters to default value                                                |
| [perform_mapping](#perform_mapping)             | combinational k-LUT mapping                                                              |
| [perform_mapping_round](#perform_mapping_round) | the mapping rounds in every step                                                         |
| [derive_final_mapping](#derive_final_mapping)   | perform best cut selection for mapped node select best cut according the _map_ref vector |

---

## klut_mapping

create aig with choices.

```cpp
mapping_qor_storage klut_mapping(Ntk& ntk, klut_mapping_params const& ps = {}, klut_mapping_stats* pst = nullptr );
```

#### Parameters

+ ntk : aig_with_choice 

#### Return Value

QoR storagement.

#### Notes

none

---

## init_parameters

initialize the parameters to default value.

```cpp
    void init_parameters();
```

#### Parameters

none

#### Return Value

none

#### Notes

none

---

## perform_mapping

combinational k-LUT mapping.

```cpp
    void perform_mapping();
```

#### Parameters

none

#### Return Value

none

#### Notes

none

---

## perform_mapping_round

the mapping rounds in every step.

```cpp
    void perform_mapping_round(int mode, bool preprocess, bool first);
```

#### Parameters

 + mode       : ETC_DELAY, ETC_FLOW, ETC_AREA   
 + preprocess : toogles for preprocess
 + first      : mark the first mapping round

#### Return Value

none

#### Notes

none

---

## derive_final_mapping

perform best cut selection for mapped node select best cut according the _map_ref vector.

```cpp
    void derive_final_mapping();
```

#### Parameters

none

#### Return Value

none

#### Notes

none

* * *
