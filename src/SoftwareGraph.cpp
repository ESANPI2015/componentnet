#include "SoftwareGraph.hpp"

namespace Software {


// Concept Ids
const UniqueId Graph::InterfaceId       = "Software::Graph::Interface";
const UniqueId Graph::InputId           = "Software::Graph::Input";
const UniqueId Graph::OutputId          = "Software::Graph::Output";
const UniqueId Graph::AlgorithmId       = "Software::Graph::Algorithm";
const UniqueId Graph::ImplementationId  = "Software::Graph::Implementation";
// Relation Ids
const UniqueId Graph::DependsOnId       = "Software::Graph::DependsOn";
const UniqueId Graph::NeedsId           = "Software::Graph::Needs";
const UniqueId Graph::ProvidesId        = "Software::Graph::Provides";
const UniqueId Graph::RealizedById      = "Software::Graph::RealizedBy";

// GRAPH STUFF
void Graph::createMainConcepts()
{
    createComponent(Graph::AlgorithmId, "ALGORITHM"); // Abstract, semantic definition (e.g. Localize)
    Component::Network::createInterface(Graph::InterfaceId, "INTERFACE"); // Abstract, semantic interface (e.g. Position)
    Component::Network::createInterface(Graph::InputId, "INPUT", Hyperedges{Graph::InterfaceId});
    Component::Network::createInterface(Graph::OutputId, "OUTPUT", Hyperedges{Graph::InterfaceId});
    createComponent(Graph::ImplementationId, "IMPLEMENTATION", Hyperedges{Graph::AlgorithmId}); // Concrete algorithm (e.g. 3DSLAM.cpp)

    // Relations
    subrelationFrom(Graph::DependsOnId, Hyperedges{Graph::InputId}, Hyperedges{Graph::OutputId}, CommonConceptGraph::ConnectsId);
    get(Graph::DependsOnId)->updateLabel("DEPENDS-ON");

    subrelationFrom(Graph::NeedsId, Hyperedges{Graph::AlgorithmId}, Hyperedges{Graph::InputId}, CommonConceptGraph::HasAId);
    get(Graph::NeedsId)->updateLabel("NEEDS");

    subrelationFrom(Graph::ProvidesId, Hyperedges{Graph::AlgorithmId}, Hyperedges{Graph::OutputId}, CommonConceptGraph::HasAId);
    get(Graph::ProvidesId)->updateLabel("PROVIDES");

    relate(Graph::RealizedById, Hyperedges{Graph::AlgorithmId}, Hyperedges{Graph::ImplementationId}, "REALIZED-BY");
}

Graph::Graph()
{
    createMainConcepts();
}

Graph::Graph(const Hypergraph& A)
: Component::Network(A)
{
    createMainConcepts();
}

Graph::~Graph()
{
}

Hyperedges Graph::createAlgorithm(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createComponent(uid, name, suids.empty() ? Hyperedges{Graph::AlgorithmId} : intersect(algorithmClasses(), suids));
}

Hyperedges Graph::createInput(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createInterface(uid, name, suids.empty() ? Hyperedges{Graph::InputId} : intersect(inputClasses(), suids));
}

Hyperedges Graph::createOutput(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createInterface(uid, name, suids.empty() ? Hyperedges{Graph::OutputId} : intersect(outputClasses(), suids));
}

Hyperedges Graph::createInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return Component::Network::createInterface(uid, name, suids.empty() ? Hyperedges{Graph::InterfaceId} : intersect(interfaceClasses(), suids));
}

Hyperedges Graph::createImplementation(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createAlgorithm(uid, name, suids.empty() ? Hyperedges{Graph::ImplementationId} : intersect(implementationClasses(), suids));
}

