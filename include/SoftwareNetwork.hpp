#ifndef _SOFTWARE_GRAPH_HPP
#define _SOFTWARE_GRAPH_HPP

#include "ComponentNetwork.hpp"

namespace Software {

/*
    Some notes:
    The underlying fundament is the concept graph.
    In this environment there are two main entities, concepts & relations between them.
    This class is derived from the concept graph class.

    It introduces these different concepts:
        ALGORITHM, INTERFACE (for logical specification and skeleton generation)
        IMPLEMENTATION (for storing already available implementations of algorithms in different languages)
    The domain is encoded as follows:

    (ALGORITHM -- has --> INTERFACE)                    optional, inferrable
    ALGORITHM -- needs(has) --> INTERFACE               encodes necessary information (inputs)
    ALGORITHM -- provides(has) --> INTERFACE            encodes results of processing (outputs)
    INTERFACE -- dependsOn(connects) --> INTERFACE      encodes information flow from one interface to another

    Although IMPLEMENTATIONS are themselves ALGORITHMS, a specific IMPLEMENTATION implements an ALGORITHM while not inheriting its INTERFACES.
    This is NOT an IS-A relation!
    The same is also true for interfaces.
    A (concrete) INTERFACE encodes the information of an (abstract) INTERFACE
    The reason for the extra relation is to cleanly decouple implementations and abstract algorithms.

    ALGORITHM <-- implements -- IMPLEMENTATION
    (abstract) INTERFACE <-- encodes -- (concrete) INTERFACE

    If some X is a ALGORITHM then there exists a path of IS-A relations from X to ALGORITHM

    Some example:

        |---------- implements -- disparity.c
        v
    DisparityMap -- needs --> left -- instanceOf --> Image
      | |                      
      | |---------- needs --> right ...
      |------------ provides --> disparity ...

    disparity.c -- needs --> left -- instanceOf --> uint8[MAX_X][MAX_Y] -- encodes --> Image

    NOTE: When merging with the concept of Finite State Machines, this whole thing would become an even whiter box :)
    NOTE: To support different programming languages, you can create e.g. specialized interfaces by ENCODES relations.
    NOTE: IMPLEMENTS denotes the possibility to use a IMPLEMENTATION CLASS for some ALGOITHM CLASS. REALIZES denotes that a specific IMPLEMENTATION INSTANCE has been chosen to realize an ALGORITHM INSTANCE.

    Open questions:
    * If we have specified NEEDS and PROVIDES ... do we need to specify INPUTS & OUTPUTS seperately? Isn't this redundant?
    * Shall we split ALGORITHM domain and IMPLEMENTATION domain into two classes?
    * Shall we drop the notion of input and output? Makes everything more complicated
*/

class Network : public Component::Network
{
    public:
        // Identifiers for the algorithmic concepts
        static const UniqueId AlgorithmId;
        static const UniqueId InterfaceId;

        // Identifiers for implementation specific concepts
        static const UniqueId ImplementationId;
        static const UniqueId ImplementationInterfaceId;

        // Ids for identifiing main relations
        static const UniqueId NeedsId;
        static const UniqueId ProvidesId;
        static const UniqueId DependsOnId;
        static const UniqueId ImplementsId;
        static const UniqueId EncodesId;
        static const UniqueId RealizesId;

        // Constructor/Destructor
        Network();
        Network(const Hypergraph& A);
        ~Network();

        // Generates the dictionary
        void createMainConcepts();

        // Factory functions
        // NOTE: Create classes
        Hyperedges createAlgorithm(const UniqueId& uid, const std::string& name="Algorithm", const Hyperedges& suids=Hyperedges());
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges());
        Hyperedges createImplementation(const UniqueId& uid, const std::string& name="Implementation", const Hyperedges& suids=Hyperedges());
        Hyperedges createImplementationInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges());

        // Queries
        // NOTE: Returns subclasses
        Hyperedges algorithmClasses(const std::string& name="", const Hyperedges& suids=Hyperedges()) const;
        Hyperedges interfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges()) const;
        Hyperedges implementationClasses(const std::string& name="", const Hyperedges& suids=Hyperedges()) const;
        Hyperedges implementationInterfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges()) const;

        // NOTE: Returns instances
        Hyperedges algorithms(const std::string& name="", const std::string& className="") const;
        Hyperedges interfaces(const std::string& name="", const std::string& className="") const;
        Hyperedges implementations(const std::string& name="", const std::string& className="") const;
        Hyperedges implementationInterfaces(const std::string& name="", const std::string& className="") const;

        // Special additional queries
        Hyperedges inputsOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        Hyperedges outputsOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=FORWARD) const;
        Hyperedges implementationsOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;
        Hyperedges encodersOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;
        Hyperedges realizersOf(const Hyperedges& uids, const std::string& name="", const TraversalDirection dir=INVERSE) const;

        // Facts
        // NOTE: Only the multidimensionals are used here (more generic)
        // Algorithms/Implementations & I/O/P
        Hyperedges providesInterface(const Hyperedges& algorithmIds, const Hyperedges& outputIds);
        Hyperedges needsInterface(const Hyperedges& algorithmIds, const Hyperedges& inputIds);
        // I/O & Dependencies
        Hyperedges dependsOn(const Hyperedges& inputIds, const Hyperedges& outputIds);

        // Concrete Implementation/Interfaces to abstract Algorithm/Interfaces facts
        Hyperedges implements(const Hyperedges& implementationIds, const Hyperedges& algorithmIds);
        Hyperedges encodes(const Hyperedges& concreteInterfaceIds, const Hyperedges& interfaceIds);
        Hyperedges realizes(const Hyperedges& implementationIds, const Hyperedges& algorithmIds);

        // Special function to find all possible implementation networks from an algorithm network
        std::vector< Software::Network > generateAllImplementationNetworks() const;
};

}

#endif
