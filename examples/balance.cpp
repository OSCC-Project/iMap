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
#include "io/detail/write_verilog.hpp"
#include "optimization/and_balance.hpp"
#include "database/views/depth_view.hpp"
#include "database/network/aig_network.hpp"
#include "utils/mem_usage.hpp"
#include "utils/tic_toc.hpp"
#include <cstdlib>

int main(int argc, char** argv)
{
    /**
     * usage:
     *
     *  ./bin/balance N <path/XXX.aig>
     *  eg: ./bin/balance 5 ../benchmark/adder.aig
     */
    assert(argc > 1);
    uint iteration   = std::atoi(argv[1]);
    std::string file = std::string(argv[2]);

    iFPGA_NAMESPACE::write_verilog_params ports;
    iFPGA_NAMESPACE::aig_network aig;
    iFPGA_NAMESPACE::Reader reader(file, aig, ports);

    iFPGA_NAMESPACE::tic_toc t;
    iFPGA_NAMESPACE::aig_network baig(aig);
    for(uint i = 0u; i < iteration; ++i ){
        baig = iFPGA_NAMESPACE::balance_and(baig);
    }
    printf("Statics:\n");
    printf("time                : %0.6f", t.toc());
    printf("area(before/after)  : %u/%u", aig.num_gates(), baig.num_gates());
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> aig_depth(aig);
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> baig_depth(baig);
    printf("level(before/after) : %u/%u", aig_depth.depth(), baig_depth.depth());
    printf("peak memory         : %ld bytes\n", getPeakRSS());

    if(argc > 3)
    {
        std::ostringstream out;
        write_verilog(baig, out, ports);
        std::string ofile(argv[3]);
        std::ofstream fout(ofile, std::ios::out);
        fout << out.str();
        fout.close();
    }
    return 0;
}
