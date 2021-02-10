#!/bin/python3
import sys
import os
import re
import time
import argparse

### stage 1 , we set a example for test
# ==============================================================================
# Parse and validate arguments
parser = argparse.ArgumentParser( description="For fpga-map-tool program daily regression!")
parser.add_argument('--style', '-s' , required=True, help='Choose fpga or asic')
# parser.add_argument('--tee', '-t' ,default='true', help='whether to log outputs or not')
args = parser.parse_args()

### create temporary ./log ./result folder
if not os.path.exists("./log"):
    os.mkdir("./log")
if not os.path.exists("./result"):
    os.mkdir("./result")

log_name = str(int(time.time()))+"_yosys_synthed"

### execute asic of fpga according to args.style , and we should save the log file, so the tee is true
if re.search('fpga' , args.style):
    print("This is for fpga test")
    os.system('yosys -c test_fpga.tcl | tee -i ./log/{0}_fpga.log'.format(log_name))
else:
    print("This is for asic test")
    os.system('yosys -c test_asic.tcl | tee -i ./log/{0}_asic.log'.format(log_name))

# ==============================================================================

### stage 2 , in every benchmark folder, we choose one for test
# ==============================================================================
