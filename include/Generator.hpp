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

class GeneratorHook {
    public:
        /*Hint to the hook function about the nature of the question (for automatic generation)*/
        enum QuestionType {
            QUESTION_GENERATE_IFCLASS = 0,
            QUESTION_PROVIDE_UID = 1,
            QUESTION_USE_IFCLASS = 2,
            QUESTION_PROVIDE_PLAIN_TYPE = 3,
            QUESTION_GENERATE_IMPLCLASS = 4,
            QUESTION_USE_IMPLCLASS = 5,
            QUESTION_PROVIDE_CODE = 6,
            QUESTION_GENERAL = 7
        };
        virtual std::string ask(const std::string& question, const enum QuestionType& type=QUESTION_GENERAL) const;
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
        virtual Hyperedges generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const GeneratorHook& hook = GeneratorHook());

        /* This function actually generates language specific implementation code stored in the label */
        virtual Hyperedges generateImplementationClassFor(const UniqueId& algorithmClassUid, const GeneratorHook& hook = GeneratorHook());

    protected:
        UniqueId ifClassUid;
        UniqueId implClassUid;
};

}

#endif
