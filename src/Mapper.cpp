#include "Mapper.hpp"

namespace Software
{
namespace Hardware
{

const UniqueId Mapper::ExecutedOnUid="Software::Hardware::Mapper::ExecutedOn";
const UniqueId Mapper::ReachableViaUid="Software::Hardware::Mapper::ReachableVia";

Mapper::Mapper(const ResourceCost::Model& rcm,
       const Software::Network& sw,
       const ::Hardware::Computational::Network& hw
      )
{
    importFrom(rcm);
    importFrom(sw);
    importFrom(hw);
    
    // Make sure that both relations exist
    subrelationFrom(ExecutedOnUid, Hyperedges{Software::Network::ImplementationId}, Hyperedges{::Hardware::Computational::Network::ProcessorId}, ResourceCost::Model::MappedToUid);
    subrelationFrom(ReachableViaUid, Hyperedges{Software::Network::InterfaceId}, Hyperedges{::Hardware::Computational::Network::InterfaceId}, ResourceCost::Model::MappedToUid);
    access(ExecutedOnUid).updateLabel("EXECUTED-ON");
    access(ReachableViaUid).updateLabel("REACHABLE-VIA");
}

Hyperedges Mapper::implementations (const ResourceCost::Model& rcm)
{
    const Software::Network& sw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncLeft(rcm), sw.implementations());
}

Hyperedges Mapper::processors (const ResourceCost::Model& rcm)
{
    const ::Hardware::Computational::Network& hw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncRight(rcm), hw.processors());
}

Hyperedges Mapper::swInterfaces (const ResourceCost::Model& rcm)
{
    const Software::Network& sw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncLeft(rcm), sw.interfacesOf(implementations(rcm)));
}

Hyperedges Mapper::hwInterfaces (const ResourceCost::Model& rcm)
{
    const ::Hardware::Computational::Network& hw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncRight(rcm), hw.interfacesOf(processors(rcm)));
}

