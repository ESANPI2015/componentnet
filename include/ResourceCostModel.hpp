#ifndef _RESOURCE_COST_MODEL_HPP
#define _RESOURCE_COST_MODEL_HPP

#include "CommonConceptGraph.hpp"

namespace ResourceCost {

/*
    This class introduces the following new concepts/relations:
    * CONSUMER
    * PROVIDER
    * RESOURCES
    * COSTS

    A resource is a property of some provider, that means that:

    X -- has --> N -- instance-of --> RESOURCE A
    X -- is-a --> PROVIDER

    In other words: "Entity X has N units of resource A"

    When some X has a certain ressource some other entity Y can cause these resources to be consumed.
    This is modeled as a new relation like this:

    Y --|
        COSTS --> M -- instance-of --> RESOURCE A
    X --|

    Y -- is-a --> CONSUMER
    X -- is-a --> PROVIDER

    In other words: "Y costs M units of resource A when applied to X"


    Example:
    
    X -- isA --> Function
    Y -- isA --> Processor

    Y -- has --> N -- instance-of --> Memory
    X,Y -- COSTS --> M -- instance-of --|

    In other words:
    "When X which is a Function is executed on Y which is a Processor and has N units of Memory X will consume/cost M units of of that Memory"
*/

class Model;

class Model: public CommonConceptGraph
{
    public:
        static const UniqueId ConsumerUid;
        static const UniqueId ProviderUid;
        static const UniqueId ResourceUid;
        static const UniqueId CostsUid;

        // Creates the above fundamental concepts/relations
        void setupMetaModel();

        // Constructor/Destructor
        Model();
        Model(const Hypergraph& A);
        ~Model();

        // Define resource types
        Hyperedges defineResource(const UniqueId& uid, const std::string& name="Resource", const Hyperedges& superResourceUids = Hyperedges{ResourceUid});
        // Instantiate a resource of a given amount
        Hyperedges instantiateResource(const Hyperedges& resourceUids, const float amount=0.f);
        // Instantiate a resource for some entity with a certain amount
        Hyperedges instantiateResourceFor(const Hyperedges& someUids, const Hyperedges& resourceUids, const float amount=0.f);
        // Make something a consumer
        Hyperedges isConsumer(const Hyperedges& consumerUids);
        Hyperedges consumerClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ConsumerUid}) const;
        Hyperedges consumers(const std::string& name="") const;
        // Make something a provider
        Hyperedges isProvider(const Hyperedges& providerUids);
        Hyperedges providerClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ProviderUid}) const;
        Hyperedges providers(const std::string& name="") const;
        // Given two entity sets, define the costs
        // NOTE: The amount of resources has to be encoded by a resource instance
        Hyperedges costs(const Hyperedges& consumerUids, const Hyperedges& providerUids, const Hyperedges& resourceInstanceUids);
        // Returns all the resources of some entities (optional: filtered by resource type)
        Hyperedges resourcesOf(const Hyperedges& providerUids, const Hyperedges& resourceUids = Hyperedges{ResourceUid}) const;
        // Returns all costs given two sets of entities (optional: filtered by resource type)
        Hyperedges costsOf(const Hyperedges& consumerUids, const Hyperedges& providerUids, const Hyperedges& resourceUids = Hyperedges{ResourceUid}) const;

        // Advanced functions
        static int partitionFunc (const ResourceCost::Model& rcm, const UniqueId& uid);
        static bool matchFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static float costFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static void mapFunc (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid); 
};

}

#endif
