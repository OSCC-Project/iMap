#include "cache.hpp"

#include "command/io/read_aiger.hpp"
#include "command/io/write_aiger.hpp"
#include "command/io/write_fpga.hpp"
#include "command/io/write_verilog.hpp"
#include "command/io/write_dot.hpp"

#include "command/rewrite.hpp"
#include "command/refactor.hpp"
#include "command/balance.hpp"
#include "command/lut_opt.hpp"
#include "command/map_fpga.hpp"
#include "command/cleanup.hpp"
#include "command/print_stats.hpp"
#include "command/history.hpp"
ALICE_MAIN(imap)