#include "SoftwareNetwork.hpp"
#include "HypergraphYAML.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <getopt.h>

/*
    This program generates networks of IMPLEMENTATION instances given network(s) of ALGORITHM instances.
    It should be used to find the possible networks for mapping to compatible PROCESSOR instances later.
*/

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0,0,0,0}
};

void usage (const char *myName)
{
    std::cout << "Usage:\n";
    std::cout << myName << " <sw_spec> <output_prefix>\n\n";
    std::cout << "Options:\n";
    std::cout << "--help\t" << "Show usage\n";
    std::cout << "\nExample:\n";
    std::cout << myName << " algorithm_net.yml implementation_net\n";
    std::cout << "The prefix implementation_net will produce as many implementation_netX.yml files as there are possibilities\n";
}

int main (int argc, char **argv)
{
    std::cout << "Implementation network generator from algorithm network\n";

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
    const std::string fileNameIn(argv[optind]);
    const std::string fileNameOutPrefix(argv[optind+1]);
    Software::Network sw(YAML::LoadFile(fileNameIn).as<Hypergraph>());

    std::cout << "Searching for possible implementation nets ...\n";
    std::vector< Software::Network > results(sw.generateAllImplementationNetworks());

    // Now we have a list of all possible implementation graphs which can be build from algorithm graphs
    std::cout << "Found " << results.size() << " possible networks.\n";

    std::cout << "Storing results\n";
    std::ofstream fout;
    int i = 0;
    for (const Software::Network& current : results)
    {
        fout.open(fileNameOutPrefix+std::to_string(i)+".yml");
        if(fout.good()) {
            fout << YAML::StringFrom(current) << std::endl;
        } else {
            std::cout << "FAILED\n";
        }
        fout.close();
        i++;
    }

    return results.size();
}
