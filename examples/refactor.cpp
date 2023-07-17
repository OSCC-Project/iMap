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

#include <cstdlib>
#include <fstream>

#include "algorithms/equivalence_checking.hpp"
#include "algorithms/miter.hpp"
#include "database/network/aig_network.hpp"
#include "database/views/depth_view.hpp"
#include "io/detail/write_verilog.hpp"
#include "io/reader.hpp"
#include "optimization/refactor.hpp"
#include "utils/mem_usage.hpp"
#include "utils/tic_toc.hpp"

int main(int argc, char **argv)
{
    std::string file = std::string(argv[1]);
    iFPGA_NAMESPACE::tic_toc t;

    iFPGA_NAMESPACE::write_verilog_params ports;
    iFPGA_NAMESPACE::aig_network aig, aig2;
    iFPGA_NAMESPACE::Reader reader(file, aig, ports);
    iFPGA_NAMESPACE::Reader reader2(file, aig2, ports);
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> aig_depth(aig);

    std::cout << std::endl;
    std::cout << "!!!!!!!!!! START !!!!!!!!!!" << std::endl;
    std::cout << "Statics:\n"
              << std::endl;
    std::cout << "origin aig  , area : " << aig.num_gates() << ", level: " << aig_depth.depth() << std::endl;

    iFPGA_NAMESPACE::aig_network raig;
    iFPGA_NAMESPACE::refactor_params ps;
    t.tic();

    raig = iFPGA_NAMESPACE::refactor(aig, ps);

    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> raig_depth(raig);
    std::cout << "refactor aig, area : " << raig.num_gates() << " , level: " << raig_depth.depth() << std::endl;
    std::cout << "time                : " << t.toc() << " s" << std::endl;
    std::cout << "peak memory         : " << getPeakRSS() << " bytes" << std::endl;

    auto mit = *iFPGA_NAMESPACE::miter<iFPGA_NAMESPACE::aig_network, iFPGA_NAMESPACE::aig_network>(aig2, raig);
    auto result = iFPGA_NAMESPACE::equivalence_checking(mit);
    std::cout << "equivalence checking result : ";
    if (result.value() == true)
    {
        std::cout << "true\n"
                  << std::endl;
    }
    else
    {
        std::cout << "false\n"
                  << std::endl;
    }
    std::cout << "!!!!!!!!!! END !!!!!!!!!!\n"
              << std::endl;

    return 0;
}
