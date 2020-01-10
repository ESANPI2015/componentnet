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
    INTERFACE <-- ALIAS-OF --> INTERFACE  (used for renaming interfaces)
    INTERFACE <-- HAS-A --> INTERFACE
    COMPONENT <-- PART-OF --> COMPONENT

    2020-01-10: Values of interfaces are now stored as properties of an interface

*/

class Network;

class Network : public CommonConceptGraph
{
    public:
        // Ids for identifiing main concepts
        static const UniqueId ComponentId;
        static const UniqueId InterfaceId;

        // Ids for identifiing main relations
        static const UniqueId HasAInterfaceId;
        static const UniqueId ConnectedToInterfaceId;
        static const UniqueId AliasOfId;
        static const UniqueId PartOfComponentId;
        static const UniqueId HasASubInterfaceId;

        // Constructor/Destructor
        Network();
        Network(const Hypergraph& A);
        ~Network();

        // Creates the main concepts
        void createMainConcepts();

        // Create classes
        Hyperedges createComponent(const UniqueId& uid, const std::string& name="Component", const Hyperedges& suids=Hyperedges{ComponentId});
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges{InterfaceId});

        // Create individuals
        Hyperedges instantiateComponent(const Hyperedges& componentIds, const std::string& newName="");
        Hyperedges instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name="");
        Hyperedges instantiateAliasInterfaceFor(const Hyperedges& parentUids, const Hyperedges& interfaceUids, const std::string& label="");

        // Query classes
        Hyperedges componentClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ComponentId}) const;
        Hyperedges interfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{InterfaceId}) const;

        // Query individuals
        Hyperedges components(const std::string& name="", const std::string& className="") const;
        Hyperedges interfaces(const std::string& name="", const std::string& className="") const;

        // Query component interfaces
        Hyperedges interfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query original interfaces of alias interfaces
        Hyperedges originalInterfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query the subinterfaces of an interface
        Hyperedges subinterfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query the subcomponents of a component
        Hyperedges subcomponentsOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;

        // Specify a component's interfaces
        Hyperedges hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds);
        // Specifiy a (composite) component
        Hyperedges partOfComponent(const Hyperedges& componentIds, const Hyperedges& compositeComponentIds);
        // Specify a (composite) interface
        Hyperedges hasSubInterface(const Hyperedges& interfaceIds, const Hyperedges& subInterfaceIds);
        // Connect interfaces
        Hyperedges connectInterface(const Hyperedges& fromInterfaceIds, const Hyperedges& toInterfaceIds);
        // Specify that an interface is an alias of another interface
        Hyperedges aliasOf(const Hyperedges& aliasInterfaceUids, const Hyperedges& originalInterfaceUids);
};

}

#endif

