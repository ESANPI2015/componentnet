#include "SoftwareGraph.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "ResourceCostModel.hpp"
#include "HypergraphYAML.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <getopt.h>

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

static UniqueId sw2hwRelationUid = "Software::Component::MappedTo::Hardware::Component";
static UniqueId sw2hwInterfaceRelationUid = "Software::Interface::MappedTo::Hardware::Interface";

bool matchFunc (const Component::Network& rcm, const UniqueId& a, const UniqueId& b)
{
    // We trust, that a is a consumer and b is a provider
    // First we check if a is an algorithm and b is a processor
    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Graph::AlgorithmId})));
    Hyperedges hwUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Hardware::Computational::Network::ProcessorId})));
    if ((std::find(swUids.begin(), swUids.end(), a) != swUids.end())
        && (std::find(hwUids.begin(), hwUids.end(), b) != hwUids.end()))
    {
        // Lets check if all connected sw components of a are mapped to neighbours of b or not mapped at all.
        Hyperedges swNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
        hwNeighbourUids = unite(hwNeighbourUids, Hyperedges{b}); // NOTE: We can map also to the same target!
        Hyperedges hwTargetUids(rcm.to(intersect(rcm.relationsFrom(swNeighbourUids),rcm.factsOf(sw2hwRelationUid))));
        // Condition check: hwTargetUids must be a true subset of hwNeighbourUids. That means that the intersection must be equal to hwTargetUids.
        if (intersect(hwTargetUids, hwNeighbourUids).size() != hwTargetUids.size())
            return false;
        return true;
    }
    // Second, we check if a is an software interface and b is an hardware interface
    Hyperedges swInterfaceUids(rcm.interfacesOf(swUids));
    Hyperedges hwInterfaceUids(rcm.interfacesOf(hwUids));
    if ((std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
        && (std::find(hwInterfaceUids.begin(), hwInterfaceUids.end(), b) != hwInterfaceUids.end()))
    {
        // Lets check if the owner of a is mapped to the owner of b
        Hyperedges swOwnerUids(rcm.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwOwnerUids(rcm.interfacesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwTargetUids(rcm.to(intersect(rcm.relationsFrom(swOwnerUids),rcm.factsOf(sw2hwRelationUid))));
        // Condition check: hwTargetUids must be a true subset of hwOwnerUids and not empty. That means that the intersection must be equal to hwTargetUids.
        if (hwTargetUids.empty() || (intersect(hwTargetUids, hwOwnerUids).size() != hwTargetUids.size()))
        {
            return false;
        }
        // Lets check if all endpoints of a are mapped to endpoints of b or not mapped at all.
        Hyperedges swNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
        Hyperedges hwNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
        Hyperedges hwTargetInterfaceUids(rcm.to(intersect(rcm.relationsFrom(swNeighbourInterfaceUids),rcm.factsOf(sw2hwInterfaceRelationUid))));
        if (intersect(hwTargetInterfaceUids, hwNeighbourInterfaceUids).size() != hwTargetInterfaceUids.size())
        {
            return false;
        }
        // Lets check if the owners of the endpoints of a are mapped to the owners of the endpoints of b
        Hyperedges swNeighbourOwnerUids(rcm.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourOwnerUids(rcm.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourTargetUids(rcm.to(intersect(rcm.relationsFrom(swNeighbourOwnerUids),rcm.factsOf(sw2hwRelationUid))));
        // Condition check: hwNeighbourTargetUids must be a true subset of hwNeighbourOwnerUids and not empty. That means that the intersection must be equal to hwNeighbourTargetUids.
        if (hwNeighbourTargetUids.empty() || (intersect(hwNeighbourTargetUids, hwNeighbourOwnerUids).size() != hwNeighbourTargetUids.size()))
        {
            return false;
        }
        return true;
    }
    // In all other cases, we do not have a match
    return false;
}

float costFunc (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b)
{
    float minimum(std::numeric_limits<float>::infinity());
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    Hyperedges resourceCostUids(rcm.costsOf(Hyperedges{a}, Hyperedges{b}));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const std::string rLabel(rcm.read(resourceUid).label());
            const float maxR(std::stof(rLabel.substr(0,rLabel.find("|"))));
            const std::size_t lastPipePos(rLabel.rfind("|"));
            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : maxR);
            const float c(std::stof(rcm.read(resourceCostUid).label()));
            // Calculate new minimum
            minimum = std::min(minimum, (r - c) / maxR);
        }
    }
    return minimum;
}

void mapFunc (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b)
{
    ResourceCost::Model& rcm = static_cast< ResourceCost::Model& >(g);
    // I. Update all resources
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    Hyperedges resourceCostUids(rcm.costsOf(Hyperedges{a}, Hyperedges{b}));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const std::string rLabel(rcm.read(resourceUid).label());
            const std::size_t lastPipePos(rLabel.rfind("|"));
            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : std::stof(rLabel));
            const float c(std::stof(rcm.read(resourceCostUid).label()));
            // Update resources by appending it! (so we always find initial and current resources
            rcm.get(resourceUid)->updateLabel(std::to_string(r)+"|"+std::to_string(r - c));
        }
    }
    // II. Map a to b
    // a) software algorithm -> hardware processor
    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Graph::AlgorithmId})));
    if (std::find(swUids.begin(), swUids.end(), a) != swUids.end())
    {
        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, sw2hwRelationUid);
        return;
    }
    // b) software interface -> hardware interface
    Hyperedges swInterfaceUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Graph::InterfaceId})));
    if (std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
    {
        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, sw2hwInterfaceRelationUid);
        return;
    }
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
                return 1;
        }
    }

    if ((argc - optind) < 2)
    {
        usage(argv[0]);
        return 1;
    }

    // Set vars
    const std::string rcmFileName(argv[optind]);
    const std::string fileNameOut(argv[optind+1]);
    ResourceCost::Model rcm(YAML::LoadFile(rcmFileName).as<Hypergraph>());

    // Make sure that both relations exist
    rcm.relate(sw2hwRelationUid, Hyperedges{Software::Graph::AlgorithmId}, Hyperedges{Hardware::Computational::Network::ProcessorId}, "EXECUTED-ON");
    rcm.relate(sw2hwInterfaceRelationUid, Hyperedges{Software::Graph::InterfaceId}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "REACHABLE-THROUGH");

    // Print out some statistics
    Software::Graph sw(rcm);
    Hardware::Computational::Network hw(rcm);
    const unsigned int algs(sw.algorithms().size());
    const unsigned int procs(hw.processors().size());
    const unsigned int swIfs(sw.interfacesOf(sw.algorithms()).size());
    const unsigned int hwIfs(hw.interfacesOf(hw.processors()).size());
    const unsigned int nConsumers(rcm.consumers().size());
    const unsigned int nProviders(rcm.providers().size());
    if (nProviders < 1)
    {
        std::cout << "No providers found\n";
        return 2;
    }
    if (nConsumers < 1)
    {
        std::cout << "No consumers found\n";
        return 3;
    }
    if (algs < 1)
    {
        std::cout << "No algorithms found\n";
        return 4;
    }
    if (procs < 1)
    {
        std::cout << "No processors found\n";
        return 5;
    }

    std::cout << "#ALGORITHMS:\t\t" << algs << "\n";
    std::cout << "#SW INTERFACES:\t\t" << swIfs << "\n";
    std::cout << "#PROCESSORS:\t\t" << procs << "\n";
    std::cout << "#HW INTERFACES:\t\t" << hwIfs << "\n";
    std::cout << "#CONSUMERS:\t\t" << nConsumers << "\n";
    std::cout << "#PROVIDERS:\t\t" << nProviders << "\n";

    ResourceCost::Model result(rcm.map(ResourceCost::Model::partitionFunc, matchFunc, costFunc, mapFunc));

    // Print mapping results
    for (const UniqueId& swUid : sw.algorithms())
    {
        std::cout << "Algorithm " << result.read(swUid).label();
        Hyperedges hwTargetUids(result.to(intersect(result.relationsFrom(Hyperedges{swUid}),result.factsOf(sw2hwRelationUid))));
        for (const UniqueId& hwUid : hwTargetUids)
        {
            std::cout << " -> Processor " << result.read(hwUid).label();
        }
        std::cout << std::endl;
        Hyperedges swInterfaceUids(sw.interfacesOf(Hyperedges{swUid}));
        for (const UniqueId& swInterfaceUid : swInterfaceUids)
        {
            std::cout << "\tInterface " << result.read(swInterfaceUid).label();
            Hyperedges hwTargetInterfaceUids(result.to(intersect(result.relationsFrom(Hyperedges{swInterfaceUid}),result.factsOf(sw2hwInterfaceRelationUid))));
            for (const UniqueId& hwInterfaceUid : hwTargetInterfaceUids)
            {
                std::cout << " -> Interface " << result.read(hwInterfaceUid).label();
            }
            std::cout << std::endl;
        }
    }

    // Store result
    std::ofstream fout;
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(result) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}