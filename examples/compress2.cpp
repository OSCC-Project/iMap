/**
 * @file compress2.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief  test for compress2
 * @version 0.1
 * @date 2022-10-19
 *
 * @copyright Copyright (c) 2022
 *
 */
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
     *  ./bin/compress2 <path/XXX.aig>
     *  eg: ./bin/compress2 ../benchmark/adder.aig
     */
    assert(argc > 1);
    //uint iteration = std::atoi(argv[1]);
    std::string file = std::string(argv[2]);

    iFPGA_NAMESPACE::write_verilog_params ports;
    iFPGA_NAMESPACE::aig_network aig;
    iFPGA_NAMESPACE::Reader reader(file, aig, ports);

    iFPGA_NAMESPACE::tic_toc t;
    iFPGA_NAMESPACE::aig_network caig(aig);
    iFPGA_NAMESPACE::rewrite_params ps_rewrite;
    iFPGA_NAMESPACE::refactor_params ps_refactor;
    // compress2 flow: "rw ; rf -l; b -l; rw -l; rwz -l;  b -l; rfz -l; rwz -l; b -l"
    {
        ps_rewrite.preserve_depth = false;
        ps_rewrite.use_zero_gain = false;
        caig = iFPGA_NAMESPACE::rewrite_online(caig, ps_rewrite);
        ps_rewrite.preserve_depth = true;

        caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

        caig = iFPGA_NAMESPACE::balance_and(caig);

        caig = iFPGA_NAMESPACE::rewrite_online(caig, ps_rewrite);

        ps_rewrite.use_zero_gain = true;
        caig = iFPGA_NAMESPACE::rewrite_online(caig, ps_rewrite);

        caig = iFPGA_NAMESPACE::balance_and(caig);

        caig = iFPGA_NAMESPACE::refactor(caig, ps_refactor);

        caig = iFPGA_NAMESPACE::rewrite_online(caig, ps_rewrite);

        caig = iFPGA_NAMESPACE::balance_and(caig);
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
