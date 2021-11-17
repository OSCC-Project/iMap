
### ```class klut_network```
The main purpose of this class is to provide the data structure and related methods for “k-LUT network” used in the mapping program. 
The k-LUT network contains a set of nodes and a set of edges. Each node represents for a k-bounded LUT with fanin number not exceeding k.
Each edge represents for the connection wire between the LUTs. k-LUT network only represents for a combinational circuit.

**Examples**


**Main APIs**

- signal create_pi( std::string const& name = std::string());
  ```markdown
  description:
    1. function: create a new signal as a primary input with the input name
    2. input: name of this primary input
    3. output: a new primary input signal with the input name
  ```

- signal create_ro( std::string const& name = std::string())
  ```markdown
  description:
    1. function: create a new signal for the register output with the input name
    2. input: name of this register output
    3. output: a new register output signal with the input name
  ```

- bool is_ci( node const& n ) const
  ```markdown
  description:
    1. function: check if this node represents for an input of the combinational circuit
    2. input: a node
    3. output: return true if this node represents for an input of the combinational circuit; otherwise, return false
  ```

- bool is_pi( node const& n ) const
  ```markdown
  description:
    1. function: check if this node represents for a primary input of the whole circuit
    2. input: a node
    3. output: return true if this node represents for a primary input; otherwise, return false
  ```

- bool is_function( node const& n) const 
  ```markdown
  description:
    1. function: check if this node represents for a LUT with a certain Boolean function
    2. input: a node
    3. output: return true if this node represents for a LUT; otherwise, return false
  ```

- template<typename Fn>
  void foreach_gate( Fn&& fn) const
  ```markdown
  description:
    1. function: apply the input function to each LUT in the network
    2. input: a function handle
    3. output: none
  ```

- template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn) const
  ```markdown
  description:
    1. function: apply the input function to each fanin of the input node
    2. input: a node and a function handle
    3. output: none
  ```

- std::vector<uint32_t> get_detailed_lut_statics(uint32_t k) const
  ```markdown
  description:
    1. function: get detailed statistics of the used LUTs with fanin not exceeding k
    2. input: LUT size limit k
    3. output: detailed information of LUT utilization
  ```










