#include "ComponentNetwork.hpp"

namespace Component {

const UniqueId Network::ComponentId            = "Component::Network::Component";
const UniqueId Network::InterfaceId            = "Component::Network::Interface";
const UniqueId Network::NetworkId              = "Component::Network::Network";
const UniqueId Network::ValueId                = "Component::Network::Value";
const UniqueId Network::HasAInterfaceId        = "Component::Network::HasAInterface";
const UniqueId Network::HasAValueId            = "Component::Network::HasAValue";
const UniqueId Network::ConnectedToInterfaceId = "Component::Network::ConnectedToInterface";
const UniqueId Network::PartOfNetworkId        = "Component::Network::PartOfNetwork";
const UniqueId Network::AliasOfId              = "Component::Network::AliasOf";

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
    create(Network::ValueId, "VALUE");

    subrelationFrom(Network::HasAInterfaceId, Hyperedges{Network::ComponentId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::HasAId);
    subrelationFrom(Network::HasAValueId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::ValueId}, CommonConceptGraph::HasAId);
    subrelationFrom(Network::ConnectedToInterfaceId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::ConnectsId);
    subrelationFrom(Network::PartOfNetworkId, Hyperedges{Network::ComponentId}, Hyperedges{Network::NetworkId}, CommonConceptGraph::PartOfId);
    relate(Network::AliasOfId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, "ALIAS-OF");

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

Hyperedges Network::hasValue(const Hyperedges& interfaceIds, const Hyperedges& valueIds)
{
    Hyperedges result;
    Hyperedges fromIds(intersect(interfaceIds, interfaces()));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : valueIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::HasAValueId));
        }
    }
    return result;
}

Hyperedges Network::valuesOf(const Hyperedges& interfaceUids, const std::string& value)
{
    // Get all uids <-- HAS-A --> X,value and return the X
    Hyperedges factUids(factsOf(Hyperedges{Network::HasAValueId}));
    Hyperedges relsFromUids(relationsFrom(interfaceUids));
    Hyperedges matches(intersect(factUids, relsFromUids));
    return to(matches, value);
}

Hyperedges Network::instantiateValueFor(const Hyperedges& interfaceUids, const std::string& value)
{
    Hyperedges result;
    for (const UniqueId& interfaceUid : interfaceUids)
    {
        Hyperedges newValueUids(instantiateDeepFrom(Hyperedges{Network::ValueId}, value));
        hasValue(Hyperedges{interfaceUid}, newValueUids);
        result = unite(result, newValueUids);
    }
    return result;
}

Hyperedges Network::aliasOf(const Hyperedges& aliasInterfaceUids, const Hyperedges& originalInterfaceUids)
{
    Hyperedges result;
    Hyperedges valid(interfaces());
    Hyperedges fromIds(intersect(aliasInterfaceUids, valid));
    Hyperedges toIds(intersect(originalInterfaceUids, valid));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::AliasOfId));
        }
    }
    return result;
}

Hyperedges Network::instantiateAliasInterfaceFor(const Hyperedges& parentUids, const Hyperedges& interfaceUids, const std::string& label)
{
    Hyperedges result;
    for (const UniqueId& parentUid : parentUids)
    {
        Hyperedges newInterfaceUids(instantiateAnother(interfaceUids, label));
        hasInterface(Hyperedges{parentUid}, newInterfaceUids);
        aliasOf(newInterfaceUids, interfaceUids);
        result = unite(result, newInterfaceUids);
    }
    return result;
}

Hyperedges Network::originalInterfacesOf(const Hyperedges& uids, const std::string& label)
{
    // Get all uids <-- ALIAS-OF --> X,label and return the X
    Hyperedges factUids(factsOf(Hyperedges{Network::AliasOfId}));
    Hyperedges relsFromUids(relationsFrom(uids));
    Hyperedges matches(intersect(factUids, relsFromUids));
    return to(matches, label);
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
