#ifndef _DOMAIN_SPECIFIC_GRAPH_HPP
#define _DOMAIN_SPECIFIC_GRAPH_HPP

#include "CommonConceptGraph.hpp"

namespace Domain {

/*
    This subclass of the CommonConceptGraph class
    introduces the concept of DOMAIN

    A domain could be a container of concepts which are PARTS of the domain.
    It can also be seen as an upper most superclass of all the concepts in the domain.

    The most sound thing would be:
    * Make things part-of the domain
    * When checking for membership, we have to check the parts as well as the subclasses & instances of these parts
*/

class Graph;

class Graph : public CommonConceptGraph
{
    private:
        // The domainId to be used
        UniqueId domainId;

    public:
        // Constructor/Destructor
        Graph(const UniqueId& uid="UnspecifiedDomainId", const std::string& name="UnspecifiedDomainName");
        Graph(CommonConceptGraph& A, const UniqueId& uid="UnspecifiedDomainId", const std::string& name="UnspecifiedDomainName");
        ~Graph();

        // Now we could override the create function.
        // That means whenever a derived class calls create(uid, name) or create(uid) it will implicitly create a PART-OF relation
        Hyperedges create(const UniqueId& uid, const std::string& label="");

        // add given uids to domain
        Hyperedges addToDomain(const Hyperedges& uids);

        // Return all parts of the domain
        Hyperedges partsOfDomain(const std::string& name="");

        // Filter out all Hyperedges which are not part of domain
        Hyperedges filterByDomain(const Hyperedges& uids, const std::string& name="");
};

}

#endif
