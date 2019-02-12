#include "Generator.hpp"
#include <iostream>

namespace Software {

std::string GenerateConcreteInterface::askForConcreteInterfaceLabel(const std::string& abstractInterfaceLabel) const
{
   std::string concreteInterfaceLabel;
   std::cout << "Please provide a C++ type definition for " << abstractInterfaceLabel << ":";
   std::cin >> concreteInterfaceLabel;
   return concreteInterfaceLabel;
}

Generator::Generator(const Network& net, const UniqueId& interfaceUid, const UniqueId& implementationUid)
: ifClassUid(interfaceUid), implClassUid(implementationUid)
{
    importFrom(net);
    createImplementation(implClassUid);
    createInterface(ifClassUid);
}

Hyperedges Generator::concreteInterfaceClasses() const
{
    return subclassesOf(ifClassUid);
}

Hyperedges Generator::concreteImplementationClasses() const
{
    return subclassesOf(implClassUid);
}

Hyperedges Generator::generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const UniqueId& concreteInterfaceClassUid, const GenerateConcreteInterface& decider)
{
    // Here, we generate a C++ class for a given interface
    // The template looks like this:
    // class <AbstractInterfaceClass.label> : public <AbstractInterfaceSuperclass.label>, ...
    // {
    //      public:
    //          const T& read() const;
    //          void write(const T& newValue);
    //      protected:
    //          T currentValue;
    // };

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges _abstractIfSuperclassUids(directSubclassesOf(Hyperedges{abstractInterfaceClassUid}, "", Hypergraph::TraversalDirection::FORWARD));
    std::string name(access(abstractInterfaceClassUid).label());
    std::string type(decider.askForConcreteInterfaceLabel(name));

    result << "#ifndef _INTERFACE_" << name << "_IMPL\n";
    result << "#define _INTERFACE_" << name << "_IMPL\n";

    // Collect all valid information
    Hyperedges abstractIfSuperclassUids;
    Hyperedges validConcreteIfSuperclassUids;
    for (const UniqueId& abstractIfSuperclassUid : _abstractIfSuperclassUids)
    {
        // Get the concrete interface class
        Hyperedges concreteIfSuperclassUids(intersect(validConcreteInterfaceClassUids, encodersOf(Hyperedges{abstractIfSuperclassUid})));
        // If none exists, ignore? Or generate it?
        if (concreteIfSuperclassUids.empty())
            continue;
        // Copy code in here
        for (const UniqueId& concreteIfSuperclassUid : concreteIfSuperclassUids)
        {
            result << access(concreteIfSuperclassUid).label();
        }
        abstractIfSuperclassUids.push_back(abstractIfSuperclassUid);
        validConcreteIfSuperclassUids = unite(validConcreteIfSuperclassUids, concreteIfSuperclassUids);
    }

    // CLASS DEF
    if (abstractIfSuperclassUids.empty())
    {
        result << "class " << name;
    } else {
        result << "class " << name << " : ";
        Hyperedges::const_iterator it(abstractIfSuperclassUids.begin());
        while (it != abstractIfSuperclassUids.end())
        {
            result << "public " << access(*it).label();
            it++;
            if (it != abstractIfSuperclassUids.end())
                result << ", ";
        }
    }
    result << "\n{\n";
    result << "\tpublic:\n";
    result << "\t\tconst " << type << "& read() const\n";
    result << "\t\t{\n";
    result << "\t\t\t return currentValue;\n";
    result << "\t\t}\n";
    result << "\t\tvoid write(const " << type << "& newValue)\n";
    result << "\t\t{\n";
    result << "\t\t\t currentValue = newValue;\n";
    result << "\t\t}\n";
    result << "\tprotected:\n";
    result << "\t\t" << type << " currentValue;\n";
    result << "\n};\n";

    result << "#endif\n";

    Hyperedges newInterfaceClassUid(createInterface(concreteInterfaceClassUid, result.str(), Hyperedges{ifClassUid}));
    encodes(newInterfaceClassUid, Hyperedges{abstractInterfaceClassUid});
    isA(newInterfaceClassUid, validConcreteIfSuperclassUids);
    return newInterfaceClassUid;
}


