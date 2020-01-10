#include "Generator.hpp"
#include <iostream>
#include <string>

namespace Software {

std::string GeneratorHook::ask(const std::string& question, const enum QuestionType& type) const
{
   std::string answer;
   std::cout << "[" << type << "] " << question << ": ";
   std::getline(std::cin, answer);
   return answer;
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

Hyperedges Generator::generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const GeneratorHook& hook)
{
    // Here, we generate a C++ class for a given interface
    // The template looks like this:
    // <include statements>
    // struct <AbstractInterfaceClass.label> : <AbstractInterfaceSuperclass.label>, ...
    // {
    //      public:
    //          <AbstractInterfacePartClass.label> partName;
    //          ...
    //          custom member variables
    // };
    std::stringstream code;
    const std::string& myName(access(abstractInterfaceClassUid).label());
    const UniqueId&    myImplementationUid(ifClassUid+"::"+abstractInterfaceClassUid); // unique + unique = unique
    // Check if implementation exists
    if (exists(myImplementationUid))
        return Hyperedges{myImplementationUid};

    // Before we generate code, we generate all superclass code first
    const Hyperedges&  myAbstractSuperclassUids(intersect(interfaceClasses(), directSubclassesOf(Hyperedges{abstractInterfaceClassUid}, "", FORWARD)));
    for (const UniqueId& myAbstractSuperclassUid : myAbstractSuperclassUids)
    {
        generateConcreteInterfaceClassFor(myAbstractSuperclassUid, hook);
    }
    
    // Before we generate code, we generate all subinterface code first
    const Hyperedges& myAbstractSubinterfaceUids(subinterfacesOf(Hyperedges{abstractInterfaceClassUid}));
    Hyperedges myAbstractSubinterfaceClassUids;
    Hyperedges myConcreteSubInterfaceUids;
    for (const UniqueId& myAbstractSubinterfaceUid : myAbstractSubinterfaceUids)
    {
        // Get classes
        const Hyperedges& _myAbstractSubinterfaceClassUids(instancesOf(Hyperedges{myAbstractSubinterfaceUid},"",FORWARD));
        for (const UniqueId& myAbstractSubinterfaceClassUid : _myAbstractSubinterfaceClassUids)
        {
            myConcreteSubInterfaceUids = unite(myConcreteSubInterfaceUids, instantiateFrom(generateConcreteInterfaceClassFor(myAbstractSubinterfaceClassUid, hook), access(myAbstractSubinterfaceUid).label()));
        }
        myAbstractSubinterfaceClassUids = unite(myAbstractSubinterfaceClassUids, _myAbstractSubinterfaceClassUids);
    }

    // Now we can be sure that all interfaces we depend on have at least one implementation
    
    code << "#ifndef _INTERFACE_" << myName << "_IMPLEMENTATION\n";
    code << "#define _INTERFACE_" << myName << "_IMPLEMENTATION\n";
    // Insert include statements of interfaces we depend on
    for (const UniqueId& myAbstractSuperclassUid : myAbstractSuperclassUids)
    {
        code << "#include \"";
        code << access(myAbstractSuperclassUid).label();
        code << ".hpp\"\n";
    }
    for (const UniqueId& myAbstractSubinterfaceClassUid : myAbstractSubinterfaceClassUids)
    {
        code << "#include \"";
        code << access(myAbstractSubinterfaceClassUid).label();
        code << ".hpp\"\n";
    }
    // Start class definition
    // Handle inheritance
    if (myAbstractSuperclassUids.empty())
    {
        code << "struct " << myName;
    } else {
        code << "struct " << myName << " : ";
        Hyperedges::const_iterator it(myAbstractSuperclassUids.begin());
        while (it != myAbstractSuperclassUids.end())
        {
            code << access(*it).label();
            it++;
            if (it != myAbstractSuperclassUids.end())
                code << ", ";
        }
    }
    code << "\n{\n";
    if (myAbstractSubinterfaceUids.empty())
    {
        std::string type(hook.ask("Please provide a C++ type for interface class " + myName, GeneratorHook::QuestionType::QUESTION_PROVIDE_PLAIN_TYPE));
        code << "\t// Storage\n";
        code << "\t" << type << " value;\n";
        // Only atomic interfaces can have an initialization function!
        code << "\t// Initialization\n";
        code << "\tvoid init(const " << type << "& initial_value)\n";
        code << "\t{\n";
        code << "\t\tvalue = initial_value;\n";
        code << "\t}\n";
    } else {
        code << "\t// Subinterfaces\n";
        // TODO: Optimize this by remembering a std::map from myAbstractSubinterfaceUid to its classes!
        for (const UniqueId& myAbstractSubinterfaceUid : myAbstractSubinterfaceUids)
        {
            const Hyperedges& myAbstractSubinterfaceClassUids(instancesOf(Hyperedges{myAbstractSubinterfaceUid},"",FORWARD));
            for (const UniqueId& myAbstractSubinterfaceClassUid : myAbstractSubinterfaceClassUids)
            {
                code << "\t" << access(myAbstractSubinterfaceClassUid).label() << " " << access(myAbstractSubinterfaceUid).label() << ";\n";
            }
        }
    }
    // End class definition
    code << "\n}\n";
    code << "#endif";

    // Finalize
    const Hyperedges& newInterfaceClassUid(createInterface(myImplementationUid, code.str(), Hyperedges{ifClassUid}));
    encodes(newInterfaceClassUid, Hyperedges{abstractInterfaceClassUid});
    isA(newInterfaceClassUid, encodersOf(myAbstractSuperclassUids));
    hasSubInterface(newInterfaceClassUid, myConcreteSubInterfaceUids);
    return newInterfaceClassUid;
}

Hyperedges Generator::generateImplementationClassFor(const UniqueId& algorithmClassUid, const GeneratorHook& hook)
{
    // Here, we generate a C++ class for a given implementation class
    // The template looks like this:
    // class <AlgorithmClass.label> : public <AlgorithmSuperclass.label>, ...
    // {
    //      public:
    //          <AlgorithmClass.label>();
    //          bool operator();
    //          <InterfaceClass> <interfaceName>;
    //          ...
    //      protected:
    //          <ImplementationClass> <partName>;
    //          ...
    // };
    std::stringstream code;
    const std::string& myName(access(algorithmClassUid).label());
    const UniqueId&    myImplementationUid(implClassUid+"::"+algorithmClassUid); // unique + unique = unique
    // Check if implementation exists
    if (exists(myImplementationUid))
        return Hyperedges{myImplementationUid};
    // Before we generate code, we have to make sure, that ...
    // ... all superclasses exist
    const Hyperedges& myAbstractSuperclassUids(intersect(algorithmClasses(), directSubclassesOf(Hyperedges{algorithmClassUid}, "", FORWARD)));
    for (const UniqueId& myAbstractSuperclassUid : myAbstractSuperclassUids)
    {
        generateImplementationClassFor(myAbstractSuperclassUid,hook);
    }
    // ... all interfaces exist
    const Hyperedges& myAbstractInterfaceUids(interfacesOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractInterfaceClassUids;
    Hyperedges myConcreteInterfaceUids;
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        // Get classes
        const Hyperedges& _myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : _myAbstractInterfaceClassUids)
        {
            myConcreteInterfaceUids = unite(myConcreteInterfaceUids, instantiateFrom(generateConcreteInterfaceClassFor(myAbstractInterfaceClassUid,hook), access(myAbstractInterfaceUid).label()));
        }
        myAbstractInterfaceClassUids = unite(myAbstractInterfaceClassUids, _myAbstractInterfaceClassUids);
    }
    // ... all part classes exist
    const Hyperedges& myAbstractPartUids(subcomponentsOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractPartClassUids;
    Hyperedges myConcretePartUids;
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        const Hyperedges& _myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        for (const UniqueId& myAbstractPartClassUid : _myAbstractPartClassUids)
        {
            myConcretePartUids = unite(myConcretePartUids, instantiateComponent(generateImplementationClassFor(myAbstractPartClassUid,hook), access(myAbstractPartUid).label()));
        }
        myAbstractPartClassUids = unite(myAbstractPartClassUids, _myAbstractPartClassUids);
    }
    // Now we can be sure that everything we depend on exists

