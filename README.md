# **fpga_map_tool**
```
 _ ______ _____   _____            __  __                             
(_)  ____|  __ \ / ____|   /\     |  \/  |                            
 _| |__  | |__) | |  __   /  \    | \  / | __ _ _ __  _ __   ___ _ __ 
| |  __| |  ___/| | |_ | / /\ \   | |\/| |/ _` | '_ \| '_ \ / _ \ '__|
| | |    | |    | |__| |/ ____ \  | |  | | (_| | |_) | |_) |  __/ |   
|_|_|    |_|     \_____/_/    \_\ |_|  |_|\__,_| .__/| .__/ \___|_|   
                                               | |   | |              
                                               |_|   |_| 
iFPGA Mapper focus on the optiomization of AIG graph and FPGA technology mapping.
For optimization, we optimize the intermediate representation of AIG(And-Inverter-Graph) on size and level. And the size and depth of AIG will highly corresponding to the area and depth of a circuit;
For technology mapping, we want to support LUT-based FPGA technology mapping firstly, expecially LUT6.
```


---
## **Build From Source**
```
for more details, please refer to the README.md file under path iFPGA/README.md
```
---
## **Related repos**
```
benchmark(ifpga_cases++)        : https://gitee.com/imap-fpga/benchmark.git
baseline abc  (modified version): https://gitee.com/imap-fpga/abc.git
verilog parser(modified version): https://gitee.com/imap-fpga/yosys.git
```

---
## **Regression**
```
for more details, please refer to the README.md file under path regression/main_test/README.md
```
---
## **Development Environment**
```markdown
* OS: ubuntu 18.04
* Ram: 16GB
* cpu cores: 12
* g++ version: 7.5.0
* c++ std: 2017
```