float Mapper::matchSwToHwInterface (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b)
{
    // We have two things to check:
    // a) resource constraints (ResourceCost::Model::matchFunc)
    const float costs(ResourceCost::Model::matchFunc(rcm, a, b));
    if (costs < 0.f)
        return costs;

    const Software::Network& sw(static_cast<const ResourceCost::Model&>(rcm));
    const ::Hardware::Computational::Network& hw(static_cast<const ResourceCost::Model&>(rcm));
    // b) reachability constraints
    // Lets check if the owner of a is mapped to the owner of b
    Hyperedges swOwnerUids(sw.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
    Hyperedges hwOwnerUids(hw.interfacesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::INVERSE));
    Hyperedges swTargetUids(rcm.providersOf(swOwnerUids));
    // Condition check: swTargetUids must be a true subset of hwOwnerUids and not empty. That means that the intersection must be equal to hwTargetUids.
    // Why: The owner of an interface has to be mapped first, and its target has to match the owner of the target interface
    if (swTargetUids.empty() || (intersect(swTargetUids, hwOwnerUids).size() != swTargetUids.size()))
    {
        std::cout << "REACH CHECK FAILED sw interface: " << sw.access(a).label() << " hardware interface: " << hw.access(b).label() << " OWNER MISMATCH\n";
        return -std::numeric_limits<float>::infinity();
    }
    // Lets check if all endpoints of a are mapped to endpoints of b or not mapped at all.
    Hyperedges swNeighbourInterfaceUids(sw.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
    Hyperedges hwNeighbourInterfaceUids(hw.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
    // NOTE: We can be mapped to the same interface!
    hwNeighbourInterfaceUids = unite(hwNeighbourInterfaceUids, Hyperedges{b});
    Hyperedges swTargetInterfaceUids(rcm.providersOf(swNeighbourInterfaceUids));
    if (intersect(swTargetInterfaceUids, hwNeighbourInterfaceUids).size() != swTargetInterfaceUids.size())
    {
        std::cout << "REACH CHECK FAILED sw interface: " << sw.access(a).label() << " hardware interface: " << hw.access(b).label() << " ENDPOINTS UNREACHABLE\n";
        return -std::numeric_limits<float>::infinity();
    }
    // Lets check if the owners of the endpoints of a are mapped to the owners of the endpoints of b
    Hyperedges swNeighbourOwnerUids(sw.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    Hyperedges hwNeighbourOwnerUids(hw.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    Hyperedges swNeighbourTargetUids(rcm.providersOf(swNeighbourOwnerUids));
    // Condition check: hwNeighbourTargetUids must be a true subset of hwNeighbourOwnerUids and not empty. That means that the intersection must be equal to hwNeighbourTargetUids.
    if (swNeighbourTargetUids.empty() || (intersect(swNeighbourTargetUids, hwNeighbourOwnerUids).size() != swNeighbourTargetUids.size()))
    {
        std::cout << "REACH CHECK FAILED sw interface: " << sw.access(a).label() << " hardware interface: " << hw.access(b).label() << " ENDPOINT OWNER MISMATCH\n";
        return -std::numeric_limits<float>::infinity();
    }
    return costs;
}

float Mapper::matchImplementationAndProcessor (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b)
{
    // We have two things to check:
    // a) resource constraints (ResourceCost::Model::matchFunc)
    const float costs(ResourceCost::Model::matchFunc(rcm, a, b));
    if (costs < 0.f)
        return costs;

    const Software::Network& sw(static_cast<const ResourceCost::Model&>(rcm));
    const ::Hardware::Computational::Network& hw(static_cast<const ResourceCost::Model&>(rcm));
    // b) reachability constraints
    // Lets check if all neighbours of a are mapped to neighbours of b or not mapped at all
    Hyperedges swNeighbourUids(sw.interfacesOf(sw.endpointsOf(sw.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    Hyperedges hwNeighbourUids(hw.interfacesOf(hw.endpointsOf(hw.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    hwNeighbourUids = unite(hwNeighbourUids, Hyperedges{b}); // NOTE: We can map also to the same target!
    Hyperedges swTargetUids(rcm.providersOf(swNeighbourUids));
    // If any of the targets of sw neighbours is not in hw neighbours, we cannot reach that sw component
    if (intersect(hwNeighbourUids, swTargetUids).size() != swTargetUids.size())
    {
        // Reachability constraint failed!
        std::cout << "REACH CHECK FAILED for consumer: " << sw.access(a).label() << " and provider: " << hw.access(b).label() << "\n";
        return -std::numeric_limits<float>::infinity();
    }
    return costs;
}

void Mapper::mapSwToHwInterface (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b)
{
    ResourceCost::Model& rcm(static_cast< ResourceCost::Model& >(g));
    // Map sw interface a to hw interface b
    rcm.factFrom(Hyperedges{a}, Hyperedges{b}, Mapper::ReachableViaUid);
}

void Mapper::mapImplementationToProcessor (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b)
{
    ResourceCost::Model& rcm(static_cast< ResourceCost::Model& >(g));
    // Map sw implementation a to processor b
    rcm.factFrom(Hyperedges{a}, Hyperedges{b}, Mapper::ExecutedOnUid);
}

/* Uses the implemented functions to map software implementations to hardware processors */
float Mapper::mapAllImplementationsToProcessors()
{
    // Perform mapping and import results
    const ResourceCost::Model result(ResourceCost::Model::map(implementations, processors, matchImplementationAndProcessor, mapImplementationToProcessor));
    importFrom(result);

    // Check if every sw implementation could be mapped
    Hyperedges implUids(implementations(result));
    for (const UniqueId& implUid : implUids)
    {
        if (!providersOf(Hyperedges{implUid}).size())
        {
            std::cout << "COMPLETENESS CHECK FAILED: " << access(implUid).label() << " has no provider\n";
            return -std::numeric_limits<float>::infinity();
        }
    }

    // Calculate global costs
    float globalCosts(0.f);
    Hyperedges hwUids(processors(result));
    for (const UniqueId& hwUid : hwUids)
    {
        Hyperedges swUids(consumersOf(Hyperedges{hwUid}));
        Hyperedges availableResourceUids(resourcesOf(Hyperedges{hwUid}));
        Hyperedges consumedResourceUids;
        if (swUids.size())
            consumedResourceUids = isPointingTo(factsOf(subrelationsOf(Hyperedges{ResourceCost::Model::ConsumesUid}), swUids));
        for (const UniqueId& availableResourceUid : availableResourceUids)
        {
            const float available(std::stof(access(availableResourceUid).label()));
            Hyperedges availableResourceClassUids(instancesOf(Hyperedges{availableResourceUid}, "", FORWARD));
            // Calculate already consumed resources
            float used = 0.f;
            for (const UniqueId& consumedResourceUid : consumedResourceUids)
            {
                Hyperedges consumedResourceClassUids(instancesOf(Hyperedges{consumedResourceUid}, "", FORWARD));
                // If types mismatch, continue
                if (intersect(availableResourceClassUids, consumedResourceClassUids).empty())
                    continue;
                // Update usage
                used += std::stof(access(consumedResourceUid).label());
            }
            globalCosts += (available - used) / available;
        }
    }
    // NOTE: Greater means better :)
    return (hwUids.size() > 0 ? globalCosts / hwUids.size() : 0.f);
}

float Mapper::mapAllSwAndHwInterfaces()
{
    // Perform mapping and import results
    const ResourceCost::Model result(ResourceCost::Model::map(swInterfaces, hwInterfaces, matchSwToHwInterface, mapSwToHwInterface));
    importFrom(result);

    return 0.f;
}

float Mapper::map()
{
    const float globalCostsA(mapAllImplementationsToProcessors());
    if (globalCostsA < 0.f)
        return globalCostsA;
    const float globalCostsB(mapAllSwAndHwInterfaces());
    if (globalCostsB < 0.f);
        return globalCostsB;
    return (globalCostsA + globalCostsB) / 2.f;
}

}
}

