#include "DomainSpecificGraph.hpp"

namespace Domain {

Graph::Graph(const UniqueId& uid, const std::string& name)
{
    Conceptgraph::create(uid, name);
    domainId = uid;
}

Graph::Graph(CommonConceptGraph& A, const UniqueId& uid, const std::string& name)
: CommonConceptGraph(A)
{
    Conceptgraph::create(uid, name);
    domainId = uid;
}

Graph::~Graph()
{
}

Hyperedges Graph::create(const UniqueId& uid, const std::string& label)
{
    Hyperedges uids = Conceptgraph::create(uid, label);
    return addToDomain(uids);
}

Hyperedges Graph::addToDomain(const Hyperedges& uids)
{
    if (CommonConceptGraph::partOf(uids, Hyperedges{domainId}).empty())
        return Hyperedges();
    return uids;
}

Hyperedges Graph::filterByDomain(const Hyperedges& uids, const std::string& name)
{
    return intersect(partsOfDomain(name), uids);
}

Hyperedges Graph::partsOfDomain(const std::string& name)
{
    Hyperedges parts(partsOf(Hyperedges{domainId}, name));
    Hyperedges subsOfParts(subclassesOf(parts, name));
    Hyperedges instOfSubs(instancesOf(subsOfParts, name));
    return unite(parts, unite(subsOfParts, instOfSubs));
}
}
