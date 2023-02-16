/**
 * @file rewrite.cpp
 * @author liwei ni (nilw@pcl.ac.cn)
 * @brief  test for rewrite
 * @version 0.1
 * @date 2022-10-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "io/reader.hpp"
#include "io/detail/write_verilog.hpp"
#include "optimization/rewrite.hpp"
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
     *  ./bin/rewrite N <path/XXX.aig>
     *  eg: ./bin/rewrite 2 ../benchmark/adder.aig
     */
    assert(argc > 1);
    uint iteration = std::atoi(argv[1]);
    std::string file = std::string(argv[2]);

    iFPGA_NAMESPACE::write_verilog_params ports;
    iFPGA_NAMESPACE::aig_network aig;
    iFPGA_NAMESPACE::Reader reader(file, aig, ports);

    iFPGA_NAMESPACE::tic_toc t;
    iFPGA_NAMESPACE::aig_network raig(aig);
    iFPGA_NAMESPACE::rewrite_params ps;
    for(uint i = 0u; i < iteration; ++i ){
        raig = iFPGA_NAMESPACE::rewrite_online(aig, ps);
    }
    printf("Statics:\n");
    printf("time                : %0.6f", t.toc());
    printf("area(before/after)  : %u/%u", aig.num_gates(), raig.num_gates());
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> aig_depth(aig);
    iFPGA_NAMESPACE::depth_view<iFPGA_NAMESPACE::aig_network> raig_depth(raig);
    printf("level(before/after) : %u/%u", aig_depth.depth(), raig_depth.depth());
    printf("peak memory         : %ld bytes\n", getPeakRSS());

    if(argc > 3)
    {
        std::ostringstream out;
        write_verilog(raig, out, ports);
        std::string ofile(argv[3]);
        std::ofstream fout(ofile, std::ios::out);
        fout << out.str();
        fout.close();
    }

    return 0;
}
