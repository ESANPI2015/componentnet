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

}
