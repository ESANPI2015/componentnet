#include "SoftwareGraph.hpp"
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
    std::cout << myName << " <sw_spec> <output>\n\n";
    std::cout << "Options:\n";
    std::cout << "--help\t" << "Show usage\n";
    std::cout << "\nExample:\n";
    std::cout << myName << " algorithm_net.yml implementation_net.yml\n";
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
                return 1;
        }
    }

    if ((argc - optind) < 2)
    {
        usage(argv[0]);
        return 1;
    }

    // Set vars
    const std::string fileNameIn(argv[optind]);
    const std::string fileNameOut(argv[optind+1]);
    Software::Graph sw(YAML::LoadFile(fileNameIn).as<Hypergraph>());

    // In order to find our implementations later on in different results, we define a new relation
    const UniqueId realizedByUid("Software::Graph::RealizedBy");
    sw.relate(realizedByUid, Hyperedges{Software::Graph::AlgorithmId}, Hyperedges{Software::Graph::ImplementationId}, "REALIZED-BY");

    std::cout << "Searching for possible implementation nets ...\n";
    std::vector< Software::Graph > results;
    results.push_back(sw);


    // Cycle through all algorithm instances
    Hyperedges algUids(sw.algorithms());
    for (const UniqueId& algUid : algUids)
    {
        std::cout << "Processing [" << algUid << "] aka " << sw.read(algUid).label() << " ";
        // Find all implementation classes
        Hyperedges algClassUids(sw.instancesOf(Hyperedges{algUid},"", Hypergraph::TraversalDirection::FORWARD));
        Hyperedges implClassUids(sw.directSubclassesOf(algClassUids));

        // For each possible implementation (except of the first one) we have a new possibility
        std::vector< Software::Graph > newResults;
        for (Software::Graph& current : results)
        {
            bool first = true;
            for (const UniqueId& implClassUid : implClassUids)
            {
                // instantiate
                if (first)
                {
                    current.factFrom(Hyperedges{algUid}, current.instantiateComponent(Hyperedges{implClassUid}, current.read(algUid).label()), realizedByUid);
                } else {
                    // create a new possiblity if needed
                    Software::Graph newResult(current);
                    newResult.factFrom(Hyperedges{algUid}, newResult.instantiateComponent(Hyperedges{implClassUid}, newResult.read(algUid).label()), realizedByUid);
                    newResults.push_back(newResult);
                }
                std::cout << "o";
                first = false; 
            }
        }
        results.insert(results.end(), newResults.begin(), newResults.end());
        std::cout << "\n";
    }

    // Now we have a list of all possible implementation graphs which can be build from algorithm graphs
    std::cout << "Found " << results.size() << " possible networks.\n";

    // TODO: Until here, the code has been tested :)

    // Wire new instance(s) to already created instances iff corresponding algorithm instances are also wired
    // TODO: Do we have to find originalInterfaces?
    std::cout << "Rewiring ...\n";
    for (const UniqueId& algUid : algUids)
    {
        // Find interfaces
        Hyperedges algInterfaceUids(sw.interfacesOf(Hyperedges{algUid}));
        for (const UniqueId& algInterfaceUid : algInterfaceUids)
        {
            // Find other interfaces
            Hyperedges endpointUids(sw.endpointsOf(Hyperedges{algInterfaceUid}));
            //Hyperedges otherAlgInterfaceClassUids(sw.instancesOf(algClassUids,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& otherAlgInterfaceUid : endpointUids)
            {
                // Find other algorithms
                Hyperedges otherAlgUids(intersect(algUids, sw.interfacesOf(Hyperedges{otherAlgInterfaceUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                for (const UniqueId& otherAlgUid : otherAlgUids)
                {
                    std::cout << "Processing [" << algUid << "] -> [" << otherAlgUid << "]\n";
                    // We now have algUid -> algInterfaceUid -> otherAlgInterfaceUid -> otherAlgUid
                    // We have to find implUid -> implInterfaceUid -> otherImpleInterfaceUid -> otherImplUid in ALL results
                    for (Software::Graph& current : results)
                    {
                        // NOTE: For each result, we have different instances (at most one though)
                        // To find them, we need to get all facts of realizedByUid, which also point from algUid or otherAlgUid
                        Hyperedges implUids(current.to(current.factsOf(realizedByUid, "", Hypergraph::TraversalDirection::INVERSE, Hyperedges{algUid})));
                        Hyperedges otherImplUids(current.to(current.factsOf(realizedByUid, "", Hypergraph::TraversalDirection::INVERSE, Hyperedges{otherAlgUid})));
                        std::cout << "Matches implementation " << implUids << " -> " << otherImplUids << "\n";
                        // Find the correct interfaces ... by ownership & name
                        Hyperedges implInterfaceUids(current.interfacesOf(implUids, sw.read(algInterfaceUid).label()));
                        Hyperedges otherImplInterfaceUids(current.interfacesOf(otherImplUids, sw.read(otherAlgInterfaceUid).label()));
                        std::cout << "Matches interfaces " << implInterfaceUids << " -> " << otherImplInterfaceUids << "\n";
                        // Wire
                        // NOTE: implInterfaceUids are inputs, otherImpleInterfaceUids are outputs
                        current.dependsOn(implInterfaceUids, otherImplInterfaceUids);
                    }
                }
            }
        }
    }

    std::cout << "Storing results\n";
    std::ofstream fout;
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(results[0]) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();


    return 0;
}
