#include "ResourceCostModel.hpp"

namespace ResourceCost {

const UniqueId Model::ConsumerUid = "ResourceCost::Model::Consumer";
const UniqueId Model::ProviderUid = "ResourceCost::Model::Provider";
const UniqueId Model::ResourceUid = "ResourceCost::Model::Resource";
const UniqueId Model::NeedsUid    = "ResourceCost::Model::Needs";
const UniqueId Model::ProvidesUid = "ResourceCost::Model::Provides";
const UniqueId Model::ConsumesUid = "ResourceCost::Model::Consumes";
const UniqueId Model::MappedToUid = "ResourceCost::Model::MappedTo";

void Model::setupMetaModel()
{
    concept(Model::ConsumerUid, "Consumer");
    concept(Model::ProviderUid, "Provider");
    concept(Model::ResourceUid, "Resource");
    relate(Model::NeedsUid, Hyperedges{Model::ConsumerUid}, Hyperedges{Model::ResourceUid}, "NEEDS");
    subrelationFrom(Model::ConsumesUid, Hyperedges{Model::ConsumerUid}, Hyperedges{Model::ResourceUid}, Model::NeedsUid);
    access(Model::ConsumesUid).label("CONSUMES");
    subrelationFrom(Model::ProvidesUid, Hyperedges{Model::ProviderUid}, Hyperedges{Model::ResourceUid}, CommonConceptGraph::HasAId);
    access(Model::ProvidesUid).label("PROVIDES");
    subrelationFrom(Model::MappedToUid, Hyperedges{Model::ConsumerUid}, Hyperedges{Model::ProviderUid}, CommonConceptGraph::PartOfId);
    access(Model::MappedToUid).label("MAPPED-TO");
}

Model::Model()
{
    setupMetaModel();
}

Model::Model(const Hypergraph& A)
: CommonConceptGraph(A)
{
    setupMetaModel();
}

Model::~Model()
{
}

Hyperedges Model::defineResource(const UniqueId& uid, const std::string& name, const Hyperedges& superResourceUids)
{
    const Hyperedges& all(subclassesOf(Hyperedges{Model::ResourceUid}));
    if(!isA(concept(uid, name), intersect(unite(Hyperedges{Model::ResourceUid}, superResourceUids), all)).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Model::instantiateResource(const Hyperedges& resourceUids, const float amount)
{
    const Hyperedges& validResourceUids(intersect(resourceUids, subclassesOf(Hyperedges{Model::ResourceUid})));
    return instantiateFrom(validResourceUids, std::to_string(amount));
}

Hyperedges Model::isConsumer(const Hyperedges& consumerUids)
{
    // NOTE: Somebody else has to check if an entity is both a instance and a class ... which would be kinda invalid
    return isA(consumerUids, Hyperedges{Model::ConsumerUid});
}

Hyperedges Model::consumerClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(subclassesOf(Hyperedges{Model::ConsumerUid}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Model::consumers(const std::string& name) const
{
    return instancesOf(consumerClasses(), name);
}

Hyperedges Model::isProvider(const Hyperedges& providerUids)
{
    return isA(providerUids, Hyperedges{Model::ProviderUid});
}

Hyperedges Model::providerClasses(const std::string& name, const Hyperedges& suids) const
{
    const Hyperedges& all(subclassesOf(Hyperedges{Model::ProviderUid}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Model::providers(const std::string& name) const
{
    return instancesOf(providerClasses(), name);
}

Hyperedges Model::needs(const Hyperedges& consumerUids, const Hyperedges& resourceUids)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(consumerUids, unite(consumerClasses(), consumers())));
    const Hyperedges& toIds(intersect(resourceUids, instancesOf(subclassesOf(Hyperedges{Model::ResourceUid}))));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Model::NeedsUid));
        }
    }
    return result;
}

Hyperedges Model::provides(const Hyperedges& providerUids, const Hyperedges& resourceUids)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(providerUids, unite(providerClasses(), providers())));
    const Hyperedges& toIds(intersect(resourceUids, instancesOf(subclassesOf(Hyperedges{Model::ResourceUid}))));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Model::ProvidesUid));
        }
    }
    return result;
}

Hyperedges Model::consumes(const Hyperedges& consumerUids, const Hyperedges& resourceUids)
{
    Hyperedges result;
    const Hyperedges& fromIds(intersect(consumerUids, unite(consumerClasses(), consumers())));
    const Hyperedges& toIds(intersect(resourceUids, instancesOf(subclassesOf(Hyperedges{Model::ResourceUid}))));
    for (const UniqueId& fromId : fromIds)
    {
        for (const UniqueId& toId : toIds)
        {
            result = unite(result, factFrom(Hyperedges{fromId}, Hyperedges{toId}, Model::ConsumesUid));
        }
    }
    return result;
}

Hyperedges Model::demandsOf(const Hyperedges& consumerUids, const Hyperedges& resourceClassUids) const
{
    const Hyperedges& validResourceInstanceUids(instancesOf(subclassesOf(resourceClassUids)));
    const Hyperedges& candidates(relatedTo(consumerUids, Hyperedges{Model::NeedsUid}, "", FORWARD));
    return intersect(candidates, validResourceInstanceUids);
}

