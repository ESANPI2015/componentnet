#include "Generator.hpp"
#include "VHDLGenerator.hpp"
#include "HypergraphYAML.hpp"

#include <fstream>
#include <iostream>
#include <limits>

int main (void)
{
    std::cout << "*** Testing C++ Generator ***\n";
    std::cout << "Create a simple algorithm\n";

    Software::Network net;
    Software::Generator gen(net);

    gen.createAlgorithm("SimpleAlgorithm", "F");
    gen.createInterface("ARealNumber", "Real");
    gen.needsInterface(Hyperedges{"SimpleAlgorithm"}, gen.instantiateFrom("ARealNumber", "x"));
    gen.providesInterface(Hyperedges{"SimpleAlgorithm"}, gen.instantiateFrom("ARealNumber", "y"));

    Hyperedges generatedStuff;
    generatedStuff = unite(generatedStuff, gen.generateConcreteInterfaceClassFor("ARealNumber", "ConcreteRealNumber"));
    generatedStuff = unite(generatedStuff, gen.generateImplementationClassFor("SimpleAlgorithm","SimpleImplementation"));

    std::cout << "Generated code:\n";
    for (const UniqueId& uid : generatedStuff)
        std::cout << gen.access(uid).label();

    std::cout << "Create a simple nested algorithm\n";
    
    gen.createAlgorithm("SimpleNestedAlgorithm", "G");
    gen.needsInterface(Hyperedges{"SimpleNestedAlgorithm"}, gen.instantiateFrom("ARealNumber", "a"));
    gen.providesInterface(Hyperedges{"SimpleNestedAlgorithm"}, gen.instantiateFrom("ARealNumber", "b"));

    Hyperedges innerPart(gen.instantiateComponent(Hyperedges{"SimpleAlgorithm"}, "f"));
    gen.partOfComponent(innerPart, Hyperedges{"SimpleNestedAlgorithm"});
    gen.aliasOf(gen.inputsOf(Hyperedges{"SimpleNestedAlgorithm"}, "a"), gen.inputsOf(innerPart, "x")); // NOK
    gen.aliasOf(gen.outputsOf(Hyperedges{"SimpleNestedAlgorithm"}, "b"), gen.outputsOf(innerPart, "y"));

    generatedStuff = unite(generatedStuff, gen.generateImplementationClassFor("SimpleNestedAlgorithm", "SimpleNestedImplementation"));
    std::cout << "Generated code:\n";
    for (const UniqueId& uid : generatedStuff)
        std::cout << gen.access(uid).label();

    std::cout << "Create a more complex nested algorithm\n";
    
    gen.createAlgorithm("NestedAlgorithm", "H");
    gen.needsInterface(Hyperedges{"NestedAlgorithm"}, gen.instantiateFrom("ARealNumber", "u"));
    gen.providesInterface(Hyperedges{"NestedAlgorithm"}, gen.instantiateFrom("ARealNumber", "v"));

    std::cout << "Place initial values on the interfaces\n";
    gen.createValue("SimpleValue", "0.f");
    gen.instantiateValueFor(gen.interfacesOf(Hyperedges{"NestedAlgorithm"}), Hyperedges{"SimpleValue"}, "0.f");

    Hyperedges innerPart2(gen.instantiateComponent(Hyperedges{"SimpleAlgorithm"}, "f"));
    Hyperedges innerPart3(gen.instantiateComponent(Hyperedges{"SimpleNestedAlgorithm"}, "g"));
    gen.partOfComponent(unite(innerPart2, innerPart3), Hyperedges{"NestedAlgorithm"});
    gen.dependsOn(gen.inputsOf(innerPart3, "a"), gen.outputsOf(innerPart2, "y"));
    gen.aliasOf(gen.inputsOf(Hyperedges{"NestedAlgorithm"}, "u"), gen.inputsOf(innerPart2, "x"));
    gen.aliasOf(gen.outputsOf(Hyperedges{"NestedAlgorithm"}, "v"), gen.outputsOf(innerPart3, "b"));

    generatedStuff = unite(generatedStuff, gen.generateImplementationClassFor("NestedAlgorithm", "NestedImplementation"));
    std::cout << "Generated code:\n";
    for (const UniqueId& uid : generatedStuff)
        std::cout << gen.access(uid).label();


    std::ofstream fout;
    fout.open("nested.hpp");
    Hyperedges impls(gen.implementationsOf(Hyperedges{"NestedAlgorithm"}));
    for (const UniqueId& impl : impls)
        fout << gen.access(impl).label();
    fout.close();

    std::cout << "*** Testing VHDL Generator ***\n";

    Software::VHDLGenerator vhdlGen(gen);

    vhdlGen.generateImplementationClassFor("SimpleAlgorithm", "SimpleVHDLImplementation");
    std::cout << vhdlGen.access("SimpleVHDLImplementation").label() << std::endl;

    vhdlGen.generateImplementationClassFor("NestedAlgorithm", "VHDLNestedImplementation");
    std::cout << vhdlGen.access("VHDLNestedImplementation").label() << std::endl;

    fout.open("generated.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(vhdlGen) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}
