#include "Generator.hpp"
#include <iostream>

namespace Software {

std::string GeneratorHook::ask(const std::string& question) const
{
   std::string answer;
   std::cout << question << ": ";
   std::cin >> answer;
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

Hyperedges Generator::generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const UniqueId& concreteInterfaceClassUid, const GeneratorHook& hook)
{
    // Here, we generate a C++ class for a given interface
    // The template looks like this:
    // class <AbstractInterfaceClass.label> : public <AbstractInterfaceSuperclass.label>, ...
    // {
    //      public:
    //          <AbstractInterfacePartClass.label> partName;
    //          ...
    //          custom member variables
    // };
    // TODO: Can we do better than copiing code?

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges _abstractIfSuperclassUids(directSubclassesOf(Hyperedges{abstractInterfaceClassUid}, "", Hypergraph::TraversalDirection::FORWARD));
    std::string name(access(abstractInterfaceClassUid).label());

    result << "#ifndef _INTERFACE_" << name << "_IMPL\n";
    result << "#define _INTERFACE_" << name << "_IMPL\n";
    result << "#include <string>\n";

    // Collect all valid information
    Hyperedges abstractIfSuperclassUids;
    Hyperedges validConcreteIfSuperclassUids;
    for (const UniqueId& abstractIfSuperclassUid : _abstractIfSuperclassUids)
    {
        // Get the concrete interface class
        Hyperedges concreteIfSuperclassUids(intersect(validConcreteInterfaceClassUids, encodersOf(Hyperedges{abstractIfSuperclassUid})));
        UniqueId uid;
        if (concreteIfSuperclassUids.empty())
        {
            // If none exists, ignore? Or generate it?
            if (hook.ask("No concrete interface found for "+access(abstractIfSuperclassUid).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(abstractIfSuperclassUid, uid, hook);
            }
        } else {
            for (const UniqueId& ifUid : concreteIfSuperclassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        abstractIfSuperclassUids.push_back(abstractIfSuperclassUid);
        validConcreteIfSuperclassUids.push_back(uid);
    }

    // Collect part information
    Hyperedges _myAbstractInterfacePartUids(subinterfacesOf(Hyperedges{abstractInterfaceClassUid}));
    Hyperedges myAbstractInterfacePartUids;
    Hyperedges myInterfacePartUids;
    Hyperedges validInterfacePartClassUids;
    for (const UniqueId& myAbstractInterfacePartUid : _myAbstractInterfacePartUids)
    {
        // Get concrete interface from superclass
        Hyperedges myAbstractInterfacePartClassUids(instancesOf(Hyperedges{myAbstractInterfacePartUid},"",FORWARD));
        Hyperedges concreteInterfacePartClassUids(intersect(validConcreteInterfaceClassUids, implementationsOf(myAbstractInterfacePartClassUids)));
        UniqueId uid;
        if (concreteInterfacePartClassUids.empty())
        {
            // If none given, generate it or ignore it
            if (hook.ask("No concrete interface found for "+access(myAbstractInterfacePartClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(myAbstractInterfacePartClassUids[0], uid, hook);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfacePartClassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractInterfacePartUids.push_back(myAbstractInterfacePartUid);
        validInterfacePartClassUids.push_back(uid);
        myInterfacePartUids = unite(myInterfacePartUids, instantiateFrom(Hyperedges{uid}, access(myAbstractInterfacePartUid).label()));
    }

    // Collect other definitions (superclasses & parts)
    // NOTE: We do this by copiing code ... Maybe we can do something better?
    for (const UniqueId& validConcreteIfSuperclassUid : validConcreteIfSuperclassUids)
    {
        result << access(validConcreteIfSuperclassUid).label();
    }
    for (const UniqueId& validInterfacePartClassUid : validInterfacePartClassUids)
    {
        result << access(validInterfacePartClassUid).label();
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
    result << "\t\tvoid init(const std::string& value)\n";
    result << "\t\t{\n";
    // Call base class(es) init. However, what does this mean? Base class could be e.g. Vector3d and we add time? But how does this relate to interface PARTS?
    for (const UniqueId& abstractIfSuperclassUid : abstractIfSuperclassUids)
    {
        result << "\t\t\t" << access(abstractIfSuperclassUid).label() << "::init(value);\n";
    }
    result << "\t\t}\n";
    result << "\t\tvoid gets(const " << name << "& other)\n";
    result << "\t\t{\n";
    // NOTE: No need to call base class gets, because their things will get set as well.
    result << "\t\t\t*this = other;\n";
    result << "\t\t}\n";
    if (myInterfacePartUids.empty())
    {
        std::string type(hook.ask("Please provide a C++ type for interface class " + name));
        result << "\t\t" << type << " currentValue;\n";
    } else {
        for (const UniqueId& myAbstractInterfacePartUid : myAbstractInterfacePartUids)
        {
            Hyperedges myAbstractInterfacePartClassUids(instancesOf(Hyperedges{myAbstractInterfacePartUid},"",FORWARD));
            for (const UniqueId& myAbstractInterfacePartClassUid : myAbstractInterfacePartClassUids)
            {
                result << "\t\t" << access(myAbstractInterfacePartClassUid).label() << " " << access(myAbstractInterfacePartUid).label() << ";\n";
            }
        }
    }
    result << "\n};\n";
    result << "#endif\n";

    Hyperedges newInterfaceClassUid(createInterface(concreteInterfaceClassUid, result.str(), Hyperedges{ifClassUid}));
    encodes(newInterfaceClassUid, Hyperedges{abstractInterfaceClassUid});
    isA(newInterfaceClassUid, validConcreteIfSuperclassUids);
    partOfInterface(myInterfacePartUids, newInterfaceClassUid);
    return newInterfaceClassUid;
}


Hyperedges Generator::generateImplementationClassFor(const UniqueId& algorithmClassUid, const UniqueId& concreteImplementationClassUid, const GeneratorHook& hook)
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
    // TODO: Can we do better than copiing code?

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
        UniqueId uid;
        if (implementationSuperclassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("No concrete implementation found for "+access(algorithmSuperclassUid).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateImplementationClassFor(algorithmSuperclassUid, uid, hook);
            }
        } else {
            for (const UniqueId& implUid : implementationSuperclassUids)
            {
                if (hook.ask("Use concrete implementation "+access(implUid).label()+"? [y/n]") == "y")
                {
                    uid = implUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        result << access(uid).label();
        algorithmSuperclassUids.push_back(algorithmSuperclassUid);
        myImplementationSuperclassUids.push_back(uid);
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
        UniqueId uid;
        if (concreteInterfaceClassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("No concrete interface found for "+access(myAbstractInterfaceClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(myAbstractInterfaceClassUids[0], uid, hook);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfaceClassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        result << access(uid).label();
        myAbstractInterfaceUids.push_back(myAbstractInterfaceUid);
        myConcreteInterfaceUids = unite(myConcreteInterfaceUids, instantiateFrom(Hyperedges{uid}, access(myAbstractInterfaceUid).label()));
    }

    // ... and of our parts (instantiate them as well)
    Hyperedges _myAbstractPartUids(subcomponentsOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractPartUids;
    Hyperedges myImplementationPartUids;
    for (const UniqueId& myAbstractPartUid : _myAbstractPartUids)
    {
        // Get concrete implementation from myAbstractPartUid superclass
        Hyperedges myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        Hyperedges implementationClassUids(intersect(validImplementationClassUids, implementationsOf(myAbstractPartClassUids)));
        UniqueId uid;
        if (implementationClassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("No concrete implementation found for "+access(myAbstractPartClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateImplementationClassFor(myAbstractPartClassUids[0], uid, hook);
            }
        } else {
            for (const UniqueId& implUid : implementationClassUids)
            {
                if (hook.ask("Use concrete implementation "+access(implUid).label()+"? [y/n]") == "y")
                {
                    uid = implUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        result << access(uid).label();
        myAbstractPartUids.push_back(myAbstractPartUid);
        myImplementationPartUids = unite(myImplementationPartUids, instantiateComponent(Hyperedges{uid}, access(myAbstractPartUid).label()));
    }
    // Collect input/output/bidir info
    Hyperedges myAbstractInputUids(intersect(myAbstractInterfaceUids, inputsOf(Hyperedges{algorithmClassUid})));
    Hyperedges myAbstractOutputUids(intersect(myAbstractInterfaceUids, outputsOf(Hyperedges{algorithmClassUid})));
    Hyperedges myAbstractIOUids(subtract(myAbstractInterfaceUids, unite(myAbstractInputUids, myAbstractOutputUids)));

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
    // NOTE: Base class constructors will be called by default
    result << "\t\t{\n";
    // Initialize interfaces
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Hyperedges myAbstractInterfaceValueUids(valuesOf(Hyperedges{myAbstractInterfaceUid}));
        for (const UniqueId& myAbstractInterfaceValueUid : myAbstractInterfaceValueUids)
        {
            result << "\t\t\t";
            result << access(myAbstractInterfaceUid).label() << ".init(\"";
            result << access(myAbstractInterfaceValueUid).label();
            result << "\");\n";
        }
    }
    // TODO: Initialize interfaces of parts
    result << "\t\t\t// Initialization code starts here\n";
    result << "\t\t}\n";

    // Evaluation function
    result << "\t\tbool operator() ()\n";
    result << "\t\t{\n";
    // Read in external inputs
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        // Pass external inputs to internal inputs
        Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractInputUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t\t";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << ".gets(";
                result << access(myAbstractInputUid).label();
                result << ");\n";
            }
        }
    }
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t\t";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << ".gets(";
                result << access(myAbstractIOUid).label();
                result << ");\n";
            }
        }
    }
    // Call base class evaluate operators
    for (const UniqueId& algorithmSuperclassUid : algorithmSuperclassUids)
    {
        result << "\t\t\t" << access(algorithmSuperclassUid).label() << "::();\n";
    }
    // Evaluate parts
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        result << "\t\t\t" << access(myAbstractPartUid).label() << "();\n";
    }
    result << "\t\t\t// Implementation starts here\n";
    // Pass outputs of internal parts to inputs of connected internal parts
    // TODO: Handle bidirectional interfaces
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
                    result << access(myAbstractConsumerUid).label() << "." << access(internalAbstractInputUid).label() << ".gets(";
                    result << access(myAbstractProducerUid).label() << "." << access(internalAbstractOutputUid).label();
                    result << ");\n";
                }
            }
        }
    }
    // Pass internal outputs to external outputs
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractOutputUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t\t";
                result << access(myAbstractOutputUid).label() << ".gets(";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                result << ");\n";
            }
        }
    }
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t\t";
                result << access(myAbstractIOUid).label() << ".gets(";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
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
    partOfComponent(myImplementationPartUids, newImplementationClassUid);

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
