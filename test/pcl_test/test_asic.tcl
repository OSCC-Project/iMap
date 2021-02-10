source ./config.tcl
yosys -import

#set TIME_STR [ format "%X" [ clock format [ clock seconds ] -format { %y-%m-%d-%H_%M_%S } ] ]
set TIME_STR [clock seconds]

foreach verilog_file $VERILOG_FILES module_name $DESIGN_NAMES {
    read_verilog -sv $verilog_file
    hierarchy -check
    proc
    opt
    fsm
    opt
    memory
    opt
    techmap
    opt
    dfflibmap -liberty $LIBERTY_SIMPLE
    abc -liberty $LIBERTY_SIMPLE
    clean
    set SYNTH_STR "${TIME_STR}_${module_name}_yosys_synthed"
    # tee -a $LOG_PATH/$SYNTH_STR.log
    write_verilog $RESULT_PATH/$SYNTH_STR.v
}