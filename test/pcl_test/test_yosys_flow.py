import sys
import os
import re
from tkinter import Tcl
import argparse

# Parse and validate arguments
# ==============================================================================
parse = argparse.ArgumentParser( description="For fpga-map-tool program daily regression!")
parser.add_argument('--style', '-s' , required=True, help='Choose fpga or asic')
parser.add_argument('--tee', '-t' ,default='true', help='whether to log outputs or not')
args = parser.parse_args()



# using tee to log files, it will log all the yosys output files in a process stage!
# "yosys -c scripts_bw/synth.tcl 2>&1 | tee logs/log_synth_" + tclVars.DESIGN_NAME + ".txt"

if __name__ == "__main__"

    print({" yosys-abc test flow ")