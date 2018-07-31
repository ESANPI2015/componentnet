#include "ComponentNetwork.hpp"

namespace Component {

const UniqueId Network::ComponentId            = "Component::Network::Component";
const UniqueId Network::InterfaceId            = "Component::Network::Interface";
const UniqueId Network::NetworkId              = "Component::Network::Network";
const UniqueId Network::HasAInterfaceId        = "Component::Network::HasAInterface";
const UniqueId Network::ConnectedToInterfaceId = "Component::Network::ConnectedToInterface";
const UniqueId Network::PartOfNetworkId        = "Component::Network::PartOfNetwork";

        // Constructor/Destructor
Network::Network()
{
    createMainConcepts();
}

Network::Network(const Hypergraph& A)
: CommonConceptGraph(A)
{
    createMainConcepts();
}
Network::~Network()
{
}

void Network::createMainConcepts()
{
    create(Network::ComponentId, "COMPONENT");
    create(Network::InterfaceId, "INTERFACE");
    create(Network::NetworkId, "NETWORK");

    subrelationFrom(Network::HasAInterfaceId, Hyperedges{Network::ComponentId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::HasAId);
    subrelationFrom(Network::ConnectedToInterfaceId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::ConnectsId);
    subrelationFrom(Network::PartOfNetworkId, Hyperedges{Network::ComponentId}, Hyperedges{Network::NetworkId}, CommonConceptGraph::PartOfId);

    isA(Hyperedges{Network::NetworkId},Hyperedges{Network::ComponentId});
}

Hyperedges Network::createComponent(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if(!isA(create(uid, name), intersect(unite(Hyperedges{Network::ComponentId}, suids), componentClasses())).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Network::createInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if (!isA(create(uid, name), intersect(unite(Hyperedges{Network::InterfaceId}, suids), interfaceClasses())).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Network::createNetwork(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if (!isA(create(uid, name), intersect(unite(Hyperedges{Network::NetworkId}, suids), networkClasses())).empty())
        return Hyperedges{uid};
    return Hyperedges();
}
Hyperedges Network::componentClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(subclassesOf(Hyperedges{Network::ComponentId}, name));
    if (!suids.empty())
        all = intersect(all, subclassesOf(suids, name));
    return all;
}
Hyperedges Network::interfaceClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(subclassesOf(Hyperedges{Network::InterfaceId}, name));
    if (!suids.empty())
        all = intersect(all, subclassesOf(suids, name));
    return all;
}
Hyperedges Network::networkClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(subclassesOf(Hyperedges{Network::NetworkId}, name));
    if (!suids.empty())
        all = intersect(all, subclassesOf(suids, name));
    return all;
}

Hyperedges Network::instantiateComponent(const Hyperedges& componentIds, const std::string& newName)
{
    return instantiateDeepFrom(componentIds, newName);
}

Hyperedges Network::instantiateComponent(const Hyperedges& componentIds)
{
    return instantiateDeepFrom(componentIds);
}

Hyperedges Network::instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name)
{
    Hyperedges result;
    for (const UniqueId& componentId : componentIds)
    {
        Hyperedges newIfs(instantiateDeepFrom(interfaceClassIds, name));
        hasInterface(Hyperedges{componentId}, newIfs);
        result = unite(result, newIfs);
    }
    return result;
}

Hyperedges Network::components(const std::string& name, const std::string& className)
{
    // Get all super classes
    Hyperedges classIds = componentClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Network::interfaces(const std::string& name, const std::string& className)
{
    // Get all super classes
    Hyperedges classIds = interfaceClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Network::networks(const std::string& name, const std::string& className)
{
    // Get all super classes
    Hyperedges classIds = networkClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Network::hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds)
{
    Hyperedges result;
    Hyperedges fromIds(intersect(componentIds, unite(componentClasses(), components())));
    Hyperedges toIds(intersect(interfaceIds, interfaces()));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::HasAInterfaceId));
        }
    }
    return result;
}
Hyperedges Network::connectInterface(const Hyperedges& fromInterfaceIds, const Hyperedges& toInterfaceIds)
{
    Hyperedges result;
    Hyperedges valid(interfaces());
    Hyperedges fromIds(intersect(fromInterfaceIds, valid));
    Hyperedges toIds(intersect(toInterfaceIds, valid));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::ConnectedToInterfaceId));
        }
    }
    return result;
}

Hyperedges Network::partOfNetwork(const Hyperedges& componentIds, const Hyperedges& networkIds)
{
    Hyperedges result;
    Hyperedges fromIds(intersect(componentIds, components()));
    Hyperedges toIds(intersect(networkIds, unite(networks(), networkClasses())));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::PartOfNetworkId));
        }
    }
    return result;
}

Hyperedges Network::interfacesOf(const Hyperedges& componentIds, const std::string& name, const TraversalDirection dir)
{
    Hyperedges children(childrenOf(componentIds,name,dir));
    if (dir == TraversalDirection::FORWARD)
        return intersect(children, interfaces(name));
    else
        return intersect(children, unite(components(name), componentClasses(name)));
}

}
