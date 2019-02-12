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
    INTERFACE <-- HAS-A --> VALUE (used to assign a constant/initial value to an interface) 
    INTERFACE <-- PART-OF --> INTERFACE
    COMPONENT <-- PART-OF --> COMPONENT

*/

class Network;

class Network : public CommonConceptGraph
{
    public:
        // Ids for identifiing main concepts
        static const UniqueId ComponentId;
        static const UniqueId InterfaceId;
        static const UniqueId ValueId;

        // Ids for identifiing main relations
        static const UniqueId HasAInterfaceId;
        static const UniqueId ConnectedToInterfaceId;
        static const UniqueId AliasOfId;
        static const UniqueId HasAValueId;
        static const UniqueId PartOfComponentId;
        static const UniqueId PartOfInterfaceId;

        // Constructor/Destructor
        Network();
        Network(const Hypergraph& A);
        ~Network();

        // Creates the main concepts
        void createMainConcepts();

        // Create classes
        Hyperedges createComponent(const UniqueId& uid, const std::string& name="Component", const Hyperedges& suids=Hyperedges{ComponentId});
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges{InterfaceId});
        Hyperedges createValue(const UniqueId& uid, const std::string& name="Value", const Hyperedges& suids=Hyperedges{ValueId});

        // Create individuals
        Hyperedges instantiateComponent(const Hyperedges& componentIds, const std::string& newName="");
        Hyperedges instantiateInterfaceFor(const Hyperedges& componentIds, const Hyperedges& interfaceClassIds, const std::string& name="");
        Hyperedges instantiateAliasInterfaceFor(const Hyperedges& parentUids, const Hyperedges& interfaceUids, const std::string& label="");
        Hyperedges instantiateValueFor(const Hyperedges& interfaceUids, const Hyperedges& valueClassUids, const std::string& value="");

        // Query classes
        Hyperedges componentClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ComponentId}) const;
        Hyperedges interfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{InterfaceId}) const;
        Hyperedges valueClasses(const std::string& name="", const Hyperedges& suids=Hyperedges{ValueId}) const;

        // Query individuals
        Hyperedges components(const std::string& name="", const std::string& className="") const;
        Hyperedges interfaces(const std::string& name="", const std::string& className="") const;
        Hyperedges values(const std::string& name="", const std::string& className="") const;

        // Query component interfaces
        Hyperedges interfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query original interfaces of alias interfaces
        Hyperedges originalInterfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        // Query the values of an interface
        Hyperedges valuesOf(const Hyperedges& uids, const std::string& value="", const TraversalDirection dir=FORWARD) const;
        // Query the subinterfaces of an interface
        Hyperedges subinterfacesOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;
        // Query the subcomponents of a component
        Hyperedges subcomponentsOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;

        // Specify a component's interfaces
        Hyperedges hasInterface(const Hyperedges& componentIds, const Hyperedges& interfaceIds);
        // Specifiy a (composite) component
        Hyperedges partOfComponent(const Hyperedges& componentIds, const Hyperedges& compositeComponentIds);
        // Specify a (composite) interface
        Hyperedges partOfInterface(const Hyperedges& interfaceIds, const Hyperedges& compositeInterfaceIds);
        // Connect interfaces
        Hyperedges connectInterface(const Hyperedges& fromInterfaceIds, const Hyperedges& toInterfaceIds);
        // Specify that an interface is an alias of another interface
        Hyperedges aliasOf(const Hyperedges& aliasInterfaceUids, const Hyperedges& originalInterfaceUids);
        // Assign values to interfaces
        Hyperedges hasValue(const Hyperedges& interfaceIds, const Hyperedges& valueIds);
};

}

#endif

