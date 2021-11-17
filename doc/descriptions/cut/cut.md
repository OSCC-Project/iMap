
### ```class cut```
The main purpose of this class is to provide the data structure and related methods for “cut” used in the mapping program. A cut is a partition of logic gates in a gate-level netlist or a partition of nodes in a directed acyclic graph (AIG). Each selected cut is implemented by a look-up table (LUT) in the FPGA later. 

**Examples**


**Main APIs**

- void cut();
  ```markdown
  description:
    1. function: construct a new object of cut
    2. input: no
    3. output: generate a new object of cut; no return value
  ```

- void cut(cut const& other);
  ```markdown
  description:
    1. function: copy constructor of the cut class
    2. input: the cut object to be copied, passed by constant reference
    3. output: generate a copy of the input object; no return value
  ```

- template<typename Iterator>
  void set_leaves( Iterator begin, Iterator end );
  ```markdown
  description:
    1. function: function template for setting the leaves of a cut by iterator
    2. input: beginning and ending leaves of the leaves to be set
    3. output: no
  ```

- template<typename Container>
  void set_leaves( Container const& c );
  ```markdown
  description:
    1. function: overloaded function template for setting the leaves of a cut by iterator
    2. input: input object, passed by constant reference
    3. output: no
  ```
  
- auto signature() const
  ```markdown
  description:
    1. function: get the signature of the cut
    2. input: no
    3. output: cut signature
  ```

- auto size() const
  ```markdown
  description:
    1. function: get the size of the cut
    2. input: no
    3. output: cut size
  ```

- bool dominates( cut const& that )
  ```markdown
  description:
    1. function: check whether the current cut dominates another cut, i.e., it is a subset of that cut
    2. input: no
    3. output: whether the current cut dominates the input cut
  ```

- bool merge( cut const& that, cut& res, uint32_t cut_size ) const;
  ```markdown
  description:
    1. function: This method merges two cuts and stores the result in `res`. The merge of two cuts is the union \f$L_1 \cup L_2\f$ of the two leaf sets \f$L_1\f$ of the current cut and \f$L_2\f$ of the other cut `that`. The merge is only successful if the union has not more than `cut_size` elements. In that case, the function returns `false`, otherwise `true`
    2. input: the other cut to be merged with the current cut, the resulting merged cut, and the cut size constraint of the merged cut
    3. output: whether the merge within the cut size constraint is successful; the merged cut
  ```