Hyperedges Graph::algorithmClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(componentClasses(name, Hyperedges{Graph::AlgorithmId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Graph::interfaceClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(Component::Network::interfaceClasses(name, Hyperedges{Graph::InterfaceId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Graph::inputClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(interfaceClasses(name, Hyperedges{Graph::InputId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Graph::outputClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(interfaceClasses(name, Hyperedges{Graph::OutputId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Graph::implementationClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(algorithmClasses(name, Hyperedges{Graph::ImplementationId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Graph::algorithms(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = algorithmClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Graph::interfaces(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = interfaceClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Graph::inputs(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = inputClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Graph::outputs(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = outputClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Graph::implementations(const std::string& name, const std::string& className) const
{
    // Get all super classes
    Hyperedges classIds = implementationClasses(className);
    // ... and then the instances of them
    return instancesOf(classIds, name);
}
Hyperedges Graph::providesInterface(const Hyperedges& algorithmIds, const Hyperedges& outputIds)
{
    Hyperedges result;
    // An algorithm class or instance can only have an output instance
    Hyperedges fromIds = unite(intersect(this->algorithms(), algorithmIds), intersect(algorithmClasses(), algorithmIds));
    Hyperedges toIds = intersect(this->outputs(), outputIds);
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Graph::ProvidesId));
        }
    }
    return result;
}

Hyperedges Graph::needsInterface(const Hyperedges& algorithmIds, const Hyperedges& inputIds)
{
    Hyperedges result;
    // An algorithm class or instance can only have an output instance
    Hyperedges fromIds = unite(intersect(this->algorithms(), algorithmIds), intersect(algorithmClasses(), algorithmIds));
    Hyperedges toIds = intersect(this->inputs(), inputIds);
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Graph::NeedsId));
        }
    }
    return result;
}

Hyperedges Graph::dependsOn(const Hyperedges& inputIds, const Hyperedges& outputIds)
{
    Hyperedges result;
    // For now only input instances can depend on output instances
    Hyperedges fromIds = intersect(this->inputs(), inputIds);
    Hyperedges toIds = intersect(this->outputs(), outputIds);
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{toId}, Hyperedges{fromId}, Graph::DependsOnId));
        }
    }
    return result;
}


std::vector< Software::Graph > Software::Graph::generateAllImplementationNetworks() const
{
    std::vector< Software::Graph > results;
    results.push_back(*this);

    // Cycle through all algorithm instances
    Hyperedges algUids(algorithms());
    for (const UniqueId& algUid : algUids)
    {
        // Find all implementation classes
        Hyperedges algClassUids(instancesOf(Hyperedges{algUid},"", Hypergraph::TraversalDirection::FORWARD));
        Hyperedges implClassUids(directSubclassesOf(algClassUids));

        // For each possible implementation (except of the first one) we have a new possibility
        std::vector< Software::Graph > newResults;
        for (const Software::Graph& current : results)
        {
            for (const UniqueId& implClassUid : implClassUids)
            {
                // Store a copy of the current graph before modification
                Software::Graph newResult(current);

                // create a new possiblity
                newResult.factFrom(Hyperedges{algUid}, newResult.instantiateComponent(Hyperedges{implClassUid}, newResult.read(algUid).label()), Graph::RealizedById);
                newResults.push_back(newResult);
            }
        }
        results = newResults;
    }

    // Reconstruct wiring of implementation instances
    for (const UniqueId& algUid : algUids)
    {
        // Find interfaces
        Hyperedges algInterfaceUids(interfacesOf(Hyperedges{algUid}));
        for (const UniqueId& algInterfaceUid : algInterfaceUids)
        {
            // Find other interfaces
            Hyperedges endpointUids(endpointsOf(Hyperedges{algInterfaceUid}));
            for (const UniqueId& otherAlgInterfaceUid : endpointUids)
            {
                // Find other algorithms
                Hyperedges otherAlgUids(intersect(algUids, interfacesOf(Hyperedges{otherAlgInterfaceUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                for (const UniqueId& otherAlgUid : otherAlgUids)
                {
                    // We now have algUid -> algInterfaceUid -> otherAlgInterfaceUid -> otherAlgUid
                    // We have to find implUid -> implInterfaceUid -> otherImpleInterfaceUid -> otherImplUid in ALL results
                    for (Software::Graph& current : results)
                    {
                        // NOTE: For each result, we have different instances (at most one though)
                        // To find them, we need to get all facts of Graph::RealizedById, which also point from algUid or otherAlgUid
                        Hyperedges implUids(current.to(current.factsOf(Graph::RealizedById, "", Hypergraph::TraversalDirection::INVERSE, Hyperedges{algUid})));
                        Hyperedges otherImplUids(current.to(current.factsOf(Graph::RealizedById, "", Hypergraph::TraversalDirection::INVERSE, Hyperedges{otherAlgUid})));
                        // Find the correct interfaces ... by ownership & name
                        Hyperedges implInterfaceUids(current.interfacesOf(implUids, read(algInterfaceUid).label()));
                        Hyperedges otherImplInterfaceUids(current.interfacesOf(otherImplUids, read(otherAlgInterfaceUid).label()));
                        // Wire
                        // NOTE: implInterfaceUids are inputs, otherImpleInterfaceUids are outputs
                        current.dependsOn(implInterfaceUids, otherImplInterfaceUids);
                    }
                }
            }
        }
    }

    return results;
}

}
