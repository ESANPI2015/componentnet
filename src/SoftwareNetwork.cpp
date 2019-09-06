#include "SoftwareNetwork.hpp"

namespace Software {


// Concept Ids
const UniqueId Network::InterfaceId       = "Software::Network::Interface";
const UniqueId Network::ImplementationInterfaceId       = "Software::Network::Implementation::Interface";
const UniqueId Network::AlgorithmId       = "Software::Network::Algorithm";
const UniqueId Network::ImplementationId  = "Software::Network::Implementation";
// Relation Ids
const UniqueId Network::DependsOnId       = "Software::Network::DependsOn";
const UniqueId Network::NeedsId           = "Software::Network::Needs";
const UniqueId Network::ProvidesId        = "Software::Network::Provides";
const UniqueId Network::ImplementsId      = "Software::Network::Implements";
const UniqueId Network::EncodesId         = "Software::Network::Encodes";
const UniqueId Network::RealizesId        = "Software::Network::Realizes";

// GRAPH STUFF
void Network::createMainConcepts()
{
    createComponent(Network::AlgorithmId, "ALGORITHM"); // Abstract, semantic definition (e.g. Localize)
    Component::Network::createInterface(Network::InterfaceId, "INTERFACE"); // Abstract, semantic interface (e.g. Position)
    createComponent(Network::ImplementationId, "IMPLEMENTATION", Hyperedges{Network::AlgorithmId}); // Concrete algorithm (e.g. 3DSLAM.cpp)
    Component::Network::createInterface(Network::ImplementationInterfaceId, "INTERFACE", Hyperedges{Network::InterfaceId}); // Concrete interface implementation (e.g. Vector3D)
    // NOTE: In general IMPLEMENTTIONS are also algorithms, so they can have interfaces as well

    // Relations
    subrelationFrom(Network::DependsOnId, Hyperedges{Network::InterfaceId}, Hyperedges{Network::InterfaceId}, Component::Network::ConnectedToInterfaceId);
    access(Network::DependsOnId).updateLabel("DEPENDS-ON");

    subrelationFrom(Network::NeedsId, Hyperedges{Network::AlgorithmId}, Hyperedges{Network::InterfaceId}, Component::Network::HasAInterfaceId);
    access(Network::NeedsId).updateLabel("NEEDS");

    subrelationFrom(Network::ProvidesId, Hyperedges{Network::AlgorithmId}, Hyperedges{Network::InterfaceId}, Component::Network::HasAInterfaceId);
    access(Network::ProvidesId).updateLabel("PROVIDES");

    relate(Network::ImplementsId, Hyperedges{Network::ImplementationId}, Hyperedges{Network::AlgorithmId}, "IMPLEMENTS");
    relate(Network::EncodesId, Hyperedges{Network::ImplementationInterfaceId}, Hyperedges{Network::InterfaceId}, "ENCODES");
    relate(Network::RealizesId, Hyperedges{Network::ImplementationId}, Hyperedges{Network::AlgorithmId}, "REALIZES");
}

Network::Network()
{
    createMainConcepts();
}

Network::Network(const Hypergraph& A)
: Component::Network(A)
{
    createMainConcepts();
}

Network::~Network()
{
}

Hyperedges Network::createAlgorithm(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createComponent(uid, name, suids.empty() ? Hyperedges{Network::AlgorithmId} : intersect(algorithmClasses(), suids));
}

Hyperedges Network::createInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return Component::Network::createInterface(uid, name, suids.empty() ? Hyperedges{Network::InterfaceId} : intersect(interfaceClasses(), suids));
}

Hyperedges Network::createImplementation(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createAlgorithm(uid, name, suids.empty() ? Hyperedges{Network::ImplementationId} : intersect(implementationClasses(), suids));
}

Hyperedges Network::createImplementationInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return Component::Network::createInterface(uid, name, suids.empty() ? Hyperedges{Network::ImplementationInterfaceId} : intersect(interfaceClasses(), suids));
}

