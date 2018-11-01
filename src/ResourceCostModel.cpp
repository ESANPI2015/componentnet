#include "ResourceCostModel.hpp"

namespace ResourceCost {

const UniqueId Model::ConsumerUid = "ResourceCost::Model::Consumer";
const UniqueId Model::ProviderUid = "ResourceCost::Model::Provider";
const UniqueId Model::ResourceUid = "ResourceCost::Model::Resource";
const UniqueId Model::CostsUid    = "ResourceCost::Model::Costs";

void Model::setupMetaModel()
{
    create(Model::ConsumerUid, "Consumer");
    create(Model::ProviderUid, "Provider");
    create(Model::ResourceUid, "Resource");
    relate(Model::CostsUid, Hyperedges{Model::ConsumerUid, Model::ProviderUid}, Hyperedges{Model::ResourceUid}, "COSTS");
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
    Hyperedges all(subclassesOf(Hyperedges{Model::ResourceUid}));
    if(!isA(create(uid, name), intersect(unite(Hyperedges{Model::ResourceUid}, superResourceUids), all)).empty())
        return Hyperedges{uid};
    return Hyperedges();
}

Hyperedges Model::instantiateResource(const Hyperedges& resourceUids, const float amount)
{
    Hyperedges validResourceUids(intersect(resourceUids, subclassesOf(Hyperedges{Model::ResourceUid})));
    return instantiateDeepFrom(validResourceUids, std::to_string(amount));
}

Hyperedges Model::instantiateResourceFor(const Hyperedges& someUids, const Hyperedges& resourceUids, const float amount)
{
    Hyperedges instanceUids;
    Hyperedges validUids(intersect(someUids, providers()));
    for (const UniqueId& validUid : validUids)
    {
        Hyperedges instanceUid(instantiateResource(resourceUids, amount));
        hasA(Hyperedges{validUid}, instanceUid);
        instanceUids = unite(instanceUids, instanceUid);
    }
    return instanceUids;
}

Hyperedges Model::isConsumer(const Hyperedges& consumerUids)
{
    // NOTE: Somebody else has to check if an entity is both a instance and a class ... which would be kinda invalid
    return isA(consumerUids, Hyperedges{Model::ConsumerUid});
}

Hyperedges Model::consumerClasses(const std::string& name, const Hyperedges& suids) const
{
    Hyperedges all(subclassesOf(Hyperedges{Model::ConsumerUid}, name));
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
    Hyperedges all(subclassesOf(Hyperedges{Model::ProviderUid}, name));
    return intersect(all, subclassesOf(suids, name));
}

Hyperedges Model::providers(const std::string& name) const
{
    return instancesOf(providerClasses(), name);
}

Hyperedges Model::costs(const Hyperedges& consumerUids, const Hyperedges& providerUids, const Hyperedges& resourceInstanceUids)
{
    Hyperedges result;
    // For each pair of consumer and producer, create a fact
    Hyperedges validConsumerClassUids(consumerClasses());
    Hyperedges validProviderClassUids(providerClasses());
    Hyperedges validConsumerInstanceUids(instancesOf(validConsumerClassUids));
    Hyperedges validProviderInstanceUids(instancesOf(validProviderClassUids));
    Hyperedges validConsumerUids(intersect(consumerUids, unite(validConsumerClassUids, validConsumerInstanceUids)));
    Hyperedges validProviderUids(intersect(providerUids, unite(validProviderClassUids, validProviderInstanceUids)));
    for (const UniqueId& consumerUid : validConsumerUids)
    {
        for (const UniqueId& providerUid : validProviderUids)
        {
            for (const UniqueId& resourceUid : resourceInstanceUids)
            {
                result = unite(result, factFrom(Hyperedges{consumerUid, providerUid}, Hyperedges{resourceUid}, Model::CostsUid));
            }
        }
    }
    return result;
}

Hyperedges Model::resourcesOf(const Hyperedges& providerUids, const Hyperedges& resourceUids) const
{
    Hyperedges validResourceInstanceUids(instancesOf(subclassesOf(resourceUids)));
    return intersect(validResourceInstanceUids, childrenOf(providerUids));
}

Hyperedges Model::costsOf(const Hyperedges& consumerUids, const Hyperedges& providerUids, const Hyperedges& resourceUids) const
{
    Hyperedges validResourceInstanceUids(instancesOf(subclassesOf(resourceUids)));
    Hyperedges factUids(factsOf(Hyperedges{Model::CostsUid}));
    Hyperedges relsFromUids(intersect(relationsFrom(consumerUids), relationsFrom(providerUids)));
    Hyperedges matches(intersect(factUids, relsFromUids));
    return intersect(validResourceInstanceUids, to(matches));
}

Hyperedges Model::partitionFuncLeft (const ResourceCost::Model& rcm)
{
    Hyperedges consumerUids(rcm.consumers()); // get all consumer instances
    return consumerUids;
}
Hyperedges Model::partitionFuncRight (const ResourceCost::Model& rcm)
{
    Hyperedges providerUids(rcm.providers()); // get all provider instances
    return providerUids;
}

bool Model::matchFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid)
{
    // We can match all consumers to all providers
    return true;
}

float Model::costFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid)
{
    float minimum(std::numeric_limits<float>::infinity());
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{providerUid}));
    Hyperedges resourceCostUids(rcm.costsOf(Hyperedges{consumerUid}, Hyperedges{providerUid}));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const float r(std::stof(rcm.read(resourceUid).label()));
            const float c(std::stof(rcm.read(resourceCostUid).label()));
            // Calculate new minimum
            minimum = std::min(minimum, (r - c));
        }
    }
    return minimum;
}

void Model::mapFunc (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid) 
{
    ResourceCost::Model& rcm = static_cast< ResourceCost::Model& >(ccg);
    // Update all resources
    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{providerUid}));
    Hyperedges resourceCostUids(rcm.costsOf(Hyperedges{consumerUid}, Hyperedges{providerUid}));
    for (const UniqueId& resourceUid : resourceUids)
    {
        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
        for (const UniqueId& resourceCostUid : resourceCostUids)
        {
            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
            // Only costs of the same class can be handled
            if (intersect(resourceClassUids, resourceCostClassUids).empty())
                continue;
            const float r(std::stof(rcm.read(resourceUid).label()));
            const float c(std::stof(rcm.read(resourceCostUid).label()));
            // Update resources
            rcm.get(resourceUid)->updateLabel(std::to_string(r - c));
        }
    }
    // Now we should map consumer to provider.
    // e.g. make consumer PART-OF provider?
}

}
