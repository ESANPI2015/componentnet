#ifndef _HW_COMPUTATIONAL_NETWORK_HPP
#define _HW_COMPUTATIONAL_NETWORK_HPP

#include "ComponentNetwork.hpp"

namespace Hardware {
namespace Computational {

/*
    This class specifies component networks
    of a specific domain: the domain of processing hardware

    In this domain, the following components are defined:
    DEVICE
    PROCESSOR
    BUS

    The BUS component is always accompanied by interfaces ... otherwise it is not valid

    Even though INTERFACE concepts exist in the Component::Network domain,
    they are subclassed to distinguish them later on from other domains:
    Consider for example the case if we'd NOT subclass them ... then also e.g. Software interfaces could be considered parts of the Hardware domain ....
    which is not what we want.
*/

class Network;

class Network: public Component::Network
{
    public:
        // Concepts part of this domain
        static const UniqueId DeviceId;
        static const UniqueId ProcessorId;
        static const UniqueId InterfaceId;
        static const UniqueId BusId;

        // Constructor/Destructor
        Network();
        Network(Component::Network& A);
        ~Network();

        // Creates the main concepts
        void createMainConcepts();

        // Factory functions
        // NOTE: These create classes, not individuals
        Hyperedges createDevice(const UniqueId& uid, const std::string& name="Device");
        Hyperedges createProcessor(const UniqueId& uid, const std::string& name="Processor");
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface");
        Hyperedges createBus(const UniqueId& uid, const std::string& name="Bus");

        // Queries
        // NOTE: These return the subclasses of the corresponding main concepts
        Hyperedges deviceClasses(const std::string& name="");
        Hyperedges processorClasses(const std::string& name="");
        Hyperedges interfaceClasses(const std::string& name="");
        Hyperedges busClasses(const std::string& name="");
        // NOTE: These return the individuals of all the corresponding classes
        Hyperedges devices(const std::string& name="", const std::string& className="");
        Hyperedges processors(const std::string& name="", const std::string& className="");
        Hyperedges interfaces(const Hyperedges deviceIds, const std::string& name="", const std::string& className=""); //< If a deviceId is given, only its interfaces are returned
        Hyperedges busses(const std::string& name="", const std::string& className="");
};

}
}

#endif
