
### ```class cut```
The main purpose of this class is to provide the data structure and related methods for “cut set” used in the mapping program. 
A cut set is a set of nodes which are the leaves of a certain cut.

**Examples**


**Main APIs**

- void cut_set();
  ```markdown
  description:
    1. function: construct a new object of cut set
    2. input: no
    3. output: generate a new object of cut set; no return value
  ```

- void repointer();
  ```markdown
  description:
    1. function: set the pointers pointing to the new memory chunk.
    2. input: no
    3. output: no
  ```

- void empty();
  ```markdown
  description:
    1. function: release memory
    2. input: no
    3. output: no
  ```

- void resize_to_fit();
  ```markdown
  description:
    1. function: release the tail-data memory which is not used
    2. input: no
    3. output: no
  ```

- auto const& best() const;
  ```markdown
  description:
    1. function: get the best priority cut
    2. input: no
    3. output: the best priority cut
  ```

- template<typename Iterator>
  CutType& add_cut(Iterator begin, Iterator end)
  ```markdown
  description:
    1. function: add new leaves to the cut set object
    2. input: the beginning and ending iterators of the leaves of a certain cut
    3. output: the cut with added leaves
  ```

- bool is_dominated(CutType const& cut) const
  ```markdown
  description:
    1. function: check whether the current cut is dominated by the input cut
    2. input: the cut to be checked to dominate the current cut
    3. output: whether the current cut is dominated by the input cut
  ```

- void insert(CutType const& cut)
  ```markdown
  description:
    1. function: insert a cut into the storage array "_cuts[]"
    2. input: the cut to be inserted
    3. output: no
  ```

- void update_best( uint32_t index)
  ```markdown
  description:
    1. function: insert the best cut to the beginning of storage array "_pcuts[]", and maintain the order of the original elements.
    2. input: the index of the best cut
    3. output: no
  ```
  
- void limit( uint32_t size)
  ```markdown
  description:
    1. function: limit the cut size by a fixed value. Prune those cuts with sizes exceeding the limit.
    2. input: the cut size limit
    3. output: no
  ```
  
- void remove( uint32_t index)
  ```markdown
  description:
    1. function: remove a cut from the stogate array "_pcuts[]"
    2. input: the index of the cut to be removed
    3. output: no
  ```

- void resize(uint8_t size)
  ```markdown
  description:
    1. function: update the iterators "_pcend" and "_pend" by resizing
    2. input: the size after resizing.
    3. output: no
  ```

























