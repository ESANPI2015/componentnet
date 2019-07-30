#include "ComponentNetwork.hpp"

namespace Component {

const UniqueId Network::ComponentId            = "Component::Network::Component";
const UniqueId Network::InterfaceId            = "Component::Network::Interface";
const UniqueId Network::ValueId                = "Component::Network::Value";
const UniqueId Network::HasAInterfaceId        = "Component::Network::HasAInterface";
const UniqueId Network::HasAValueId            = "Component::Network::HasAValue";
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
    concept(Network::ValueId, "VALUE");
    concept(Network::InterfaceId, "INTERFACE");
    concept(Network::ComponentId, "COMPONENT");

    subrelationFrom(Network::HasAInterfaceId, Hyperedges{Network::ComponentId}, Hyperedges{Network::InterfaceId}, CommonConceptGraph::HasAId);
    subrelationFrom(Network::HasAValueId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::ValueId}, CommonConceptGraph::HasAId);
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

Hyperedges Network::createValue(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    if (!isA(concept(uid, name), intersect(unite(Hyperedges{Network::ValueId}, suids), valueClasses())).empty())
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

Hyperedges Network::valueClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(subclassesOf(Hyperedges{Network::ValueId}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::instantiateComponent(const Hyperedges& componentIds, const std::string& newName)
{
    // When instantiating a component, we also want to instantiate the interfaces
    // NOTE: Here we have to clone the facts of any subrelation of 'HasAInterface'
    //Hyperedges superClassUids(subclassesOf(componentIds,"",FORWARD));
    //Hyperedges instanceUid(instantiateFrom(componentIds, newName));
    //for (const UniqueId& superClassUid : superClassUids)
    //{
    //    Hyperedges interfacesToBeCloned(interfacesOf(Hyperedges{superClassUid}));
    //    for (const UniqueId& interfaceUid : interfacesToBeCloned)
    //    {
    //        factFromAnother(instanceUid, instantiateAnother(Hyperedges{interfaceUid}), factsOf(subrelationsOf(Hyperedges{Network::HasAInterfaceId}), Hyperedges{superClassUid}, Hyperedges{interfaceUid}));
    //    }
    //}
    //return instanceUid;
    // I. Find all superclasses
    Hyperedges superClassUids(subclassesOf(componentIds,"",FORWARD));
    Hyperedges instanceUid(instantiateFrom(componentIds, newName));
    for (const UniqueId& superClassUid : superClassUids)
    {
        // II. Find all descendants of all superclasses
        Hyperedges descUids(descendantsOf(Hyperedges{superClassUid}));
        std::map< UniqueId, Hyperedges > clones;
        // III. Clone descendants and their relations
        // b) between them and new instance
        for (const UniqueId& toBeClonedUid : descUids)
        {
            Hyperedges newUid(instantiateAnother(Hyperedges{toBeClonedUid}));
            factFromAnother(instanceUid, newUid, factsOf(subrelationsOf(Hyperedges{CommonConceptGraph::HasAId}), Hyperedges{superClassUid}, Hyperedges{toBeClonedUid}));
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
    return instanceUid;
}

Hyperedges Network::instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name)
{
    Hyperedges result;
    for (const UniqueId& componentId : componentIds)
    {
        Hyperedges newIfs(instantiateFrom(interfaceClassIds, name));
        hasInterface(Hyperedges{componentId}, newIfs);
        result = unite(result, newIfs);
    }
    return result;
}

Hyperedges Network::components(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = componentClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::interfaces(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = interfaceClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::values(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = valueClasses(className);
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

Hyperedges Network::valuesOf(const Hyperedges& uids, const std::string& value, const TraversalDirection dir) const
{
    Hyperedges result;
    // For empty uids, return empty result
    if (uids.empty())
        return result;

    Hyperedges subRelUids(subrelationsOf(Hyperedges{Network::HasAValueId}));
    Hyperedges factUids;
    switch (dir)
    {
        case INVERSE:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, value));
            break;
        case BOTH:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, value));
        case FORWARD:
            factUids = unite(factUids, factsOf(subRelUids, uids, Hyperedges()));
            result = unite(result, isPointingTo(factUids, value));
            break;
    }
    return result;
}

Hyperedges Network::instantiateValueFor(const Hyperedges& interfaceUids, const Hyperedges& valueClassUids, const std::string& value)
{
    Hyperedges result;
    for (const UniqueId& interfaceUid : interfaceUids)
    {
        Hyperedges newValueUids(instantiateFrom(valueClassUids, value));
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

Hyperedges Network::originalInterfacesOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    Hyperedges result;
    Hyperedges subRelUids(subrelationsOf(Hyperedges{Network::AliasOfId}));
    Hyperedges factUids;
    switch (dir)
    {
        case INVERSE:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
            break;
        case BOTH:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
        case FORWARD:
            factUids = unite(factUids, factsOf(subRelUids, uids, Hyperedges()));
            result = unite(result, isPointingTo(factUids, name));
            break;
    }
    return result;
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

Hyperedges Network::interfacesOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    Hyperedges result;
    // For empty uids, return empty result
    if (uids.empty())
        return result;
    Hyperedges subRelUids(subrelationsOf(Hyperedges{Network::HasAInterfaceId}));
    Hyperedges factUids;
    switch (dir)
    {
        case INVERSE:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
            break;
        case BOTH:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
        case FORWARD:
            factUids = unite(factUids, factsOf(subRelUids, uids, Hyperedges()));
            result = unite(result, isPointingTo(factUids, name));
            break;
    }
    return result;
}

Hyperedges Network::partOfComponent(const Hyperedges& componentIds, const Hyperedges& compositeComponentIds)
{
    Hyperedges result;
    Hyperedges fromIds(intersect(componentIds, components()));
    Hyperedges toIds(intersect(compositeComponentIds, unite(components(), componentClasses())));
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
    Hyperedges fromIds(intersect(interfaceIds, unite(interfaces(), interfaceClasses())));
    Hyperedges toIds(intersect(subInterfaceIds, interfaces()));
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
    Hyperedges result;
    // For empty uids, return empty result
    if (uids.empty())
        return result;
    Hyperedges subRelUids(subrelationsOf(Hyperedges{Network::HasASubInterfaceId}));
    Hyperedges factUids;
    switch (dir)
    {
        case INVERSE:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
            break;
        case BOTH:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
        case FORWARD:
            factUids = unite(factUids, factsOf(subRelUids, uids, Hyperedges()));
            result = unite(result, isPointingTo(factUids, name));
            break;
    }
    return result;
}

Hyperedges Network::subcomponentsOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    Hyperedges result;
    // For empty uids, return empty result
    if (uids.empty())
        return result;
    Hyperedges subRelUids(subrelationsOf(Hyperedges{Network::PartOfComponentId}));
    Hyperedges factUids;
    switch (dir)
    {
        case INVERSE:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
            break;
        case BOTH:
            factUids = unite(factUids, factsOf(subRelUids, Hyperedges(), uids));
            result = unite(result, isPointingFrom(factUids, name));
        case FORWARD:
            factUids = unite(factUids, factsOf(subRelUids, uids, Hyperedges()));
            result = unite(result, isPointingTo(factUids, name));
            break;
    }
    return result;
}


}
