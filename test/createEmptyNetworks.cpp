#include "ResourceCostModel.hpp"
#include "SoftwareGraph.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"

#include <fstream>
#include <iostream>
#include <limits>

int main (void)
{
    std::cout << "Create empty networks\n";

    std::ofstream fout;
    Software::Graph sw;
    fout.open("empty_sw.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(sw) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    Hardware::Computational::Network hw;
    fout.open("empty_hw.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(hw) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    ResourceCost::Model rcm;
    fout.open("empty_rcm.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(rcm) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}
