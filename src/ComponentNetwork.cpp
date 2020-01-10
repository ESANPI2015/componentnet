#include "ComponentNetwork.hpp"

namespace Component {

const UniqueId Network::ComponentId            = "Component::Network::Component";
const UniqueId Network::InterfaceId            = "Component::Network::Interface";
const UniqueId Network::HasAInterfaceId        = "Component::Network::HasAInterface";
const UniqueId Network::ConnectedToInterfaceId = "Component::Network::ConnectedToInterface";
const UniqueId Network::AliasOfId              = "Component::Network::AliasOf";
const UniqueId Network::PartOfComponentId      = "Component::Network::PartOfComponent";
const UniqueId Network::HasASubInterfaceId     = "Component::Network::HasASubInterface";

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
    concept(Network::InterfaceId, "INTERFACE");
    concept(Network::ComponentId, "COMPONENT");

    subrelationFrom(Network::HasAInterfaceId, Hyperedges{Network::ComponentId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::HasAId);
    subrelationFrom(Network::ConnectedToInterfaceId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::ConnectsId);
    subrelationFrom(Network::PartOfComponentId, Hyperedges{Network::ComponentId}, Hyperedges{Network::ComponentId}, CommonConceptGraph::PartOfId);
    subrelationFrom(Network::HasASubInterfaceId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::HasAId);
    relate(Network::AliasOfId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, "ALIAS-OF");
}

Hyperedges Network::createComponent(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if(!isA(concept(uid, name), intersect(unite(Hyperedges{Network::ComponentId}, suids), componentClasses())).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Network::createInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if (!isA(concept(uid, name), intersect(unite(Hyperedges{Network::InterfaceId}, suids), interfaceClasses())).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Network::componentClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(subclassesOf(Hyperedges{Network::ComponentId}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::interfaceClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(subclassesOf(Hyperedges{Network::InterfaceId}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::instantiateComponent(const Hyperedges& componentIds, const std::string& newName)
{
    // Possible optimizations:
    // * Why dont we use 'directSubclassesOf' instead of 'subclassesOf'? NO, THIS WILL NOT WORK BECAUSE NEED TO INHERIT ALSO FROM SUPERCLASSES
    // * Can we use 'const Hyperedges&' ? DONE
    // * Can we use a better algorithm (special traversal) to instantiate?
    // * Cloning children is creating redundant information (unless something is changed, e.g. making connections). So we should use this method ONLY if needed.
    Hyperedges instanceUids;
    for (const UniqueId& componentId : componentIds)
    {
        // I. Find all superclasses
        const Hyperedges& superclassUids(subclassesOf(Hyperedges{componentId}, "", FORWARD));
        const Hyperedges& instanceUid(instantiateFrom(Hyperedges{componentId}, newName));
        for (const UniqueId& superclassUid : superclassUids)
        {
            // II. Find all descendants for each superclass
            std::unordered_map< UniqueId, Hyperedges > clones;
            const Hyperedges& descUids(descendantsOf(Hyperedges{superclassUid}));
            // III. Clone descendants and their relations (for each superclass)
            // b) between them and new instance
            for (const UniqueId& toBeClonedUid : descUids)
            {
                const Hyperedges& newUid(instantiateAnother(Hyperedges{toBeClonedUid}));
                factFromAnother(instanceUid, newUid, factsOf(subrelationsOf(Hyperedges{CommonConceptGraph::HasAId}), Hyperedges{superclassUid}, Hyperedges{toBeClonedUid}));
                clones[toBeClonedUid] = newUid;
            }
            // a) between them
            for (const UniqueId& srcUid : descUids)
            {
                for (const UniqueId& dstUid : descUids)
                {
                    // TODO: Any facts or just descendative facts?
                    factFromAnother(clones[srcUid], clones[dstUid], factsOf(subrelationsOf(Hyperedges{CommonConceptGraph::HasAId}), Hyperedges{srcUid}, Hyperedges{dstUid}));
                }
            }
        }
        // Register new component instance in results
        instanceUids = unite(instanceUids, instanceUid);
    }
    return instanceUids;
}

Hyperedges Network::instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name)
{
    Hyperedges result;
    for (const UniqueId& componentId : componentIds)
    {
        const Hyperedges& newIfs(instantiateFrom(interfaceClassIds, name));
        hasInterface(Hyperedges{componentId}, newIfs);
        result = unite(result, newIfs);
    }
    return result;
}

Hyperedges Network::components(const std::string& name, const std::string& className) const
{
    // Get all super classes
    const Hyperedges classIds(componentClasses(className));
    // ... and then the instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::interfaces(const std::string& name, const std::string& className) const
{
    // Get all super classes
    const Hyperedges& classIds(interfaceClasses(className));
    // ... and then the instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::aliasOf(const Hyperedges& aliasInterfaceUids, const Hyperedges& originalInterfaceUids)
{
    Hyperedges result;
    const Hyperedges& valid(interfaces());
    const Hyperedges& fromIds(intersect(aliasInterfaceUids, valid));
    const Hyperedges& toIds(intersect(originalInterfaceUids, valid));
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
        const Hyperedges& newInterfaceUids(instantiateAnother(interfaceUids, label));
        hasInterface(Hyperedges{parentUid}, newInterfaceUids);
        aliasOf(newInterfaceUids, interfaceUids);
        result = unite(result, newInterfaceUids);
    }
    return result;
}

Hyperedges Network::originalInterfacesOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return CommonConceptGraph::relatedTo(uids, Hyperedges{Network::AliasOfId}, name, dir);
}


Hyperedges Network::hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(componentIds, unite(componentClasses(), components())));
    const Hyperedges& toIds(intersect(interfaceIds, interfaces()));
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
    const Hyperedges& valid(interfaces());
    const Hyperedges& fromIds(intersect(fromInterfaceIds, valid));
    const Hyperedges& toIds(intersect(toInterfaceIds, valid));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::ConnectedToInterfaceId));
        }
    }
    return result;
}

Hyperedges Network::interfacesOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return CommonConceptGraph::relatedTo(uids, Hyperedges{Network::HasAInterfaceId}, name, dir);
}

Hyperedges Network::partOfComponent(const Hyperedges& componentIds, const Hyperedges& compositeComponentIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(componentIds, components()));
    const Hyperedges& toIds(intersect(compositeComponentIds, unite(components(), componentClasses())));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::PartOfComponentId));
        }
    }
    return result;
}

Hyperedges Network::hasSubInterface(const Hyperedges& interfaceIds, const Hyperedges& subInterfaceIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(interfaceIds, unite(interfaces(), interfaceClasses())));
    const Hyperedges& toIds(intersect(subInterfaceIds, interfaces()));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, CommonConceptGraph::factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::HasASubInterfaceId));
        }
    }
    return result;
}

Hyperedges Network::subinterfacesOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return CommonConceptGraph::relatedTo(uids, Hyperedges{Network::HasASubInterfaceId}, name, dir);
}

Hyperedges Network::subcomponentsOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return CommonConceptGraph::relatedTo(uids, Hyperedges{Network::PartOfComponentId}, name, dir);
}


}
