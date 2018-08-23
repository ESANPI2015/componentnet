#include "ComponentNetwork.hpp"
#include "ResourceCostModel.hpp"

#include <iostream>
#include <limits>

int main (void)
{
    std::cout << "Test mapping of Component Networks using Resource Cost Model\n";

    Component::Network cn;

    std::cout << "Create components\n";
    
    // Define meta model
    cn.createComponent("Component::Class::A", "Component A");
    cn.createComponent("Component::Class::B", "Component B");
    cn.relate("Component::Relation::MappedTo", cn.find("Component A"), cn.find("Component B"), "MAPPED-TO");

    // Create instances
    cn.instantiateComponent(cn.find("Component A"), "a");
    cn.instantiateComponent(cn.find("Component A"), "b");
    cn.instantiateComponent(cn.find("Component A"), "c");
    cn.instantiateComponent(cn.find("Component B"), "1");
    cn.instantiateComponent(cn.find("Component B"), "2");
    cn.instantiateComponent(cn.find("Component B"), "3");

    std::cout << "Merging component network into resource model\n";
    ResourceCost::Model rm(cn);

    std::cout << "Create resources\n";

    rm.defineResource("Resource::Class::A", "Apples");

    std::cout << "Make components A consumers and components B providers\n";
    
    rm.isConsumer(rm.find("Component A"));
    rm.isProvider(rm.find("Component B"));

    std::cout << "Assign resources to providers\n";
    
    rm.instantiateResourceFor(rm.find("1"), rm.find("Apples"), 3.f);
    rm.instantiateResourceFor(rm.find("2"), rm.find("Apples"), 4.f);
    rm.instantiateResourceFor(rm.find("3"), rm.find("Apples"), 1.f);

    std::cout << "Assign costs for provider/consumer pairs\n";
    
    rm.costs(rm.find("a"), rm.find("1"), rm.instantiateResource(rm.find("Apples"), 1.f));
    rm.costs(rm.find("a"), rm.find("2"), rm.instantiateResource(rm.find("Apples"), 1.f));
    rm.costs(rm.find("a"), rm.find("3"), rm.instantiateResource(rm.find("Apples"), 1.f));
    rm.costs(rm.find("b"), rm.find("1"), rm.instantiateResource(rm.find("Apples"), 2.f));
    rm.costs(rm.find("b"), rm.find("2"), rm.instantiateResource(rm.find("Apples"), 2.f));
    rm.costs(rm.find("b"), rm.find("3"), rm.instantiateResource(rm.find("Apples"), 2.f));
    rm.costs(rm.find("c"), rm.find("1"), rm.instantiateResource(rm.find("Apples"), 3.f));
    rm.costs(rm.find("c"), rm.find("2"), rm.instantiateResource(rm.find("Apples"), 2.f));
    rm.costs(rm.find("c"), rm.find("3"), rm.instantiateResource(rm.find("Apples"), 1.f));

    std::cout << "Query consumers\n";
    std::cout << rm.consumers() << std::endl;
    std::cout << "Query providers\n";
    std::cout << rm.providers() << std::endl;
    std::cout << "Query resources of providers\n";
    for (const UniqueId& providerUid : rm.providers())
    {
        std::cout << providerUid << ": ";
        for (const UniqueId& resourceUid : rm.resourcesOf(Hyperedges{providerUid}))
        {
            std::cout << rm.get(resourceUid)->label() << " ";
        }
        std::cout << "\n";
    }

    std::cout << "Map components of type A to components of type B using the resource cost model\n";

    // Define mapping functions
    auto matchFunc = [] (const ResourceCost::Model& RM, const UniqueId& a, const UniqueId& b) -> bool {
        Hyperedges consumerUids(RM.consumers());
        Hyperedges producerUids(RM.providers());
        consumerUids = subtract(consumerUids, Hyperedges{"Component::Class::A", ResourceCost::Model::ConsumerUid});
        producerUids = subtract(producerUids, Hyperedges{"Component::Class::B", ResourceCost::Model::ProviderUid});
        if (std::find(consumerUids.begin(), consumerUids.end(), a) == consumerUids.end())
            return false;
        if (std::find(producerUids.begin(), producerUids.end(), b) == producerUids.end())
            return false;
        return true;
    };

    auto costFunc = [] (const ResourceCost::Model& RM, const UniqueId& a, const UniqueId& b) -> float {
        Hyperedges resourceUids(RM.resourcesOf(Hyperedges{b}));
        Hyperedges resourceCostUids(RM.costsOf(Hyperedges{a}, Hyperedges{b}));
        if (resourceUids.empty() || resourceCostUids.empty())
            return std::numeric_limits<float>::infinity();
        return std::stof(RM.read(*resourceUids.begin()).label()) - std::stof(RM.read(*resourceCostUids.begin()).label());
    };

    auto mapFunc = [] (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b) -> void {
        ResourceCost::Model RM(g); //expensive, but since we change g, we cannot use ResourceCost::Model above
        Hyperedges resourceUids(RM.resourcesOf(Hyperedges{b}));
        Hyperedges resourceCostUids(RM.costsOf(Hyperedges{a}, Hyperedges{b}));
        if (resourceUids.empty() || resourceCostUids.empty())
            return;
        // Update the resources
        const float resourcesLeft(std::stof(RM.read(*resourceUids.begin()).label()) - std::stof(RM.read(*resourceCostUids.begin()).label()));
        g.get(*resourceUids.begin())->updateLabel(std::to_string(resourcesLeft));
        g.factFrom(Hyperedges{a}, Hyperedges{b}, "Component::Relation::MappedTo");
    };

    std::cout << "Graph before map()\n";
    for (const UniqueId& conceptUid : rm.find())
    {
        std::cout << *rm.get(conceptUid) << std::endl;
        for (const UniqueId& relUid : rm.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << *rm.get(relUid) << std::endl;
        }
    }

    ResourceCost::Model rm2(rm.map(matchFunc, costFunc, mapFunc));

    std::cout << "Graph after map()\n";
    for (const UniqueId& conceptUid : rm2.find())
    {
        std::cout << *rm2.get(conceptUid) << std::endl;
        for (const UniqueId& relUid : rm2.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << *rm2.get(relUid) << std::endl;
        }
    }

    return 0;
}
