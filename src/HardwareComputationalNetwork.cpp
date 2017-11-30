#include "HardwareComputationalNetwork.hpp"
#include <iostream>

namespace Hardware {
namespace Computational {

// DICTIONARY
// Concept Ids
const UniqueId Network::InterfaceId = "Hardware::Computational::Network::Interface";
const UniqueId Network::BusId       = "Hardware::Computational::Network::Bus";
const UniqueId Network::DeviceId    = "Hardware::Computational::Network::Device";
const UniqueId Network::ProcessorId = "Hardware::Computational::Network::Processor";

// Network
void Network::createMainConcepts()
{
    // Create concepts
    createComponent(Network::DeviceId, "DEVICE");
    isA(Hyperedges{Network::DeviceId}, Hyperedges{Component::Network::NetworkId}); // Make devices also networks
    createComponent(Network::ProcessorId, "PROCESSOR", Hyperedges{Network::DeviceId});
    Component::Network::createInterface(Network::InterfaceId, "INTERFACE");
    createComponent(Network::BusId, "BUS");
}

Network::Network()
: Component::Network()
{
    createMainConcepts();
}

Network::Network(Component::Network& A)
: Component::Network(A)
{
    createMainConcepts();
}

Network::~Network()
{
}

Hyperedges Network::processorClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(componentClasses(name, Hyperedges{Network::ProcessorId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::deviceClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(componentClasses(name, Hyperedges{Network::DeviceId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::interfaceClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(Component::Network::interfaceClasses(name, Hyperedges{Network::InterfaceId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::busClasses(const std::string& name, const Hyperedges& suids)
{
    Hyperedges all(componentClasses(name, Hyperedges{Network::BusId}));
    return suids.empty() ? all : intersect(all, subclassesOf(suids, name));
}

Hyperedges Network::createProcessor(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createComponent(uid, name, suids.empty() ? Hyperedges{Network::ProcessorId} : intersect(processorClasses(), suids));
}

Hyperedges Network::createDevice(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createComponent(uid, name, suids.empty() ? Hyperedges{Network::DeviceId} : intersect(deviceClasses(), suids));
}

Hyperedges Network::createInterface(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return Component::Network::createInterface(uid, name, suids.empty() ? Hyperedges{Network::InterfaceId} : intersect(interfaceClasses(), suids));
}

Hyperedges Network::createBus(const UniqueId& uid, const std::string& name, const Hyperedges& suids)
{
    return createComponent(uid, name, suids.empty() ? Hyperedges{Network::BusId} : intersect(busClasses(), suids));
}

Hyperedges Network::devices(const std::string& name, const std::string& className)
{
    // Get all device classes
    Hyperedges classIds = deviceClasses(className);
    // ... and return all instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::processors(const std::string& name, const std::string& className)
{
    // Get all processor classes
    Hyperedges classIds = processorClasses(className);
    // ... and return all instances of them
    return instancesOf(classIds, name);
}

Hyperedges Network::interfaces(const Hyperedges deviceIds, const std::string& name, const std::string& className)
{
    // Get all interfaceClasses
    Hyperedges classIds = interfaceClasses(className);
    // ... get the instances with the given name
    Hyperedges result = instancesOf(classIds, name);
    if (deviceIds.size())
    {
        result = intersect(result, interfacesOf(deviceIds, name));
    }
    return result;
}

Hyperedges Network::busses(const std::string& name, const std::string& className)
{
    // Get all busClasses
    Hyperedges classIds = busClasses(className);
    // ... and return all instances of them
    return instancesOf(classIds, name);
}

}
}
