#ifndef _DOMAIN_SPECIFIC_GRAPH_HPP
#define _DOMAIN_SPECIFIC_GRAPH_HPP

#include "CommonConceptGraph.hpp"

namespace Domain {

/*
    This subclass of the CommonConceptGraph class
    introduces the concept of DOMAIN

    This DOMAIN can partition the space of CONCEPTS & RELATIONS into domain specific variants

    Main conept(s):
    DOMAIN

    Main relation(s):
    CONCEPT <-- PART-OF --> DOMAIN
    RELATION <-- PART-OF --> DOMAIN
*/

class Graph;

class Graph : public CommonConceptGraph
{
    public:
        // Ids for identifiing main concepts
        static const UniqueId DomainId;
        // Ids for identifiing main relations
        static const UniqueId PartOfDomainId;

        // Constructor/Destructor
        Graph();
        Graph(CommonConceptGraph& A);
        ~Graph();

        // Creates the main concepts
        void createMainConcepts();

        // Factory functions
        // Creates a new domain with a specific UID and label
        Hyperedges createDomain(const UniqueId uid, const std::string& name="Domain");

        // Queries
        // Returns all domains with a specific name
        Hyperedges findDomainBy(const std::string& name="");
        // Returns all things belonging to a specific domain
        Hyperedges partsOfDomain(const Hyperedges uids, const std::string& name="");
        // Returns all concepts belonging to a specific domain
        Hyperedges conceptsOfDomain(const Hyperedges uids, const std::string& name="");
        Hyperedges relationsOfDomain(const Hyperedges uids, const std::string& name="");
        bool isPartOfDomain(const Hyperedges partIds, const Hyperedges domainIds)
        {
            return subtract(partIds, partsOfDomain(domainIds)).empty();
        }

        // Facts
        // Adds concepts to domains
        Hyperedges partOfDomain(const Hyperedges& conceptOrRelationIds, const Hyperedges& domainIds);
};

}

#endif