Hyperedges Network::algorithmClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(componentClasses(name, Hyperedges{Network::AlgorithmId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::interfaceClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(Component::Network::interfaceClasses(name, Hyperedges{Network::InterfaceId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::implementationClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(algorithmClasses(name, Hyperedges{Network::ImplementationId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::implementationInterfaceClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(Component::Network::interfaceClasses(name, Hyperedges{Network::ImplementationInterfaceId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::algorithms(const std::string& name, const std::string& className) const
{
    // Get all super classes
    const Hyperedges& classIds(algorithmClasses(className));
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
Hyperedges Network::implementationInterfaces(const std::string& name, const std::string& className) const
{
    // Get all super classes
    const Hyperedges& classIds(implementationInterfaceClasses(className));
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Network::implementations(const std::string& name, const std::string& className) const
{
    // Get all super classes
    const Hyperedges& classIds(implementationClasses(className));
    // ... and then the instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::inputsOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return relatedTo(uids, Hyperedges{Network::NeedsId}, name, dir);
}

Hyperedges Network::outputsOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return relatedTo(uids, Hyperedges{Network::ProvidesId}, name, dir);
}

Hyperedges Network::implementationsOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return relatedTo(uids, Hyperedges{Network::ImplementsId}, name, dir);
}

Hyperedges Network::encodersOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return relatedTo(uids, Hyperedges{Network::EncodesId}, name, dir);
}

Hyperedges Network::realizersOf(const Hyperedges& uids, const std::string& name, const TraversalDirection dir) const
{
    return relatedTo(uids, Hyperedges{Network::RealizesId}, name, dir);
}

Hyperedges Network::providesInterface(const Hyperedges& algorithmIds, const Hyperedges& outputIds)
{
    Hyperedges result;
    // An algorithm class or instance can only have an output instance
    const Hyperedges& fromIds(unite(intersect(this->algorithms(), algorithmIds), intersect(algorithmClasses(), algorithmIds)));
    const Hyperedges& toIds(intersect(this->interfaces(), outputIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::ProvidesId));
        }
    }
    return result;
}

Hyperedges Network::needsInterface(const Hyperedges& algorithmIds, const Hyperedges& inputIds)
{
    Hyperedges result;
    // An algorithm class or instance can only have an output instance
    const Hyperedges& fromIds(unite(intersect(this->algorithms(), algorithmIds), intersect(algorithmClasses(), algorithmIds)));
    const Hyperedges& toIds(intersect(this->interfaces(), inputIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::NeedsId));
        }
    }
    return result;
}

Hyperedges Network::dependsOn(const Hyperedges& inputIds, const Hyperedges& outputIds)
{
    Hyperedges result;
    // For now only input instances can depend on output instances
    const Hyperedges& fromIds(intersect(this->interfaces(), inputIds));
    const Hyperedges& toIds(intersect(this->interfaces(), outputIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{toId}, Hyperedges{fromId}, Network::DependsOnId));
        }
    }
    return result;
}

Hyperedges Network::implements(const Hyperedges& implementationIds, const Hyperedges& algorithmIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(implementationClasses(), implementationIds));
    const Hyperedges& toIds(intersect(algorithmClasses(), algorithmIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::ImplementsId));
        }
    }
    return result;
}

Hyperedges Network::encodes(const Hyperedges& concreteInterfaceIds, const Hyperedges& interfaceIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(interfaceClasses(), concreteInterfaceIds));
    const Hyperedges& toIds(intersect(interfaceClasses(), interfaceIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::EncodesId));
        }
    }
    return result;
}

Hyperedges Network::realizes(const Hyperedges& implementationIds, const Hyperedges& algorithmIds)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(implementations(), implementationIds));
    const Hyperedges& toIds(intersect(algorithms(), algorithmIds));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Network::RealizesId));
        }
    }
    return result;
}

std::vector< Software::Network > Software::Network::generateAllImplementationNetworks() const
{
    std::vector< Software::Network > results;
    results.push_back(*this);

    // Cycle through all algorithm instances
    const Hyperedges& algUids(algorithms());
    for (const UniqueId& algUid : algUids)
    {
        // Find all implementation classes
        const Hyperedges& algClassUids(instancesOf(Hyperedges{algUid},"", Hypergraph::TraversalDirection::FORWARD));
        const Hyperedges& implClassUids(implementationsOf(algClassUids));

        // For each possible implementation (except of the first one) we have a new possibility
        std::vector< Software::Network > newResults;
        for (const Software::Network& current : results)
        {
            for (const UniqueId& implClassUid : implClassUids)
            {
                // Store a copy of the current graph before modification
                Software::Network newResult(current);

                // create a new possiblity
                newResult.realizes(newResult.instantiateComponent(Hyperedges{implClassUid}, newResult.access(algUid).label()), Hyperedges{algUid});
                newResults.push_back(newResult);
            }
        }
        results = newResults;
    }

    // Reconstruct wiring of implementation instances
    for (const UniqueId& algUid : algUids)
    {
        // Find interfaces
        const Hyperedges& algInterfaceUids(interfacesOf(Hyperedges{algUid}));
        for (const UniqueId& algInterfaceUid : algInterfaceUids)
        {
            // Find other interfaces
            const Hyperedges& endpointUids(endpointsOf(Hyperedges{algInterfaceUid}));
            for (const UniqueId& otherAlgInterfaceUid : endpointUids)
            {
                // Find other algorithms
                const Hyperedges& otherAlgUids(intersect(algUids, interfacesOf(Hyperedges{otherAlgInterfaceUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                for (const UniqueId& otherAlgUid : otherAlgUids)
                {
                    // We now have algUid -> algInterfaceUid -> otherAlgInterfaceUid -> otherAlgUid
                    // We have to find implUid -> implInterfaceUid -> otherImplInterfaceUid -> otherImplUid in ALL results
                    for (Software::Network& current : results)
                    {
                        // NOTE: For each result, we have different instances (at most one though)
                        // To find them, we need to get all facts of Network::RealizedById, which also point from algUid or otherAlgUid
                        const Hyperedges& implUids(current.realizersOf(Hyperedges{algUid}));
                        const Hyperedges& otherImplUids(current.realizersOf(Hyperedges{otherAlgUid}));
                        // Find the correct interfaces ... by ownership & name
                        const Hyperedges& implInterfaceUids(current.interfacesOf(implUids, access(algInterfaceUid).label()));
                        const Hyperedges& otherImplInterfaceUids(current.interfacesOf(otherImplUids, access(otherAlgInterfaceUid).label()));
                        // Wire
                        // NOTE: implInterfaceUids are inputs, otherImplInterfaceUids are outputs
                        current.dependsOn(implInterfaceUids, otherImplInterfaceUids);
                    }
                }
            }
        }
    }

    return results;
}

}
