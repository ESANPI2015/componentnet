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
        ALGORITHM, INPUT, OUTPUT, INTERFACE (for logical specification and skeleton generation)
        IMPLEMENTATION, DATATYPE (for storing already available implementations of algorithms in different languages)
    The domain is encoded as follows:

    (ALGORITHM -- has --> INTERFACE)                    optional, inferrable
    INPUT -- is-a --> INTERFACE
    OUTPUT -- is-a --> INTERFACE
    ALGORITHM -- needs(has) --> INPUT
    ALGORITHM -- provides(has) --> OUTPUT
    INPUT -- dependsOn(connects) --> OUTPUT

    ALGORITHM <-- realizes(is-a) -- IMPLEMENTATION
    INTERFACE <-- represents(is-a) -- DATATYPE

    (IMPLEMENTATION -- uses --> DATATYPE)               questionable
    (IMPLEMENTATION -- dependsOn --> IMPLEMENTATION)    optional, not needed yet
    (IMPLEMENTATION -- needs --> INPUT)                 redundant
    (IMPLEMENTATION -- provides --> OUTPUT)             redundant

    If some X is a ALGORITHM then there exists a path of IS-A relations from X to ALGORITHM

    Some example:

        |---------- realizes -- disparity.c
        v
    DisparityMap -- needs --> left -- is-a --> Input
      | |                       |---- is-a --> Image <-- represents -- uint8_t[MAX_X][MAX_Y]
      | |---------- needs --> right ...
      |------------ provides --> disparity ...

    NOTE: When merging with the concept of Finite State Machines, this whole thing would become an even whiter box :)
    NOTE: To support different programming languages, you can create a domains.
*/

class Graph : public Component::Network
{
    public:
        // Identifiers for the main concepts
        static const UniqueId AlgorithmId;
        static const UniqueId InterfaceId;
        static const UniqueId InputId;
        static const UniqueId OutputId;
        static const UniqueId ImplementationId;
        static const UniqueId DatatypeId;

        // Ids for identifiing main relations
        static const UniqueId NeedsId;
        static const UniqueId ProvidesId;
        static const UniqueId DependsOnId;

        // Constructor/Destructor
        Graph();
        Graph(const Hypergraph& A);
        ~Graph();

        // Generates the dictionary
        void createMainConcepts();

        // Factory functions
        // NOTE: Create classes
        Hyperedges createAlgorithm(const UniqueId& uid, const std::string& name="Algorithm", const Hyperedges& suids=Hyperedges());
        Hyperedges createInterface(const UniqueId& uid, const std::string& name="Interface", const Hyperedges& suids=Hyperedges());
        Hyperedges createInput(const UniqueId& uid, const std::string& name="Input", const Hyperedges& suids=Hyperedges());
        Hyperedges createOutput(const UniqueId& uid, const std::string& name="Output", const Hyperedges& suids=Hyperedges());
        Hyperedges createImplementation(const UniqueId& uid, const std::string& name="Implementation", const Hyperedges& suids=Hyperedges());
        Hyperedges createDatatype(const UniqueId& uid, const std::string& name="DataType", const Hyperedges& suids=Hyperedges()); // TODO: Needed? Why dont we just subclass interface?

        // Queries
        // NOTE: Returns subclasses
        Hyperedges algorithmClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());
        Hyperedges interfaceClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());
        Hyperedges inputClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());
        Hyperedges outputClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());
        Hyperedges implementationClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());
        Hyperedges datatypeClasses(const std::string& name="", const Hyperedges& suids=Hyperedges());

        // NOTE: Returns instances
        Hyperedges algorithms(const std::string& name="", const std::string& className="");
        Hyperedges interfaces(const std::string& name="", const std::string& className="");
        Hyperedges inputs(const std::string& name="", const std::string& className="");
        Hyperedges outputs(const std::string& name="", const std::string& className="");
        Hyperedges implementations(const std::string& name="", const std::string& className="");
        Hyperedges datatypes(const std::string& name="", const std::string& className="");

        // TODO: Nice additional queries
        // inputsOf()
        // outputsOf()
        // datatypesOf()

        // Facts
        // NOTE: Only the multidimensionals are used here (more generic)
        // Algorithms & I/O/P
        // RULE: A has X -> X is-a Interface
        // RULE: A provides O -> A has O, O is-a Output (, O is-a Interface)
        // RULE: A needs I -> A has I, I is-a Input (, I is-a Interface)
        Hyperedges providesInterface(const Hyperedges& algorithmIds, const Hyperedges& outputIds);
        Hyperedges needsInterface(const Hyperedges& algorithmIds, const Hyperedges& inputIds);
        // I/O & Dependencies
        // RULE: I dependsOn O -> I is-a Input, O is-a Output
        Hyperedges dependsOn(const Hyperedges& inputIds, const Hyperedges& outputIds);
};

}

#endif
