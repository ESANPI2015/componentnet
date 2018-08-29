#ifndef _COMPONENT_NETWORK_HPP
#define _COMPONENT_NETWORK_HPP

#include "CommonConceptGraph.hpp"

namespace Component {

/*
    This subclass of the CommonConceptGraph class
    introduces the notion of COMPONENT, INTERFACE & NETWORK

    Main concept(s):
    COMPONENT
    INTERFACE
    NETWORK

    Main relation(s):
    COMPONENT <-- HAS-A --> INTERFACE
    INTERFACE <-- CONNECTED-TO --> INTERFACE
    COMPONENT <-- PART-OF --> NETWORK
    NETWORK   <-- IS-A --> COMPONENT
    INTERFACE <-- ALIAS-OF --> INTERFACE  (used for renaming interfaces)
    INTERFACE <-- HAS-A --> VALUE (used to assign a constant/initial value to an interface) 

    Open questions:
    * Do we need CONNECTORS? In software these are just components themself ... In Hardware they nicely encode the concept of a BUS
      For now, it is left as a design decision ... you would only use connector components if they have work to do, e.g. convert between interface types
*/

class Network;

class Network : public CommonConceptGraph
{
    public:
        // Ids for identifiing main concepts
        static const UniqueId ComponentId;
        static const UniqueId InterfaceId;
        static const UniqueId NetworkId;
        static const UniqueId ValueId;
        // Ids for identifiing main relations
        static const UniqueId HasAInterfaceId;
        static const UniqueId ConnectedToInterfaceId;
        static const UniqueId PartOfNetworkId;
        static const UniqueId AliasOfId;
        static const UniqueId HasAValueId;

        // Constructor/Destructor
        Network();
        Network(const Hypergraph& A);
        ~Network();

        // Creates the main concepts
        void createMainConcepts();

        // Create classes
        Hyperedges createComponent(const UniqueId& uid, const std::string& name="Component", const Hyperedges& suids=Hyperedges{ComponentId});
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges{InterfaceId});
        Hyperedges createNetwork(const UniqueId& uid, const std::string& name="Network", const Hyperedges& suids=Hyperedges{NetworkId});
        Hyperedges createValue(const UniqueId& uid, const std::string& name="Value", const Hyperedges& suids=Hyperedges{ValueId});
        // Create individuals
        Hyperedges instantiateComponent(const Hyperedges& componentIds);
        Hyperedges instantiateComponent(const Hyperedges& componentIds, const std::string& newName);
        Hyperedges instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name="");
        // Create alias interfaces
        Hyperedges instantiateAliasInterfaceFor(const Hyperedges& parentUids, const Hyperedges& interfaceUids, const std::string& label="");
        // Create a value for an interface
        Hyperedges instantiateValueFor(const Hyperedges& interfaceUids, const Hyperedges& valueClassUids, const std::string& value="");

        // Query classes
        Hyperedges componentClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ComponentId}) const;
        Hyperedges interfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{InterfaceId}) const;
        Hyperedges networkClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{NetworkId}) const;
        Hyperedges valueClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ValueId}) const;
        // Query individuals
        Hyperedges components(const std::string& name="", const std::string& className="") const;
        Hyperedges interfaces(const std::string& name="", const std::string& className="") const;
        Hyperedges networks(const std::string& name="", const std::string& className="") const;
        Hyperedges values(const std::string& name="", const std::string& className="") const;
        // Query component interfaces
        Hyperedges interfacesOf(const Hyperedges& componentIds, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query original interfaces of alias interfaces
        Hyperedges originalInterfacesOf(const Hyperedges& aliasInterfaceUids, const std::string& label="") const;
        // Query the values of an interface
        Hyperedges valuesOf(const Hyperedges& interfaceUids, const std::string& value="") const;

        // Specify a component
        Hyperedges hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds);
        // Connect interfaces
        Hyperedges connectInterface(const Hyperedges& fromInterfaceIds, const Hyperedges& toInterfaceIds);
        // Specify network
        Hyperedges partOfNetwork(const Hyperedges& componentIds, const Hyperedges& networkIds);
        // Specify that an interface is an alias of another interface
        Hyperedges aliasOf(const Hyperedges& aliasInterfaceUids, const Hyperedges& originalInterfaceUids);
        // Assign values to interfaces
        Hyperedges hasValue(const Hyperedges& interfaceIds, const Hyperedges& valueIds);
};

}

#endif

