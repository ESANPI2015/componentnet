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
    const Hyperedges& candidateInterfaces(intersect(ResourceCost::Model::partitionFuncLeft(rcm), sw.interfacesOf(implementations(rcm))));
    // We remove all interfaces which are pure internal interfaces. These dont have to be mapped to hw interfaces
    Hyperedges internalInterfaces;
    for (const UniqueId& a : candidateInterfaces)
    {
        // Get targets of owner
        const Hyperedges& swOwnerUids(sw.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
        const Hyperedges& swTargetUids(rcm.providersOf(swOwnerUids));
        // Get connected interfaces
        const Hyperedges& swNeighbourInterfaceUids(sw.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
        // ... and their owners
        const Hyperedges& swNeighbourOwnerUids(sw.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
        // ... and the targets of the owners
        const Hyperedges& swNeighbourTargetUids(rcm.providersOf(swNeighbourOwnerUids));
        // Check that each swNeighbourTargetUid is in swTargetUids
        if (subtract(swNeighbourTargetUids, swTargetUids).size())
            continue; // at least one target is different
        std::cout << "PARTITION INTERFACES: Removing pure internal sw interface: " << a << "\n";
        internalInterfaces.push_back(a);
    }
    return subtract(candidateInterfaces, internalInterfaces);
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

    // NEW
    const Hyperedges& swOwnerUids(sw.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
    const Hyperedges& hwOwnerUids(hw.interfacesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::INVERSE));
    const Hyperedges& swTargetUids(rcm.providersOf(swOwnerUids));
    // RULE I: if owner of 'a' is not mapped at all, we fail
    if (swTargetUids.empty())
    {
        std::cout << "REACH CHECK FAILED: sw interface: " << a << " hardware interface: " << b << " OWNER NOT MAPPED\n";
        return -std::numeric_limits<float>::infinity();
    }
    // RULE II: Check if owner of 'b' is the target of owner of 'a'
    if (subtract(swTargetUids, hwOwnerUids).size()) // Some entries in swTargetUids are not in hwOwnerUids
    {
        std::cout << "REACH CHECK FAILED: sw interface: " << a << " hardware interface: " << b << " OWNER MISMATCH\n";
        return -std::numeric_limits<float>::infinity();
    }
    // NOTE: We do not have to check for internal interfaces. They have been filtered by the partition function already
    const Hyperedges& swNeighbourInterfaceUids(sw.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
    const Hyperedges& hwNeighbourInterfaceUids(hw.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
    const Hyperedges& swTargetInterfaceUids(rcm.providersOf(swNeighbourInterfaceUids));
    // RULE III: Some of the connections are external or unmapped. Check if every hw interface of connected interfaces can be reached by 'b'
    if (subtract(swTargetInterfaceUids, hwNeighbourInterfaceUids).size()) // Some x in swTargetInterfaceUids not in hwNeighbourInterfaceUids
    {
        std::cout << "REACH CHECK FAILED: sw interface: " << a << " hardware interface: " << b << " ENDPOINTS UNREACHABLE\n";
        return -std::numeric_limits<float>::infinity();
    }
    const Hyperedges& swNeighbourOwnerUids(sw.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    const Hyperedges& hwNeighbourOwnerUids(hw.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    const Hyperedges& swNeighbourTargetUids(rcm.providersOf(swNeighbourOwnerUids));
    // RULE IV: If all remote interfaces are unmapped we are unable to verify reachability.
    if (swNeighbourTargetUids.empty())
    {
        std::cout << "REACH CHECK FAILED: sw interface: " << a << " hardware interface: " << b << " ENDPOINT OWNERS UNMAPPED\n";
        return -std::numeric_limits<float>::infinity();
    }
    // RULE V: owners of the endpoints of 'a' have to be mapped to the owners of the endpoints of 'b'
    if (subtract(swNeighbourTargetUids, hwNeighbourOwnerUids).size()) // Some x in swNeighbourTargetUids not in hwNeighbourOwnerUids
    {
        std::cout << "REACH CHECK FAILED: sw interface: " << a << " hardware interface: " << b << " ENDPOINT OWNERS MISMATCH\n";
        return -std::numeric_limits<float>::infinity();
    }
    //std::cout << "REACH CHECK SUCCESSFUL sw interface: " << sw.access(a).label() << " hardware interface: " << hw.access(b).label() << " MATCH\n";
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
    const Hyperedges& swNeighbourUids(sw.interfacesOf(sw.endpointsOf(sw.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    const Hyperedges& hwNeighbourUids(unite(hw.interfacesOf(hw.endpointsOf(hw.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE), Hyperedges{b}));
    const Hyperedges& swTargetUids(rcm.providersOf(swNeighbourUids));
    // If any of the targets of sw neighbours is not in hw neighbours, we cannot reach that sw component
    if (subtract(swTargetUids, hwNeighbourUids).size()) // Some owner of an endpoint of 'a' is not in the owners of the endpoints of 'b'
    {
        // Reachability constraint failed!
        std::cout << "REACH CHECK FAILED: for consumer: " << sw.access(a).label() << " and provider: " << hw.access(b).label() << "\n";
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
    const Hyperedges& implUids(implementations(result));
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
    const Hyperedges& hwUids(processors(result));
    for (const UniqueId& hwUid : hwUids)
    {
        const Hyperedges& swUids(consumersOf(Hyperedges{hwUid}));
        const Hyperedges& availableResourceUids(resourcesOf(Hyperedges{hwUid}));
        const Hyperedges& consumedResourceUids(swUids.size() > 0 ? isPointingTo(factsOf(subrelationsOf(Hyperedges{ResourceCost::Model::ConsumesUid}), swUids)) : Hyperedges());
        for (const UniqueId& availableResourceUid : availableResourceUids)
        {
            const float available(std::stof(access(availableResourceUid).label()));
            const Hyperedges& availableResourceClassUids(instancesOf(Hyperedges{availableResourceUid}, "", FORWARD));
            // Calculate already consumed resources
            float used = 0.f;
            for (const UniqueId& consumedResourceUid : consumedResourceUids)
            {
                const Hyperedges& consumedResourceClassUids(instancesOf(Hyperedges{consumedResourceUid}, "", FORWARD));
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

