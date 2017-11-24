#ifndef _COMPONENT_NETWORK_HPP
#define _COMPONENT_NETWORK_HPP

#include "DomainSpecificGraph.hpp"

namespace Component {

/*
    This subclass of the DomainSpecificGraph class
    introduces the notion of COMPONENT, INTERFACE, CONNECTOR & NETWORK

    Main concept(s):
    COMPONENT
    INTERFACE
    NETWORK

    Main relation(s):
    COMPONENT <-- HAS-A --> INTERFACE
    INTERFACE <-- CONNECTED-TO --> INTERFACE
    COMPONENT <-- PART-OF --> NETWORK
    NETWORK   <-- IS-A --> COMPONENT

    Open questions:
    * Do we need CONNECTORS? In software these are just components themself ... In Hardware they nicely encode the concept of a BUS
*/

class Network;

class Network : public Domain::Graph
{
    public:
        // Ids for identifiing main concepts
        static const UniqueId ComponentId;
        static const UniqueId InterfaceId;
        static const UniqueId NetworkId;
        // Ids for identifiing main relations
        static const UniqueId HasAInterfaceId;
        static const UniqueId ConnectedToInterfaceId;
        static const UniqueId PartOfNetworkId;

        // Constructor/Destructor
        Network();
        Network(Domain::Graph& A);
        ~Network();

        // Creates the main concepts
        void createMainConcepts();

        // Create classes
        Hyperedges createComponent(const UniqueId& uid, const std::string& name="");
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="");
        Hyperedges createNetwork(const UniqueId& uid, const std::string& name="");
        // Create individuals
        Hyperedges instantiateComponent(const Hyperedges& componentIds);

        // Query classes
        Hyperedges componentClasses(const std::string& name="");
        Hyperedges interfaceClasses(const std::string& name="");
        Hyperedges networkClasses(const std::string& name="");
        // Query individuals
        Hyperedges components(const std::string& name="", const std::string& className="");
        Hyperedges interfaces(const std::string& name="", const std::string& className="");
        Hyperedges networks(const std::string& name="", const std::string& className="");

        // Specify a component
        Hyperedges hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds);
        // Connect interfaces
        Hyperedges connectInterface(const Hyperedges& fromInterfaceIds, const Hyperedges& toInterfaceIds);
        // Specify network
        Hyperedges partOfNetwork(const Hyperedges& componentIds, const Hyperedges& networkIds);
};

}

#endif