Hyperedges Model::resourcesOf(const Hyperedges& providerUids, const Hyperedges& resourceClassUids) const
{
    const Hyperedges& validResourceInstanceUids(instancesOf(subclassesOf(resourceClassUids)));
    const Hyperedges& candidates(relatedTo(providerUids, Hyperedges{Model::ProvidesUid}, "", FORWARD));
    return intersect(candidates, validResourceInstanceUids);
}

Hyperedges Model::consumersOf(const Hyperedges& providerUids) const
{
    return relatedTo(providerUids, Hyperedges{Model::MappedToUid},"", INVERSE);
}

Hyperedges Model::providersOf(const Hyperedges& consumerUids) const
{
    return relatedTo(consumerUids, Hyperedges{Model::MappedToUid},"", FORWARD);
}

float Model::satisfies(const Hyperedges& providerUids, const Hyperedges& consumerUids) const
{
    float minimum(1.0f);
    // To check satisfiability
    // we have to check for every consumer,provider pair, that
    // each needed resource N of type X fullfills N <= M of any provided resource M of type X minus already consumed amounts of that resource
    for (const UniqueId& consumerUid : consumerUids)
    {
        // Collect demands of consumer
        const Hyperedges& neededResourceUids(demandsOf(Hyperedges{consumerUid}));
        for (const UniqueId& providerUid : providerUids)
        {
            // Collect available resources of provider
            const Hyperedges& availableResourceUids(resourcesOf(Hyperedges{providerUid}));
            // ... by also taking into account potential consumers of that resource
            const Hyperedges& mappedConsumerUids(consumersOf(Hyperedges{providerUid}));
            const Hyperedges& consumedResourceUids(mappedConsumerUids.size() ? isPointingTo(factsOf(subrelationsOf(Hyperedges{Model::ConsumesUid}), mappedConsumerUids)) : Hyperedges());
            for (const UniqueId& neededResourceUid : neededResourceUids)
            {
                // Get amount of needed resources (demand)
                const float needed(std::stof(access(neededResourceUid).label()));
                const Hyperedges& neededResourceClassUids(instancesOf(Hyperedges{neededResourceUid}, "", FORWARD));
                bool matched = false;
                for (const UniqueId& availableResourceUid : availableResourceUids)
                {
                    // Get amount of available resources
                    const float available(std::stof(access(availableResourceUid).label()));
                    const Hyperedges& availableResourceClassUids(instancesOf(Hyperedges{availableResourceUid}, "", FORWARD));
                    // If types mismatch, continue
                    if (intersect(availableResourceClassUids, neededResourceClassUids).empty())
                        continue;
                    // When we are here, we have found a matching pair of resources
                    // Calculate already consumed resources
                    float used = 0.f;
                    for (const UniqueId& consumedResourceUid : consumedResourceUids)
                    {
                        const Hyperedges& consumedResourceClassUids(instancesOf(Hyperedges{consumedResourceUid}, "", FORWARD));
                        // If types mismatch, continue
                        if (intersect(availableResourceClassUids, consumedResourceClassUids).empty())
                            continue;
                        // Update usage
                        used += std::stof(access(consumedResourceUid).label());
                    }
                    // Calculate a quantity which reflects the amount of resources consumed
                    // A very small value stands for a high amount of resources needed
                    // A high value (<= 1) stands for a low amount of resources needed
                    // A negative value stands for unsatisfiable demands
                    const float cost((available - used - needed) / available);
                    // For consistent handling, we remember the 'worst'/minimal value
                    minimum = std::min(minimum, cost);
                    // Check if demands can be fullfilled
                    // NOTE: Even if the new consumer would consume the resource, we just check this constraint to handle both, existential and consumable resources
                    if (cost < 0.f)
                    {
                        std::cout << "SAT-CHECK FAILED: Available: " << available << " Used: " << used << " Needed: " << needed << "\n";
                        return cost;
                    }
                    // When we are here, the demands have been fullfilled!!! That means, we can leave the loop and check another demand.
                    matched = true;
                    break;
                }
                // Check if we have found a match
                if (!matched)
                {
                    // If not, we are done :/
                    std::cout << "SAT-CHECK FAILED: No resources found for " << neededResourceUid << "\n";
                    return -std::numeric_limits<float>::infinity();
                }
            }
        }
    }
    return minimum;
}

Hyperedges Model::partitionFuncLeft (const ResourceCost::Model& rcm)
{
    const Hyperedges& consumerUids(rcm.consumers()); // get all consumer instances
    return consumerUids;
}
Hyperedges Model::partitionFuncRight (const ResourceCost::Model& rcm)
{
    const Hyperedges& providerUids(rcm.providers()); // get all provider instances
    return providerUids;
}

float Model::matchFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid)
{
    // We can match all consumers to all providers
    return rcm.satisfies(Hyperedges{providerUid}, Hyperedges{consumerUid});
}

void Model::mapFunc (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid) 
{
    ResourceCost::Model& rcm(static_cast< ResourceCost::Model& >(ccg));
    // Assign consumer to provider
    rcm.factFrom(Hyperedges{consumerUid}, Hyperedges{providerUid}, ResourceCost::Model::MappedToUid);
}

}
