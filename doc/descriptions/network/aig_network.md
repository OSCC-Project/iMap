
### ```class aig_network```
The main purpose of this class is to provide the data structure and related methods for “AIG network” used in the mapping program. 
The AIG network considered here only contains AND2 and inverter gates, where the AND2 gate is represented by the node, and the inverter is represented
by the edge (named as signal in this class).

**Examples**


**Main APIs**

- aig_network();
  ```markdown
  description:
    1. function: construct a new object of aig_network
    2. input: no
    3. output: a new object of aig_network; no return value
  ```

- aig_network( std::shared_ptr<aig_storage> storage );
  ```markdown
  description:
    1. function: assign the input storage data to the current aig_network.
    2. input: a pointer to a storage data
    3. output: the aig_network object with the input storage data
  ```

- template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  ```markdown
  description:
    1. function: apply a function to each of the two fanins of a node, assuming each node only has 2 fanins.
    2. input: the node n; the function to be applied to each fanin.
    3. output: none
  ```

- uint32_t fanin_size( node const& n ) const
  ```markdown
  description:
    1. function: get the fanin size of a node
    2. input: the node n
    3. output: the fanin size of the input node. If the node is a constant or primary input, return 0; otherwise, return 2 assuming the network is AIG.
  ```

- uint32_t fanout_size( node const& n ) const
	```markdown
	description:
    1. function: get the fanout size of a node
    2. input: the node n
    3. output: the fanout size of the input node
  ```

- uint32_t incr_fanout_size( node const& n ) const
	```markdown
	description:
    1. function: increase the fanout size of a node by 1
    2. input: the node n
    3. output: the modified fanout size of node n increased by 1
  ```

- uint32_t decr_fanout_size( node const& n ) const
	```markdown
	description:
    1. function: decrease the fanout size of a node by 1
    2. input: the node n
    3. output: the modified fanout size of node n reduced by 1
  ```

- bool is_combinational() const
	```markdown
	description:
    1. function: check whether the stored network is a combinational circuit
    2. input: none
    3. output: return true if the network is a combinational circuit; otherwise, return false
  ```

- bool is_constant( node const& n ) const
	```markdown
	description:
    1. function: check whether a node is a constant node
    2. input: the node to be checked
    3. output: return true if the node is a constant node; otherwise, return false
  ```

- bool is_ci( node const& n ) const
  ```markdown
  description:
    1. function: check whether this node represents for an input of the combinational circuit, i.e., either a primary input or an output of a register.
    2. input: the node to be checked
    3. output: return true if this node represents for an input of the combinational circuit; otherwise, return false
  ```

- bool is_pi( node const& n ) const
	```markdown
	description:
    1. function: check whether this node represents for a primary input of the whole circuit without partition
    2. input: the node to be checked
    3. output: return true if this node represents for a primary input; otherwise, return false
  ```

- bool is_ro( node const& n ) const
	```markdown
	description:
    1. function: check whether this node represents for a register output
    2. input: the node to be checked
    3. output: return true if this node represents for a register output; otherwise, return false
  ```

- uint32_t node_to_index( node const& n ) const
	```markdown
	description:
    1. function: get the index of the input node
    2. input: a node
    3. output: index of this node
  ```

- node index_to_node( uint32_t index ) const
	```markdown
	description:
    1. function: get the node corresponding to the input index
    2. input: index of a node
    3. output: the node corresponding to the input index
  ```










