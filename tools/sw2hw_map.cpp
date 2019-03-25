#include "SoftwareNetwork.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "ResourceCostModel.hpp"
#include "HypergraphYAML.hpp"

#include "Mapper.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <getopt.h>

/*
    This program maps a network of IMPLEMENTATION INSTANCES to a network of PROCESSOR INSTANCES
*/

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0,0,0,0}
};

void usage (const char *myName)
{
    std::cout << "Usage:\n";
    std::cout << myName << " <rcm_spec> <output>\n\n";
    std::cout << "Options:\n";
    std::cout << "--help\t" << "Show usage\n";
    std::cout << "\nExample:\n";
    std::cout << myName << " rcm_spec.yml sw2hw_mapped.yml\n";
}

int main (int argc, char **argv)
{
    std::cout << "Software to Hardware Mapper using Resource Cost Model\n";

    // Parse command line
    int c;
    while (1)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "h", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
            case '?':
                break;
            default:
                std::cout << "W00t?!\n";
                return -1;
        }
    }

    if ((argc - optind) < 2)
    {
        usage(argv[0]);
        return -1;
    }

    // Set vars
    const std::string rcmFileName(argv[optind]);
    const std::string fileNameOut(argv[optind+1]);
    Software::Hardware::Mapper mapper(YAML::LoadFile(rcmFileName).as<Hypergraph>());

    // Print out some statistics
    Software::Network sw(mapper);
    Hardware::Computational::Network hw(mapper);
    const unsigned int impls(sw.implementations().size());
    const unsigned int procs(hw.processors().size());
    const unsigned int swIfs(sw.interfacesOf(sw.implementations()).size());
    const unsigned int hwIfs(hw.interfacesOf(hw.processors()).size());
    const unsigned int nConsumers(mapper.consumers().size());
    const unsigned int nProviders(mapper.providers().size());
    if (nProviders < 1)
    {
        std::cout << "No providers found\n";
        return -2;
    }
    if (nConsumers < 1)
    {
        std::cout << "No consumers found\n";
        return -3;
    }
    if (impls < 1)
    {
        std::cout << "No implementations found\n";
        return -4;
    }
    if (procs < 1)
    {
        std::cout << "No processors found\n";
        return -5;
    }

    std::cout << "#IMPLEMENTATIONS:\t\t" << impls << "\n";
    std::cout << "#SW INTERFACES:\t\t" << swIfs << "\n";
    std::cout << "#PROCESSORS:\t\t" << procs << "\n";
    std::cout << "#HW INTERFACES:\t\t" << hwIfs << "\n";
    std::cout << "#CONSUMERS:\t\t" << nConsumers << "\n";
    std::cout << "#PROVIDERS:\t\t" << nProviders << "\n";

    float globalCosts = mapper.map();

    // Print mapping results & sum up remaining resources/max resources per target (or multiply it?)
    for (const UniqueId& swUid : sw.implementations())
    {
        std::cout << "Implementation " << mapper.access(swUid).label();
        Hyperedges hwUids(mapper.providersOf(Hyperedges{swUid}));
        for (const UniqueId& hwUid : hwUids)
        {
            std::cout << " -> Processor " << mapper.access(hwUid).label();
        }
        std::cout << "\n";
        // TODO: Print interface mapping
    }
    std::cout << "Global Normalized Costs: " << std::to_string(globalCosts) << "\n";

    // Store result
    std::ofstream fout;
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(mapper) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return static_cast<int>(globalCosts*100.f);
}
