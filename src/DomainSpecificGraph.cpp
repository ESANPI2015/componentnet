#include "DomainSpecificGraph.hpp"

namespace Domain {

// UniqueIds
const UniqueId Graph::DomainId = "DomainSpecific::Graph::Domain";
const UniqueId Graph::PartOfDomainId = "DomainSpecific::Graph::PartOf";

void Graph::createMainConcepts()
{
    create(Graph::DomainId, "DOMAIN");
    relate(Graph::PartOfDomainId, Hyperedges{Conceptgraph::IsConceptId, Conceptgraph::IsRelationId}, Hyperedges{Graph::DomainId}, "PART-OF");
    subrelationOf(Hyperedges{Graph::PartOfDomainId}, Hyperedges{CommonConceptGraph::PartOfId});
}

Graph::Graph()
{
    createMainConcepts();
}

Graph::Graph(CommonConceptGraph& A)
: CommonConceptGraph(A)
{
    createMainConcepts();
}

Graph::~Graph()
{
}

Hyperedges Graph::createDomain(const UniqueId uid, const std::string& name)
{
    if (!create(uid, name).empty())
    {
        isA(Hyperedges{uid}, Graph::DomainId);
        return Hyperedges{uid};
    }
    return Hyperedges();
}

Hyperedges Graph::findDomainBy(const std::string& name)
{
    return subclassesOf(Hyperedges{Graph::DomainId}, name);
}

Hyperedges Graph::partsOfDomain(const Hyperedges uids, const std::string& name)
{
    // Get and filter domains
    Hyperedges domains(findDomainBy());
    domains = intersect(domains, uids);
    // Get parts and filter by concept
    Hyperedges parts(partsOf(domains, name));
    return parts;
}

Hyperedges Graph::conceptsOfDomain(const Hyperedges uids, const std::string& name)
{
    // Get parts and filter by concept
    Hyperedges parts(partsOfDomain(uids, name));
    parts = intersect(parts, Conceptgraph::find(name));
    return parts;
}

Hyperedges Graph::relationsOfDomain(const Hyperedges uids, const std::string& name)
{
    // Get parts and filter by relation
    Hyperedges parts(partsOfDomain(uids, name));
    parts = intersect(parts, Conceptgraph::relations(name));
    return parts;
}

Hyperedges Graph::partOfDomain(const Hyperedges& conceptOrRelationIds, const Hyperedges& domainIds)
{
    return relateFrom(conceptOrRelationIds, domainIds, Graph::PartOfDomainId);
}

}
