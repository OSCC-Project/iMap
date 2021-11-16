### **flow_manager**
```markdown
flow manager class is for control the whole mapping flow.
the whole mapping flow is depicted as below:
```
![flow manager](../pics/flow-manager.png)


- **Examples**
  ```c++
    using namespace ifpga;
    string input, config, output_verilog_path, output_lut_path;
    ... // feed the string params up
    flow_manager manager(input, config, output_verilog_path, output_lut_path );
    manager.run();
  ```

- **Main APIs**
  - **void run()**;
  ```markdown
    1. function: the main process center of the flow_manager, the detail flow depicted as the flow diagram above.
    2. params  : none.
    3. return  : none.
  ```
  - **void configure_params()**;
  ```markdown
    1. function: get the assignment from the config file. internal_config is for the programmer, consumer_config is for the user.
    2. params  : none.
    3. return  : none.
  ```
  - **void update_min_network()**;
  ```markdown
    1. function: update the min_nodes or min_depth network after each optimization.
    2. params  : none.
    3. return  : none.
  ```
  - **tuple<klut_network, mapping_view<aig_network, true>, mapping_qor_storage> mapper(aig_network& aig)**;
  ```markdown
    1. function: the klut_mapping algorithms for FPGA mapping.
    2. params  : 
      aig: the input network for the design.
    3. return  : 
      tuple<klut_network, mapping_view<aig_network, true>, mapping_qor_storage>: the mapped klut_network, the mapped aig_network and the QoR of this mapping result. the mapped aig_network is for the multi-rounds mapping algorithms.
  ```
  - **bool compare_mapping(const klut_network& klut1, const mapping_qor_storage& qor1, const klut_network& klut2, const mapping_qor_storage& qor2)**;
  ```markdown
    1. function: we focus on the min nodes and depth optimizated network, so we want to mapped this 2 network and choose the best mapped network by this compare_mapping.
    2. params  : 
      klut1, qor1: the first mapped network and QoR;
      klut2, qor2: the second mapped network and QoR.
    3. return  : 
      bool: true for the first is better then second, otherwise.
  ```
  - **void report(const klut_network& klut, mapping_qor_storage qor)**;
  ```markdown
    1. function: printf the mapping result for test regression.
    2. params  : 
      klut, qor: the mapped result of the optimized network.
    3. return  : nopne.
  ```
  