Hyperedges Generator::generateImplementationClassFor(const UniqueId& algorithmClassUid, const UniqueId& concreteImplementationClassUid)
{
    // Here, we generate a C++ class for a given implementation class
    // The template looks like this:
    // class <AlgorithmClass.label> : public <AlgorithmSuperclass.label>, ...
    // {
    //      public:
    //          AlgorithmClass.label();
    //          bool operator();
    //          InterfaceClass interfaceName;
    //          ...
    //      protected:
    //          ImplementationClass partName;
    //          ...
    // };

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges validImplementationClassUids(concreteImplementationClasses());
    Hyperedges _algorithmSuperclassUids(directSubclassesOf(Hyperedges{algorithmClassUid}, "", FORWARD));

    std::string name(access(algorithmClassUid).label());

    result << "#ifndef _ALGORITHM_" << name << "_IMPL\n";
    result << "#define _ALGORITHM_" << name << "_IMPL\n";

    // Collect all valid superclass info
    Hyperedges algorithmSuperclassUids;
    Hyperedges myImplementationSuperclassUids;
    for (const UniqueId& algorithmSuperclassUid : _algorithmSuperclassUids)
    {
        // Get concrete implementation
        Hyperedges implementationSuperclassUids(intersect(validImplementationClassUids, implementationsOf(Hyperedges{algorithmSuperclassUid})));
        // If none exists, ignore it
        if (implementationSuperclassUids.empty())
            continue;
        // Otherwise, copy the code in here
        for (const UniqueId& implementationSuperclassUid : implementationSuperclassUids)
        {
            result << access(implementationSuperclassUid).label();
        }
        algorithmSuperclassUids.push_back(algorithmSuperclassUid);
        myImplementationSuperclassUids = unite(myImplementationSuperclassUids, implementationSuperclassUids);
    }

    // Collect all interface info (and instantiate concrete interfaces)
    Hyperedges _myAbstractInterfaceUids(interfacesOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractInterfaceUids;
    Hyperedges myConcreteInterfaceUids;
    for (const UniqueId& myAbstractInterfaceUid : _myAbstractInterfaceUids)
    {
        // Get concrete interface
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        Hyperedges concreteInterfaceClassUids(intersect(validConcreteInterfaceClassUids, encodersOf(Hyperedges{myAbstractInterfaceClassUids})));
        // If none exists, ignore it
        if (concreteInterfaceClassUids.empty())
            continue;
        // Otherwise, copy the code in here
        for (const UniqueId& concreteInterfaceClassUid : concreteInterfaceClassUids)
        {
            result << access(concreteInterfaceClassUid).label();
        }
        myAbstractInterfaceUids.push_back(myAbstractInterfaceUid);
        myConcreteInterfaceUids = unite(myConcreteInterfaceUids, instantiateFrom(concreteInterfaceClassUids, access(myAbstractInterfaceUid).label()));
    }

    // ... and of our parts (instantiate them as well)
    Hyperedges _myAbstractPartUids(componentsOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractPartUids;
    Hyperedges myImplementationPartUids;
    for (const UniqueId& myAbstractPartUid : _myAbstractPartUids)
    {
        // Get concrete implementation from myAbstractPartUid superclass
        Hyperedges myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        Hyperedges implementationClassUids(intersect(validImplementationClassUids, implementationsOf(myAbstractPartClassUids)));
        // If none given, ignore
        if (implementationClassUids.empty())
            continue;
        // Copy code in here
        for (const UniqueId& implementationClassUid : implementationClassUids)
        {
            result << access(implementationClassUid).label();
        }
        myAbstractPartUids.push_back(myAbstractPartUid);
        myImplementationPartUids = unite(myImplementationPartUids, instantiateComponent(implementationClassUids, access(myAbstractPartUid).label()));
    }

    if (algorithmSuperclassUids.empty())
    {
        result << "class " << name;
    } else {
        result << "class " << name << " : ";
        Hyperedges::const_iterator it(algorithmSuperclassUids.begin());
        while (it != algorithmSuperclassUids.end())
        {
            result << "public " << access(*it).label();
            it++;
            if (it != algorithmSuperclassUids.end())
                result << ", ";
        }
    }
    result << "\n{\n";
    result << "\tpublic:\n";

    // Constructor
    result << "\t\t" << name << "()\n";
    result << "\t\t{\n";
    // Initialize interfaces
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Hyperedges myAbstractInterfaceValueUids(valuesOf(Hyperedges{myAbstractInterfaceUid}));
        for (const UniqueId& myAbstractInterfaceValueUid : myAbstractInterfaceValueUids)
        {
            // FIXME: Currently we just pass the values in ... but what if they need to be converted first?
            result << "\t\t\t";
            result << access(myAbstractInterfaceUid).label() << ".write(";
            result << access(myAbstractInterfaceValueUid).label();
            result << ");\n";
        }
    }
    // TODO: Initialize interfaces of parts
    result << "\t\t\t// Initialization code starts here\n";
    result << "\t\t}\n";

    // Evaluation function
    result << "\t\tbool operator() ()\n";
    result << "\t\t{\n";
    // Read in external inputs
    Hyperedges myAbstractInputUids(inputsOf(Hyperedges{algorithmClassUid}));
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        std::cout << access(myAbstractInputUid).label(); //OK
        // Pass external inputs to internal inputs
        Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractInputUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            std::cout << "\t" << access(myOriginalAbstractInputUid).label(); //OK
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                std::cout << "\t" << access(myAbstractInternalPartUid).label() << "\n"; // NOK
                result << "\t\t\t";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << ".write(";
                result << access(myAbstractInputUid).label() << ".read()";
                result << ");\n";
            }
        }
    }
    std::cout << "\n";

    // Evaluate parts
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        result << "\t\t\t" << access(myAbstractPartUid).label() << "();\n";
    }
    result << "\t\t\t// Implementation starts here\n";
    // Pass outputs of internal parts to inputs of connected internal parts
    for (const UniqueId& myAbstractProducerUid : myAbstractPartUids)
    {
        Hyperedges internalAbstractOutputUids(outputsOf(Hyperedges{myAbstractProducerUid}));
        for (const UniqueId& internalAbstractOutputUid : internalAbstractOutputUids)
        {
            Hyperedges internalAbstractInputUids(endpointsOf(Hyperedges{internalAbstractOutputUid}));
            for (const UniqueId& internalAbstractInputUid : internalAbstractInputUids)
            {
                // TODO: Construct inputsOf with INVERSE direction!
                Hyperedges myAbstractConsumerUids(interfacesOf(Hyperedges{internalAbstractInputUid}, "", INVERSE));
                for (const UniqueId& myAbstractConsumerUid : myAbstractConsumerUids)
                {
                    result << "\t\t\t";
                    result << access(myAbstractConsumerUid).label() << "." << access(internalAbstractInputUid).label() << ".write(";
                    result << access(myAbstractProducerUid).label() << "." << access(internalAbstractOutputUid).label() << ".read()";
                    result << ");\n";
                }
            }
        }
    }
    // Pass internal outputs to external outputs
    Hyperedges myAbstractOutputUids(outputsOf(Hyperedges{algorithmClassUid}));
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractOutputUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t\t";
                result << access(myAbstractOutputUid).label() << ".write(";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label() << ".read()";
                result << ");\n";
            }
        }
    }
    result << "\t\t}\n";

    // Instantiate interfaces
    // NOTE: myAbstractInterfaceUid holds the name of the instance, myAbstractInterfaceClassUid holds the name of the class
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            result << "\t\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractInterfaceUid).label() << ";\n";
        }
    }

    result << "\tprotected:\n";

    // Instantiate parts
    // NOTE: myAbstractPartUid holds the name of the instance, myAbstractPartClassUid holds the name of the class
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        Hyperedges myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        for (const UniqueId& myAbstractPartClassUid : myAbstractPartClassUids)
        {
            result << "\t\t" << access(myAbstractPartClassUid).label() << " " << access(myAbstractPartUid).label() << ";\n";
        }
    }

    result << "\n};\n";
    result << "#endif\n";

    Hyperedges newImplementationClassUid(createImplementation(concreteImplementationClassUid, result.str(), Hyperedges{implClassUid}));
    implements(newImplementationClassUid, Hyperedges{algorithmClassUid});
    isA(newImplementationClassUid, myImplementationSuperclassUids);
    partOf(myImplementationPartUids, newImplementationClassUid);

    // For interfaces we have to setup the correct relations, depending on whether something is an input or an output or neither of them
    for (const UniqueId& myConcreteInterfaceUid : myConcreteInterfaceUids)
    {
        for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
        {
            if (access(myAbstractInputUid).label() == access(myConcreteInterfaceUid).label())
                needsInterface(newImplementationClassUid, Hyperedges{myConcreteInterfaceUid});
        }
        for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
        {
            if (access(myAbstractOutputUid).label() == access(myConcreteInterfaceUid).label())
                providesInterface(newImplementationClassUid, Hyperedges{myConcreteInterfaceUid});
        }
        hasInterface(newImplementationClassUid, Hyperedges{myConcreteInterfaceUid});
    }
    return newImplementationClassUid;
}

}
