# fpga_map_tool

FPGA Logic Synthesis test.

---

### Folder Descriptions

* [doc/](doc/README.md) folder: Stores some documents.
* [res/](res/README.md) folder: Stored some results that from test.
* [src/](src/README.md) folder: Stored our source codes.
* [test/](test/README.md) folder: Stored out test codes.
* [data/](data/README.md) folder: Stored some data that may be used.
* [config/](config/README.md) folder: Stored some configure files.

---
### Benchmarks
for more benchmarks, you can refer to https://gitee.com/nlwmode_personal/benchmark.git
* [EPFL benchmarks](https://github.com/fpga-tool-org/benchmarks)
* [Yosys benchmarks](https://github.com/fpga-tool-org/yosys-bench)
* [IWLS 2005](http://iwls.org/iwls2005/benchmarks.html)
* MCNC
* LGSynth91

---
### Environment

* OS: ubuntu 18.04 WSL(Windows Subsystem for Linux)
* Ram: 16GB
* cpu cores: 12
* g++ version: 7.5.0
* c++ std: 2017

---
### Build

1. Setup our whole project
  ```
      git clone https://github.com/fpga-tool-org/fpga-map-tool.git
      cd fpga-map-tool
  ```

2. Install Logic Synthesis tool - [Yosys](src/yosys)
  ```
      sudo apt-get install build-essential clang bison flex \
    libreadline-dev gawk tcl-dev libffi-dev git \
      libboost1.71-dev libboost-system1.71-dev \
    graphviz xdot pkg-config python3 libboost-system-dev \
    libboost-python-dev libboost-filesystem-dev zlib1g-dev

      /* jump to the yosys fpga-map-tool/src folder*/
      cd src/yosys

      make config-clang

      mkdir build
      cd build
      make -f ../Makefile
      sudo make install -f ../Makefile
  ```

3. Now we can test the verilog file using yosys!
  ```
  for example: 
      cd benchmarks/verilog/IWLS2005
      yosys
      read_verilog ac97_ctrl.v
      hierarchy -check
      proc; opt; fsm; opt; memory; opt
      techmap; opt
      dfflibmap -liberty ../../../celllibs/simple/simple.lib
      abc -liberty ../../../celllibs/simple/simple.lib
      clean
      /* write synthesized design*/
      write_verilog synth.v
  ```
---
### Build-iFPGA
  ```
  1. cd src/iFPGA
  2. mkdir build && cd build
  3. sudo cmake ..
  4. make
  5. sudo make install  # install ifpga to /usr/local/bin
  6. ifpga -i [input_path] -c [config_file_path]
  (the detail usage follow the readme.md at src/iFPGA/readme.md)  
  ```

--- 
### Example FLow
we integrated the ifpga into yosys, so you can directly using ifpga in yosys commands.
  ```
  yosys
  yosys> read_aiger [xxx.aig]
  yosys> hierarchy -check
  yosys> proc;opt
  yosys> fsm;opt
  yosys> memory;opt
  yosys> ifpga      # if with the default confie file
  yosys> ifpga -c [xxx.yaml]     # if with specified confie file
  ```
---

### Regression
we analyze our fpga mapping result in the step.
```
  # see [test/anlogic_test/README.md](test/anlogic_test/README.md) for details
  cd fpga-map-tool/test/anlogic_test/
  python3 scripts/run_test.py config.ini 
```
---
### Daily Build
*We use Jenkins for auto daily build and regression*

- Our Server: ubuntu 20.04.2 LTS
- RAM: 64GB
- cpu cores: 64

```
  
  /* isntall java for jenkins dependency*/
  sudo apt install openjdk-11-jdk
  /* check*/
  java -version

  /* install Weekly releaseed jenkins */
  wget -q -O - https://pkg.jenkins.io/debian/jenkins.io.key | sudo apt-key add -
  sudo sh -c 'echo deb https://pkg.jenkins.io/debian binary/ > /etc/apt/sources.list.d/jenkins.list'
  sudo apt-get update
  sudo apt-get install jenkins
  
  /* check*/
  systemctl start jenkins

  /* start jenkins*/
  open your browser, and input http://localhost:8080 to install plugins and config it!
```

