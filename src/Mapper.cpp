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
    relate(executedOnUid, Hyperedges{Software::Network::ImplementationId}, Hyperedges{::Hardware::Computational::Network::ProcessorId}, "EXECUTED-ON");
    relate(reachableViaUid, Hyperedges{Software::Network::InterfaceId}, Hyperedges{::Hardware::Computational::Network::InterfaceId}, "REACHABLE-VIA");
}

Hyperedges Mapper::partitionFuncLeft (const ResourceCost::Model& rcm)
{
    return ResourceCost::Model::partitionFuncLeft(rcm);
}

Hyperedges Mapper::partitionFuncRight (const ResourceCost::Model& rcm)
{
    return ResourceCost::Model::partitionFuncRight(rcm);
}

bool Mapper::matchFunc (const Component::Network& rcm, const UniqueId& a, const UniqueId& b)
{
    // We trust, that a is a consumer and b is a provider
    // First we check if a is an implementation and b is a processor
    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    Hyperedges hwUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{::Hardware::Computational::Network::ProcessorId})));
    if ((std::find(swUids.begin(), swUids.end(), a) != swUids.end())
        && (std::find(hwUids.begin(), hwUids.end(), b) != hwUids.end()))
    {
        // Lets check if all connected sw components of a are mapped to neighbours of b or not mapped at all.
        Hyperedges swNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
        hwNeighbourUids = unite(hwNeighbourUids, Hyperedges{b}); // NOTE: We can map also to the same target!
        Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourUids),rcm.factsOf(executedOnUid))));
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
        Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swOwnerUids),rcm.factsOf(executedOnUid))));
        // Condition check: hwTargetUids must be a true subset of hwOwnerUids and not empty. That means that the intersection must be equal to hwTargetUids.
        if (hwTargetUids.empty() || (intersect(hwTargetUids, hwOwnerUids).size() != hwTargetUids.size()))
        {
            return false;
        }
        // Lets check if all endpoints of a are mapped to endpoints of b or not mapped at all.
        Hyperedges swNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
        Hyperedges hwNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
        Hyperedges hwTargetInterfaceUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourInterfaceUids),rcm.factsOf(reachableViaUid))));
        if (intersect(hwTargetInterfaceUids, hwNeighbourInterfaceUids).size() != hwTargetInterfaceUids.size())
        {
            return false;
        }
        // Lets check if the owners of the endpoints of a are mapped to the owners of the endpoints of b
        Hyperedges swNeighbourOwnerUids(rcm.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourOwnerUids(rcm.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
        Hyperedges hwNeighbourTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourOwnerUids),rcm.factsOf(executedOnUid))));
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

float Mapper::costFunc (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b)
{
    float minimum(std::numeric_limits<float>::infinity());
    // For finding the resources we can query b ...
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    // ... but for the costs we have to query the classes of a and b
    Hyperedges consumerClassUids(rcm.instancesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::FORWARD));
    Hyperedges providerClassUids(rcm.instancesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::FORWARD));
    Hyperedges resourceCostUids(rcm.costsOf(consumerClassUids, providerClassUids));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const std::string rLabel(rcm.access(resourceUid).label());
            const float maxR(std::stof(rLabel.substr(0,rLabel.find("|"))));
            const std::size_t lastPipePos(rLabel.rfind("|"));
            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : maxR);
            const float c(std::stof(rcm.access(resourceCostUid).label()));
            // Calculate new minimum
            minimum = std::min(minimum, (r - c) / maxR);
        }
    }
    return minimum;
}

void Mapper::mapFunc (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b)
{
    ResourceCost::Model& rcm = static_cast< ResourceCost::Model& >(g);
    // I. Update all resources
    // For finding the resources we can query b ...
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    // ... but for the costs we have to query the classes of a and b
    Hyperedges consumerClassUids(rcm.instancesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::FORWARD));
    Hyperedges providerClassUids(rcm.instancesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::FORWARD));
    Hyperedges resourceCostUids(rcm.costsOf(consumerClassUids, providerClassUids));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const std::string rLabel(rcm.access(resourceUid).label());
            const float maxR(std::stof(rLabel.substr(0,rLabel.find("|"))));
            const std::size_t lastPipePos(rLabel.rfind("|"));
            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : maxR);
            const float c(std::stof(rcm.access(resourceCostUid).label()));
            // Update resources by appending it! (so we always find initial and current resources
            rcm.access(resourceUid).updateLabel(std::to_string(maxR)+"|"+std::to_string(r - c));
        }
    }
    // II. Map a to b
    // a) software implementation -> hardware processor
    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    if (std::find(swUids.begin(), swUids.end(), a) != swUids.end())
    {
        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, executedOnUid);
        return;
    }
    // b) software interface -> hardware interface
    Hyperedges swInterfaceUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::InterfaceId})));
    if (std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
    {
        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, reachableViaUid);
        return;
    }
}

/* Uses the implemented functions to map software implementations to hardware processors */
float Mapper::map()
{
    // Perform mapping and import results
    ResourceCost::Model result(ResourceCost::Model::map(partitionFuncLeft, partitionFuncRight, matchFunc, costFunc, mapFunc));
    importFrom(result);

    // Calculate global costs
    float globalCosts = 0.f;
    Hyperedges swUids(instancesOf(subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    for (const UniqueId& swUid : swUids)
    {
        Hyperedges hwTargetUids(isPointingTo(intersect(relationsFrom(Hyperedges{swUid}),factsOf(executedOnUid))));
        for (const UniqueId& hwUid : hwTargetUids)
        {
            Hyperedges resourceUids(resourcesOf(Hyperedges{hwUid}));
            for (const UniqueId& resourceUid : resourceUids)
            {
                const std::string rLabel(access(resourceUid).label());
                const float maxR(std::stof(rLabel.substr(0,rLabel.find("|"))));
                const std::size_t lastPipePos(rLabel.rfind("|"));
                const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : maxR);
                globalCosts += r / maxR;
            }
        }
        //Hyperedges swInterfaceUids(interfacesOf(Hyperedges{swUid})); // FIXME: I do not have interfacesOf here!
        //for (const UniqueId& swInterfaceUid : swInterfaceUids)
        //{
        //    Hyperedges hwTargetInterfaceUids(isPointingTo(intersect(relationsFrom(Hyperedges{swInterfaceUid}),factsOf(reachableViaUid))));
        //    for (const UniqueId& hwInterfaceUid : hwTargetInterfaceUids)
        //    {
        //        // TODO: Do we need to calculate costs here?
        //    }
        //}
    }
    return globalCosts;
}

}
}

