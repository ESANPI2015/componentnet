#ifndef _RESOURCE_COST_MODEL_HPP
#define _RESOURCE_COST_MODEL_HPP

#include "CommonConceptGraph.hpp"

namespace ResourceCost {

/*
    This class introduces the following new concepts:
    * CONSUMER
    * PROVIDER
    * RESOURCE

    and the following relations:
    
    * CONSUMER NEEDS RESOURCE
    * COMSUMER CONSUMES RESOURCE
    * PROVIDER PROVIDES RESOURCE

    Rules:

    * CONSUMES implies NEEDS but not vice versa. That means that CONSUMES is a subrelation of NEEDS.

    A resource is a property of some provider, that means that:

    X -- provides --> N -- instance-of --> RESOURCE A
    X -- is-a --> PROVIDER

    In other words: "Entity X provides N units of resource A"

    When some X has a certain ressource some other entity Y might be in need of that resource.
    This is modeled as a new relation like this:

    Y -- needs --> M -- instance-of --> RESOURCE A
    Y -- is-a --> CONSUMER

    In other words: "Entity Y needs M units of resource A"

    However, some resources need only to exist for Y but others are depleted.
    Therefore another relation encodes this fact:

    Y -- consumes --> M -- instance-of --> RESOURCE A
    Y -- is-a --> CONSUMER

    In other words: "Entitiy Y consumes M units of resource A"

    Whenever a CONSUMER is about to be bound to a PROVIDER, the mapping algorithm should check if:
    For every pair NEEDS N, PROVIDES M of a RESOURCE A holds N <= M

    Furthermore, if a CONSUMER is bound to a PROVIDER:
    Every CONSUMES N will result in an update of PROVIDES M to reflect resource consumption

    To prevent overwriting information, the mapping algorithm should also check if:
    For every CONSUMES N and a PROVIDES M of a RESOURCE A, the sum of them (let it be X) holds X <= M


    Example:

    Y -- provides --> M of RESOURCE A
    Y -- consumes --> I of RESOURCE A
    Z -- consumes --> J of RESOURCE A

    Y -- mapped-to --> X
    Z -- mapped-to --> X

    ONLY, IFF

    I <= M, J <= M, I+J <= M

*/

class Model;

class Model: public CommonConceptGraph
{
    public:
        static const UniqueId ConsumerUid;
        static const UniqueId ProviderUid;
        static const UniqueId ResourceUid;
        static const UniqueId NeedsUid;
        static const UniqueId ProvidesUid;
        static const UniqueId ConsumesUid;
        static const UniqueId MappedToUid;

        // Creates the above fundamental concepts/relations
        void setupMetaModel();

        // Constructor/Destructor
        Model();
        Model(const Hypergraph& A);
        ~Model();

        // Define resource types
        Hyperedges defineResource(const UniqueId& uid, const std::string& name="Resource", const Hyperedges& superResourceUids = Hyperedges{ResourceUid});
        // Instantiate a resource of a given amount
        Hyperedges instantiateResource(const Hyperedges& resourceClassUids, const float amount=0.f);

        // Make something a consumer
        Hyperedges isConsumer(const Hyperedges& consumerUids);
        Hyperedges consumerClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ConsumerUid}) const;
        Hyperedges consumers(const std::string& name="") const;
        // Make something a provider
        Hyperedges isProvider(const Hyperedges& providerUids);
        Hyperedges providerClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ProviderUid}) const;
        Hyperedges providers(const std::string& name="") const;

        // Bind resources to consumer or provider
        Hyperedges needs(const Hyperedges& consumerUids, const Hyperedges& resourceUids);
        Hyperedges provides(const Hyperedges& providerUids, const Hyperedges& resourceUids);
        // This states that a consumer actually consumes a resource
        // NOTE: This should be stated ONLY if a resource is in fact consumable
        Hyperedges consumes(const Hyperedges& consumerUids, const Hyperedges& resourceUids);

        // Returns all the resources a consumer needs/consumes (optional: filtered by resource type)
        Hyperedges demandsOf(const Hyperedges& consumerUids, const Hyperedges& resourceClassUids = Hyperedges{ResourceUid}) const;
        // Returns all the resources of providers (optional: filtered by resource type)
        Hyperedges resourcesOf(const Hyperedges& providerUids, const Hyperedges& resourceClassUids = Hyperedges{ResourceUid}) const;
        // Returns all consumers mapped to providers
        Hyperedges consumersOf(const Hyperedges& providerUids) const;
        // Returns all providers to which consumerUids are mapped to
        Hyperedges providersOf(const Hyperedges& consumerUids) const;
        // Check if a provider fullfills all resource needs of a consumer
        // Returns >= 0 if satisfiable and < 0 if not satisfiable
        float satisfies(const Hyperedges& providerUids, const Hyperedges& consumerUids) const;

        // Advanced functions
        static Hyperedges partitionFuncLeft (const ResourceCost::Model& rcm);
        static Hyperedges partitionFuncRight (const ResourceCost::Model& rcm);
        static float matchFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static void mapFunc (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid); 
};

}

#endif
