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

## **Build From Souce**
This project is based on C++17, so you need a C++ compiler with C++17 support(above gcc5). So please check your gcc version:
```
$ gcc --version
```
otherwise the version is under gcc5, you can install supported gcc by:
```
$ sudo apt install gcc-7 g++-7
```

Then we can compile this project by:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..   // cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
$ sudo make install                     // optional
```

---
## **Getting Started**
We only support "command + configation" for now, so if you want to use iFPGA-map-tool, you need to config you configation file before run command "ifpga". \
the args of ifpga is easy to understand, if you do not know how to run, you can following:
```
$ ./path/ifpga -h                       // if you compile iFPGA-map-tool with "sudo make install", just using "ifpga -h" is enough
```
For now, we want to support choice-based flow ,so the usage for ifpga is temporary: \
we assume our file tree like below:
```
$ cd iFPGA/
> bin/ifpga
> benchmark/
> config/
```
And you can run ifpga like commands below:
```
$ ./bin/ifpga -i benchmark/adder/adder.aig -c config/consumer_config.json -v adder.v -l adder.lut.v

//old
specifications:
  -i : the choice files folder which contains original.aig, compress.aig, compress2.aig
  -c : the configuration file path
  -v : the write out verilog file
  -l : the write out lut-based file 
```
the configuration file is mainly about some switches of the flow control.
```
flow_manager:
  params:
    i_opt_iterations: 0
    b_use_balance: true
    b_use_rewrite: true
```

---
## **Related repos**
```
benchmark(ifpga_cases++)        : https://gitee.com/imap-fpga/benchmark.git
regression                      : https://gitee.com/imap-fpga/regression.git
docs                            : https://gitee.com/imap-fpga/docs.git
baseline abc  (modified version): https://gitee.com/imap-fpga/abc.git
verilog parser(modified version): https://gitee.com/imap-fpga/yosys.git
```

---
## **Regression**
```
for more details, please refer to regression/main_test/README.md
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