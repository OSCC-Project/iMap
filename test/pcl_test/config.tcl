
set LIB_PATH "../../data/yosys-bench/celllibs"
set RTL_PATH "../../data/yosys-bench/verilog"

set LOG_PATH "log"
set RESULT_PATH "result"

set DESIGN_NAMES " \
decode_1_0 \
set_1_0 \
clr_1_0 \
"

set VERILOG_FILES " \
$RTL_PATH/benchmarks_small/decoder/decode_1_0.v \
$RTL_PATH/benchmarks_small/decoder/set_1_0.v \
$RTL_PATH/benchmarks_small/decoder/clr_1_0.v \
"

set LIBERTY_SIMPLE "$LIB_PATH/simple/simple.lib"
set LIBERTY_SUPERGATE "$LIB_PATH/supergate/supergate.lib"

#set LIB_FILES{
    #$LIB_PATH/simple/simple.lib
    #$LIB_PATH/supergate/supergate.lib
#}