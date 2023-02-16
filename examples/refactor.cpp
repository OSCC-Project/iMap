/**
 * @file refactor.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief  test for refactor
 * @version 0.1
 * @date 2022-10-19
 *
 * @copyright Copyright (c) 2022
 *
 */

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
