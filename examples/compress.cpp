// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Shanghai Anlogic Infotech Co.,Ltd.
// Copyright (c) 2023-2025 Peking University
//
// iMAP-FPGA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************

#include "io/reader.hpp"
#include "optimization/and_balance.hpp"
#include "optimization/rewrite.hpp"
#include "optimization/refactor.hpp"
#include "database/views/depth_view.hpp"
#include "database/network/aig_network.hpp"
#include "utils/mem_usage.hpp"
#include "utils/tic_toc.hpp"
#include <cstdlib>

int main(int argc, char **argv)
{
    /**
     * usage:
     *
     *  ./bin/compress <path/XXX.aig>
     *  eg: ./bin/compress ../benchmark/adder.aig
     */
    assert(argc > 1);
    //uint iteration = std::atoi(argv[1]);
    std::string file = std::string(argv[2]);

    iFPGA_NAMESPACE::write_verilog_params ports;
    iFPGA_NAMESPACE::aig_network aig;
    iFPGA_NAMESPACE::Reader reader(file, aig, ports);

    iFPGA_NAMESPACE::tic_toc t;
    iFPGA_NAMESPACE::aig_network caig(aig);
    iFPGA_NAMESPACE::rewrite_params  ps_rewrite;
    iFPGA_NAMESPACE::refactor_params ps_refactor;
    // compress flow: "rw -l; rf -l; b -l; rwz -l;"
    {
        ps_rewrite.b_preserve_depth = true;
        ps_rewrite.b_use_zero_gain = false;
        caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);

        caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

        caig = iFPGA_NAMESPACE::balance_and(caig);

        ps_rewrite.b_use_zero_gain = true;
        caig = iFPGA_NAMESPACE::rewrite(caig, ps_rewrite);
    }

    printf("Statics:\n");
    printf("time                : %0.6f", t.toc());
    printf("area(before/after)  : %u/%u", aig.num_gates(), caig.num_gates());
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> aig_depth(aig);
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> caig_depth(caig);
    printf("level(before/after) : %u/%u", aig_depth.depth(), caig_depth.depth());
    printf("peak memory         : %ld bytes\n", getPeakRSS());
    return 0;
}
