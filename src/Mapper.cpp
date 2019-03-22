#include "Mapper.hpp"

namespace Software
{
namespace Hardware
{

const UniqueId Mapper::executedOnUid="Software::Hardware::Mapper::ExecutedOn";
const UniqueId Mapper::reachableViaUid="Software::Hardware::Mapper::ReachableVia";

Mapper::Mapper(const ResourceCost::Model& rcm,
       const Software::Network& sw,
       const ::Hardware::Computational::Network& hw
      )
{
    importFrom(rcm);
    importFrom(sw);
    importFrom(hw);
    
    // Make sure that both relations exist
    subrelationFrom(executedOnUid, Hyperedges{Software::Network::ImplementationId}, Hyperedges{::Hardware::Computational::Network::ProcessorId}, ResourceCost::Model::MappedToUid);
    subrelationFrom(reachableViaUid, Hyperedges{Software::Network::InterfaceId}, Hyperedges{::Hardware::Computational::Network::InterfaceId}, ResourceCost::Model::MappedToUid);
    access(executedOnUid).updateLabel("EXECUTED-ON");
    access(reachableViaUid).updateLabel("REACHABLE-VIA");
}

Hyperedges Mapper::partitionFuncLeft (const ResourceCost::Model& rcm)
{
    const Software::Network& sw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncLeft(rcm), sw.implementations());
}

Hyperedges Mapper::partitionFuncRight (const ResourceCost::Model& rcm)
{
    const ::Hardware::Computational::Network& hw(static_cast<const ResourceCost::Model&>(rcm));
    return intersect(ResourceCost::Model::partitionFuncRight(rcm), hw.processors());
}

float Mapper::matchFunc (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b)
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
        return -std::numeric_limits<float>::infinity();
    }
    return costs;


    // OLD BELOW
    // We trust, that a is a consumer and b is a provider
    // First we check if a is an implementation and b is a processor
    //Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    //Hyperedges hwUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{::Hardware::Computational::Network::ProcessorId})));
    //if ((std::find(swUids.begin(), swUids.end(), a) != swUids.end())
    //    && (std::find(hwUids.begin(), hwUids.end(), b) != hwUids.end()))
    //{
    //    // Lets check if all connected sw components of a are mapped to neighbours of b or not mapped at all.
    //    Hyperedges swNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    //    Hyperedges hwNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    //    hwNeighbourUids = unite(hwNeighbourUids, Hyperedges{b}); // NOTE: We can map also to the same target!
    //    Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourUids),rcm.factsOf(executedOnUid))));
    //    // Condition check: hwTargetUids must be a true subset of hwNeighbourUids. That means that the intersection must be equal to hwTargetUids.
    //    if (intersect(hwTargetUids, hwNeighbourUids).size() != hwTargetUids.size())
    //        return false;
    //    return true;
    //}
    //// Second, we check if a is an software interface and b is an hardware interface
    //Hyperedges swInterfaceUids(rcm.interfacesOf(swUids));
    //Hyperedges hwInterfaceUids(rcm.interfacesOf(hwUids));
    //if ((std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
    //    && (std::find(hwInterfaceUids.begin(), hwInterfaceUids.end(), b) != hwInterfaceUids.end()))
    //{
    //    // Lets check if the owner of a is mapped to the owner of b
    //    Hyperedges swOwnerUids(rcm.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
    //    Hyperedges hwOwnerUids(rcm.interfacesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::INVERSE));
    //    Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swOwnerUids),rcm.factsOf(executedOnUid))));
    //    // Condition check: hwTargetUids must be a true subset of hwOwnerUids and not empty. That means that the intersection must be equal to hwTargetUids.
    //    if (hwTargetUids.empty() || (intersect(hwTargetUids, hwOwnerUids).size() != hwTargetUids.size()))
    //    {
    //        return false;
    //    }
    //    // Lets check if all endpoints of a are mapped to endpoints of b or not mapped at all.
    //    Hyperedges swNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
    //    Hyperedges hwNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
    //    Hyperedges hwTargetInterfaceUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourInterfaceUids),rcm.factsOf(reachableViaUid))));
    //    if (intersect(hwTargetInterfaceUids, hwNeighbourInterfaceUids).size() != hwTargetInterfaceUids.size())
    //    {
    //        return false;
    //    }
    //    // Lets check if the owners of the endpoints of a are mapped to the owners of the endpoints of b
    //    Hyperedges swNeighbourOwnerUids(rcm.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    //    Hyperedges hwNeighbourOwnerUids(rcm.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    //    Hyperedges hwNeighbourTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourOwnerUids),rcm.factsOf(executedOnUid))));
    //    // Condition check: hwNeighbourTargetUids must be a true subset of hwNeighbourOwnerUids and not empty. That means that the intersection must be equal to hwNeighbourTargetUids.
    //    if (hwNeighbourTargetUids.empty() || (intersect(hwNeighbourTargetUids, hwNeighbourOwnerUids).size() != hwNeighbourTargetUids.size()))
    //    {
    //        return false;
    //    }
    //    return true;
    //}
    //// In all other cases, we do not have a match
    //return false;
}

void Mapper::mapFunc (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b)
{
    ResourceCost::Model& rcm(static_cast< ResourceCost::Model& >(g));
    // Map sw implementation a to processor b
    rcm.factFrom(Hyperedges{a}, Hyperedges{b}, Mapper::executedOnUid);
}

/* Uses the implemented functions to map software implementations to hardware processors */
float Mapper::map()
{
    // Perform mapping and import results
    const ResourceCost::Model result(ResourceCost::Model::map(partitionFuncLeft, partitionFuncRight, matchFunc, mapFunc));
    importFrom(result);

    // Calculate global costs
    float globalCosts(0.f);
    Hyperedges hwUids(partitionFuncRight(result));
    for (const UniqueId& hwUid : hwUids)
    {
        Hyperedges swUids(consumersOf(Hyperedges{hwUid}));
        Hyperedges availableResourceUids(resourcesOf(Hyperedges{hwUid}));
        Hyperedges consumedResourceUids(isPointingTo(factsOf(subrelationsOf(Hyperedges{ResourceCost::Model::ConsumesUid}), swUids)));
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

}
}

