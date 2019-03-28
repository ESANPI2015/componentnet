#ifndef _SOFTWARE_VHDL_GENERATOR_HPP
#define _SOFTWARE_VHDL_GENERATOR_HPP

#include "Generator.hpp"

namespace Software {

class VHDLGenerator : public Generator
{
    public:
        VHDLGenerator(const Network& net, const UniqueId& interfaceUid="Software::Generator::VHDL::Interface", const UniqueId& implementationUid="Software::Generator::VHDL::Implementation")
        : Generator(net, interfaceUid, implementationUid)
        {}

        virtual Hyperedges generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const UniqueId& concreteInterfaceClassUid, const GeneratorHook& hook = GeneratorHook());
        virtual Hyperedges generateImplementationClassFor(const UniqueId& algorithmClassUid, const UniqueId& concreteImplementationClassUid, const GeneratorHook& hook = GeneratorHook());
};

}

#endif