    // Collect input/output/bidir info
    const Hyperedges& myAbstractInputUids(intersect(myAbstractInterfaceUids, inputsOf(Hyperedges{algorithmClassUid})));
    const Hyperedges& myAbstractOutputUids(intersect(myAbstractInterfaceUids, outputsOf(Hyperedges{algorithmClassUid})));
    const Hyperedges& myAbstractIOUids(subtract(myAbstractInterfaceUids, unite(myAbstractInputUids, myAbstractOutputUids)));

    code << "#ifndef _ALGORITHM_" << myName << "_IMPLEMENTATION\n";
    code << "#define _ALGORITHM_" << myName << "_IMPLEMENTATION\n";
    // Include all things we depend on
    for (const UniqueId& uid : myAbstractSuperclassUids)
    {
        code << "#include \"";
        code << access(uid).label();
        code << ".hpp\"\n";
    }
    for (const UniqueId& uid : myAbstractInterfaceClassUids)
    {
        code << "#include \"";
        code << access(uid).label();
        code << ".hpp\"\n";
    }
    for (const UniqueId& uid : myAbstractPartClassUids)
    {
        code << "#include \"";
        code << access(uid).label();
        code << ".hpp\"\n";
    }
    // Start class definition
    code << "class " << myName;
    if (!myAbstractSuperclassUids.empty())
    {
        code << " : ";
        Hyperedges::const_iterator it(myAbstractSuperclassUids.begin());
        while (it != myAbstractSuperclassUids.end())
        {
            // TODO: Public or (default) private inheritance?
            code << "public " << access(*it).label();
            it++;
            if (it != myAbstractSuperclassUids.end())
                code << ", ";
        }
    }
    code << "\n{\n";
    code << "public:\n";

