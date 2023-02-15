# **iFPGA_map_tool**
```
 _ ______ _____   _____            __  __                             
(_)  ____|  __ \ / ____|   /\     |  \/  |                            
 _| |__  | |__) | |  __   /  \    | \  / | __ _ _ __  _ __   ___ _ __ 
| |  __| |  ___/| | |_ | / /\ \   | |\/| |/ _` | '_ \| '_ \ / _ \ '__|
| | |    | |    | |__| |/ ____ \  | |  | | (_| | |_) | |_) |  __/ |   
|_|_|    |_|     \_____/_/    \_\ |_|  |_|\__,_| .__/| .__/ \___|_|   
                                               | |   | |              
                                               |_|   |_| 
iFPGA-map-tool focus on the optiomization of AIG graph and FPGA technology mapping.
For optimization, we optimize the intermediate representation of AIG(And-Inverter-Graph) on size and level. And the size and depth of AIG will highly corresponding to the area and depth of a circuit;
For technology mapping, we want to support LUT-based FPGA technology mapping firstly, expecially LUT6.
```

---

## **Folder Descriptions**

* [doc/](doc/README.md) stores some documents.
* [iFPGA/](iFPGA/READMEmd) stores our source codes.
* [regression/](test/README.md) stores our regression and daily build scripts.

---
## **Benchmarks**
* [EPFL benchmarks](https://github.com/fpga-tool-org/benchmarks)
* [Yosys benchmarks](https://github.com/fpga-tool-org/yosys-bench)
* [IWLS 2005](http://iwls.org/iwls2005/benchmarks.html)
* MCNC
* LGSynth91
```
for more details, please refer to https://gitee.com/imap-fpga/benchmark
```

---
## **Development Environment**

* OS: ubuntu 18.04
* Ram: 16GB
* cpu cores: 12
* g++ version: 7.5.0
* c++ std: 2017

---
## **Build From Source**
for more details, please refer to the README.md file under path iFPGA/README.md

---
## **Regression**
for more details, please refer to the README.md file under path regression/main_test/README.md

---
## **LICENSE**
MIT License \
Copyright (c) 2021 Peng Cheng Lab; Peking University; Shanghai Anlogic Infotech Co., Ltd.