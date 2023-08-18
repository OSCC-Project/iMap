<div align="center">
<h1>iMAP</h1>
</div>

```
Logic optimization and technology mapping tool.
```
### **Features**
- All the operation is mainly based on the strcuture of And-Inverter Graph (AIG) now.
- Logic Optimization
  - rewrite
  - refactor 
  - balance
  - lut_opt
- Technology mapping
  - map_fpga
- IO
  - read_aiger
  - write_aiger
  - write_fpga
  - write_verilog
  - write_dot

### **Compilation**
```
mkdir build && cd build
cmake ..
make -j 8
```
After compilation, the binary will be stored in the ${project_path}/bin/ directory.

### **Usages**
- Interactive execution
```
username@pcl-X11DPi-N-T:<project_path>/bin$ ./imap
imap> read_aiger -f ../../../benchmark/EPFL/arithmetic/adder.aig
imap> print_stats -t 0
Stats of AIG: pis=256, pos=129, area=1020, depth=255
imap> rewrite 
imap> balance 
imap> refactor 
imap> lut_opt 
imap> map_fpga 
imap> print_stats -t 1
Stats of FPGA: pis=256, pos=129, area=215, depth=75
imap> write_fpga -f adder.fpga.v
```
- batch execution \
The above step-by-step interactive execution can be performed at a time as shown below:
```
./imap -c "read_aiger -f ../../../benchmark/EPFL/arithmetic/adder.aig; print_stats -t 0; rewrite; balance; refactor; lut_opt; map_fpga; print_stats -t 1; write_fpga -f adder.fpga.v"
```

### **WIP**
- Yosys + imap;
- Sequential circuit;
- ASIC technology mapping;

---
### Contact us
- Peng Cheng Laboratory / iEDA group
- Peking University / Center for Energy-Efficient Computing and Applications
- Shanghai Anlogic Infotech CO., LTD.

If you have any question, please feel free to submit issues, or just send an email to "nlwmode AT gmail DOT com". 

<div align="center">
 <img src="https://gitee.com/oscc-project/iEDA/raw/master/docs/resources/WeChatGroup.png" width="20%" height="20%" alt="微信讨论群" />
</div>