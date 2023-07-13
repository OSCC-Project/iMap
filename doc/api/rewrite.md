# Rewrite

The class [rewrite_impl](fpga-map-tool/include/operations/optimization/rewrite.hpp) 

DAG-aware rewriting based on priority cut.

| Command                       | Description                                        |
| :---------------------------- | :------------------------------------------------- |
| [rewrite](#rewrite)           | main api for user                                  |
| [rewrite_impl](#rewrite_impl) | the main init procedure, including cut enumeration |
| [run](#run)                   | the main algorithm procedure                       |


---

## rewrite

the main api to run rewrite.

```cpp
template <typename Ntk         = iFPGA_NAMESPACE::aig_network,
          typename RewritingFn = node_rewriting<Ntk> >
Ntk rewrite( Ntk& ntk, rewrite_params const& params );
```

#### Templates
+ Ntk : the type of input network, default is aig_network
+ RewritingFn : the rewriting function, default is node_rewriting(fpga-map-tool/include/operations/optimization/detail/node_rewriting.hpp)

#### Parameters

+ ntk: the input logic network, default is aig_network
+ rewrite_params: params of DAG-aware rewriting, including params of cut enumeration, whether accept zero gain replacement and preserve depth

#### Return Value

Returns the logic network after DAG-aware rewriting.

#### Notes

The returned network and the input network are not the same data structure.

---

## rewrite_impl

the main init procedure, including cut enumeration.

```cpp
template <typename Ntk, typename RewritingFn>
rewrite_impl( Ntk&                   ntk,
              RewritingFn const&     rewriting_fn,
              rewrite_params const&  ps)
    : _ntk( ntk ),
      _rewriting_fn( rewriting_fn ),
      _ps( ps );
```

#### Templates
+ Ntk : the type of input network
+ RewritingFn : the rewriting function

#### Parameters

+ ntk: the input logic network, default is aig_network
+ rewriting_fn : the rewriting function
+ rewrite_params: params of DAG-aware rewriting

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

Returns the logic network after DAG-aware rewriting.

#### Notes

The returned network and the input network are not the same data structure.

* * *
