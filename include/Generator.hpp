#ifndef _SOFTWARE_GENERATOR_HPP
#define _SOFTWARE_GENERATOR_HPP

#include "SoftwareNetwork.hpp"

namespace Software {

/*
    GENERATOR CLASS

    This class can be used to generate implementations for abstract algorithms.
    It is a base class from which concrete, language specific generators can be derived.

    A generator is supposed to support one specific programming language to generator valid implementation skeletons.
    For this to work, it has to:
    * go through any abstract interface and produce a concrete one (this is not possible without user interaction or some predefined model)
    * produce a 'header-only' code which will be stored in the label of the implementation which respects the previously defined type system.

    This generator class per default generates pure, header-only C++ code.
*/

class GenerateConcreteInterface {
    public:

        /* This function shall return a language specific interface type for an abstract type */
        virtual std::string askForConcreteInterfaceLabel(const std::string& abstractInterfaceLabel) const;
};

class Generator : public Network {
    public:
        /*
            The constructor takes a graph to work on, a interface uid to group its concrete interfaces and an implementation uid to group its generated implementations
        */
        Generator(const Network& net, const UniqueId& interfaceUid="Software::Generator::C++::Interface", const UniqueId& implementationUid="Software::Generator::C++::Implementation");

        /* Queries */
        Hyperedges concreteInterfaceClasses() const;
        Hyperedges concreteImplementationClasses() const;

        /* This function actually generates language specific implementation code stored in the label. It needs some decision function to determine language specific type of the interface */
        virtual Hyperedges generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const UniqueId& concreteInterfaceClassUid, const GenerateConcreteInterface& decider=GenerateConcreteInterface());

        /* This function actually generates language specific implementation code stored in the label */
        virtual Hyperedges generateImplementationClassFor(const UniqueId& algorithmClassUid, const UniqueId& concreteImplementationClassUid);

    protected:
        UniqueId ifClassUid;
        UniqueId implClassUid;
};

}

#endif