    // Interfaces
    // Instantiate input interfaces
    code << "\t// Input interfaces\n";
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        const Hyperedges& myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInputUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            code << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractInputUid).label() << ";\n";
        }
    }
    // Instantiate bidirectional interfaces
    // NOTE: myAbstractIOUid holds the name of the instance, myAbstractInterfaceClassUid holds the name of the class
    code << "\t// Bidirectional interfaces\n";
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        const Hyperedges& myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractIOUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            code << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractIOUid).label() << ";\n";
        }
    }
    // Instantiate output interfaces
    code << "\t// Output interfaces\n";
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        const Hyperedges& myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractOutputUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            code << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractOutputUid).label() << ";\n";
        }
    }

    // Constructor
    code << "\t//Constructor\n";
    code << "\t" << myName << "()\n";
    // NOTE: Base class constructors will be called by default
    code << "\t{\n";
    code << "\t\t// Initialize atomic interfaces\n";
    // Initialize atomic interfaces only
    // For each atomic (sub-) interface call init
    // Complex case: we now have to make something like a depth-first-search to identify atomic subinterfaces and also track the path to it
    // Define a prefix (used later)
    std::string prefix("");
    auto cf = [&](const Conceptgraph& cg, const UniqueId& c, const Hyperedges& p) -> bool {
        const Component::Network& cn(static_cast<const Component::Network&>(cg));
        // Check if interface has subinterfaces
        const Hyperedges& subUids(cn.subinterfacesOf(Hyperedges{c}));
        if (subUids.empty())
        {
            // c contains the atomic interface uid, p contains the complete path of subinterface(s) to c (and including c)
            if (cn.access(c).hasProperty("value"))
            {
                code << "\t\t" << prefix;
                for (const UniqueId& pUid : p)
                    code << cn.access(pUid).label() << ".";
                code << "init(";
                code << cn.access(c).property("value");
                code << ");\n";
            }
        }
        // If it does, do nothing
        return false;
    };
    auto rf = [](const Conceptgraph& cg, const UniqueId& c, const UniqueId& r) -> bool {
        const Component::Network& cn(static_cast<const Component::Network&>(cg));
        // Check r <- FACT-OF -> subrelationsOf(HasASubInterfaceId)
        const Hyperedges& toSearch(cn.isPointingTo(cn.relationsFrom(Hyperedges{r}, cn.access(CommonConceptGraph::FactOfId).label())));
        if (intersect(toSearch, cn.subrelationsOf(Hyperedges{Component::Network::HasASubInterfaceId})).empty())
            return false;
        return true;
    };
    // We use the traverse function to generate the correct code
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        prefix = "";
        Conceptgraph::traverse(myAbstractInterfaceUid, cf, rf);
    }
    // Initialize atomic interfaces of parts (see above)
    code << "\t\t// Initialize atomic interfaces of parts\n";
    // Fill with partInterfaceUid and "<partLabel>.<partInterfaceLabel>"
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        prefix = access(myAbstractPartUid).label() + ".";
        const Hyperedges& myAbstractPartInterfaceUids(interfacesOf(Hyperedges{myAbstractPartUid}));
        for (const UniqueId& myAbstractPartInterfaceUid : myAbstractPartInterfaceUids)
        {
            Conceptgraph::traverse(myAbstractPartInterfaceUid, cf, rf);
        }
    }
    code << "\t}\n";

    // Evaluation operator ()
    code << "\t// Evaluation function\n";
    code << "\tvoid operator() ()\n";
    code << "\t{\n";
    code << "\t\t// Pass external inputs to internal inputs\n";
    // Read in external inputs
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        // Pass external inputs to internal inputs
        const Hyperedges& myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractInputUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            const Hyperedges& myAbstractInternalPartUids(inputsOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                code << "\t\t";
                code << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << " = ";
                code << access(myAbstractInputUid).label();
                code << ";\n";
            }
        }
    }
    code << "\t\t// Pass external bidirectional interfaces to internal bidirectional interfaces/inputs\n";
    // Pass bidirectional data to corresponding internal interfaces
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        const Hyperedges& myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            const Hyperedges& myAbstractInternalPartUids(subtract(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE), outputsOf(Hyperedges{myOriginalAbstractInputUid}, "", INVERSE)));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                code << "\t\t";
                code << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << " = ";
                code << access(myAbstractIOUid).label();
                code << ";\n";
            }
        }
    }
    // Call base class evaluation function
    code << "\t\t// Call base class(es) evaluation function\n";
    for (const UniqueId& myAbstractSuperclassUid : myAbstractSuperclassUids)
    {
        code << "\t\t" << access(myAbstractSuperclassUid).label() << "::();\n";
    }
    // Evaluate parts
    code << "\t\t// Call part(s) evaluation function\n";
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        code << "\t\t" << access(myAbstractPartUid).label() << "();\n";
    }

    code << "\t\t// Pass internal outputs/bidirectional interfaces to internal inputs/bidirectional interfaces\n";
    // Pass outputs of internal parts to inputs of connected internal parts
    for (const UniqueId& myAbstractProducerUid : myAbstractPartUids)
    {
        const Hyperedges& internalAbstractOutputUids(subtract(interfacesOf(Hyperedges{myAbstractProducerUid}), inputsOf(Hyperedges{myAbstractProducerUid})));
        for (const UniqueId& internalAbstractOutputUid : internalAbstractOutputUids)
        {
            const Hyperedges& internalAbstractInterfaceUids(endpointsOf(Hyperedges{internalAbstractOutputUid})); // could be any interface (including outputs)
            for (const UniqueId& internalAbstractInterfaceUid : internalAbstractInterfaceUids)
            {
                const Hyperedges& myAbstractConsumerUids(subtract(interfacesOf(Hyperedges{internalAbstractInterfaceUid},"",INVERSE), outputsOf(Hyperedges{internalAbstractInterfaceUid}, "", INVERSE)));
                for (const UniqueId& myAbstractConsumerUid : myAbstractConsumerUids)
                {
                    code << "\t\t";
                    code << access(myAbstractConsumerUid).label() << "." << access(internalAbstractInterfaceUid).label() << " = ";
                    code << access(myAbstractProducerUid).label() << "." << access(internalAbstractOutputUid).label();
                    code << ";\n";
                }
            }
        }
    }
    code << "\t\t// Pass internal outputs to external outputs\n";
    // Pass internal outputs to external outputs
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        const Hyperedges& myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractOutputUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            const Hyperedges& myAbstractInternalPartUids(outputsOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                code << "\t\t";
                code << access(myAbstractOutputUid).label() << " = ";
                code << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                code << ";\n";
            }
        }
    }
    code << "\t\t// Pass internal bidirectional interfaces to external bidirectional interfaces/outputs\n";
    // Pass bidirectional data to corresponding external interface
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        const Hyperedges& myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            const Hyperedges& myAbstractInternalPartUids(subtract(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE), inputsOf(Hyperedges{myOriginalAbstractOutputUid}, "", INVERSE)));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                code << "\t\t";
                code << access(myAbstractIOUid).label() << " = ";
                code << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                code << ";\n";
            }
        }
    }
    code << "\t}\n";

    // Instantiate parts
    code << "\t// Instantiate parts\n";
    code << "protected:\n";
    // NOTE: myAbstractPartUid holds the name of the instance, myAbstractPartClassUid holds the name of the class
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        const Hyperedges& myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        for (const UniqueId& myAbstractPartClassUid : myAbstractPartClassUids)
        {
            code << "\t" << access(myAbstractPartClassUid).label() << " " << access(myAbstractPartUid).label() << ";\n";
        }
    }

    code << "\n}\n";
    code << "#endif";

    // Finalize
    const Hyperedges& newImplementationClassUid(createImplementation(myImplementationUid, code.str(), Hyperedges{implClassUid}));
    implements(newImplementationClassUid, Hyperedges{algorithmClassUid});
    isA(newImplementationClassUid, implementationsOf(myAbstractSuperclassUids));
    partOfComponent(myConcretePartUids, newImplementationClassUid);

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
