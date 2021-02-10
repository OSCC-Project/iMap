#!/bin/python3
import sys
import os
import re
import time
from tkinter import Tcl
import argparse

### stage 1 , we set a example for test
# ==============================================================================
# Parse and validate arguments
parser = argparse.ArgumentParser( description="For fpga-map-tool program daily regression!")
parser.add_argument('--style', '-s' , required=True, help='Choose fpga or asic')
# parser.add_argument('--design' , '-d'  , default= '' , help= 'Choose a RTL design for input')   # we should set a default verilog for test
# parser.add_argument('--liberty' , '-l' , default= '' , help= 'the liberty file for synthesis')
# parser.add_argument('--tee', '-t' ,default='true', help='whether to log outputs or not')
args = parser.parse_args()

design_name = "../../data/yosys-bench/verilog/benchmarks_small/decoder/decode_3_1.v"
lib_name = "../../data/yosys-bench/celllibs/simple/simple.lib"
# log_name = time.strftime("%Y-%m-%d-%H_%M_%S_" ,time.localtime(time.time())) + "synthed"
log_name = str(int(time.time()))+"_yosys_synthed"

header = "This is a PCL test log file!"
input_args = "Just for test!"
# input_args = "set_style:{style};\nset_design:{design};\nset_liberty:{liberty}\n;set_tee:{tee}\n".format(style=args.style , design=design_name , liberty=lib_name , tee=log_name)

### create temporary ./log ./result folder
if not os.path.exists("./log"):
    os.mkdir("./log")
if not os.path.exists("./result"):
    os.mkdir("./result")

### execute asic of fpga according to args.style , and we should save the log file, so the tee is true
if re.search('fpga' , args.style):
    print("This is for fpga test")
    os.system('yosys -c test_fpga.tcl | tee -i ./log/{0}_fpga.log'.format(log_name))
 # os.system('mkdir log ; touch ./log/{1}_fpga.log ; echo "{3}" > ./log/{1}_fpga.log ; echo “{2}” > ./log/{1}_fpga.log ; yosys | read_verilog{0} | hierarchy -check | proc | opt | fsm | opt | memory | opt | techmap | opt | tee -ia ./log/{1}_fpga.log'.format(design_name , log_name , input_args , header))
else:
    print("This is for asic test")
    os.system('yosys -c test_asic.tcl | tee -ia ./log/{0}_asic.log'.format(log_name))

# ==============================================================================

### stage 2 , in every benchmark folder, we choose one for test
# ==============================================================================
