### ```class cut_enumeration_impl```
The main purpose of this class is to provide the data structure and related methods for “cut_enumeration_impl” used in the mapping program. 
It enumerates the cuts for each node in the DAG of the circuit network.


**Examples**


**Main APIs**

- void run()
  ```markdown
  description:
    1. function: the overall flow for enumerating the cuts for each node
    2. input: no
    3. output: enumerated cuts for each node
  ```
  
- uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
  ```markdown
  description:
    1. function: compute the truth table for each cut rooted from the node with the input index
    2. input: index of the root node; vcuts[] for the left and right child cuts rooted from this node; "res" is the resulting cut with the computed truth table. 
    3. output: truth table for the input cut
  ```
  
- void merge_cuts2( uint32_t index )
  ```markdown
  description:
    1. function: merge 2 cuts rooted from the same node. Only applicable to the fanin-2 node.
    2. input: index of the root node
    3. output: the merged cut at the root node if the merging is possible
  ```

- void merge_cuts( uint32_t index )
  ```markdown
  description:
    1. function: merge multiple cuts rooted from the same node. It is applicable to the multiple fanin node.
    2. input: index of the root node
    3. output: the merged cut at the root node if the merging is possible
  ```


- template<typename Ntk, bool ComputeTruth, typename CutData>
network_cuts<Ntk, ComputeTruth, CutData> cut_enumeration( Ntk const& ntk, cut_enumeration_params const& ps, cut_enumeration_stats * pst )
  ```markdown
  description:
    1. function: implement cut enumeration for the whole network
    2. input: "ntk" is the whole network; "ps" holds the set of cut enumeration parameters; "pst" holds the cut enumeration statistics.
    3. output: the network with possible cuts rooted from each node
  ```















  
  