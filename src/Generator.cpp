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
            if (hook.ask("No concrete interface found for "+access(abstractIfSuperclassUid).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IFCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateConcreteInterfaceClassFor(abstractIfSuperclassUid, uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteIfSuperclassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]", GeneratorHook::QuestionType::QUESTION_USE_IFCLASS) == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        abstractIfSuperclassUids = unite(abstractIfSuperclassUids, Hyperedges{abstractIfSuperclassUid}); // NOTE: unite() will not add duplicates!
        validConcreteIfSuperclassUids = unite(validConcreteIfSuperclassUids, Hyperedges{uid});
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
            if (hook.ask("No concrete interface found for "+access(myAbstractInterfacePartClassUids[0]).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IFCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateConcreteInterfaceClassFor(myAbstractInterfacePartClassUids[0], uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfacePartClassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]", GeneratorHook::QuestionType::QUESTION_USE_IFCLASS) == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractInterfacePartUids = unite(myAbstractInterfacePartUids, Hyperedges{myAbstractInterfacePartUid});
        validInterfacePartClassUids = unite(validInterfacePartClassUids, Hyperedges{uid});
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
    // NOTE: We use a struct because then all members are per default public
    if (abstractIfSuperclassUids.empty())
    {
        result << "struct " << name;
    } else {
        result << "struct " << name << " : ";
        Hyperedges::const_iterator it(abstractIfSuperclassUids.begin());
        while (it != abstractIfSuperclassUids.end())
        {
            result << access(*it).label();
            it++;
            if (it != abstractIfSuperclassUids.end())
                result << ", ";
        }
    }
    result << "\n{\n";
    // TODO: Can we do better than initializing from string? Also, shouldn't this reflect the underlying C++ types / parts?
    result << "\tvoid init(const std::string& config)\n";
    result << "\t{\n";
    // Call base class(es) init. However, what does this mean? Base class could be e.g. Vector3d and we add time? But how does this relate to interface PARTS?
    for (const UniqueId& abstractIfSuperclassUid : abstractIfSuperclassUids)
    {
        result << "\t\t" << access(abstractIfSuperclassUid).label() << "::init(value);\n";
    }
    result << "\t}\n";
    if (myInterfacePartUids.empty())
    {
        std::string type(hook.ask("Please provide a C++ type for interface class " + name, GeneratorHook::QuestionType::QUESTION_PROVIDE_PLAIN_TYPE));
        result << "\t" << type << " value;\n";
    } else {
        for (const UniqueId& myAbstractInterfacePartUid : myAbstractInterfacePartUids)
        {
            Hyperedges myAbstractInterfacePartClassUids(instancesOf(Hyperedges{myAbstractInterfacePartUid},"",FORWARD));
            for (const UniqueId& myAbstractInterfacePartClassUid : myAbstractInterfacePartClassUids)
            {
                result << "\t" << access(myAbstractInterfacePartClassUid).label() << " " << access(myAbstractInterfacePartUid).label() << ";\n";
            }
        }
    }
    result << "\n};\n";
    result << "#endif\n";

    Hyperedges newInterfaceClassUid(createInterface(concreteInterfaceClassUid, result.str(), Hyperedges{ifClassUid}));
    encodes(newInterfaceClassUid, Hyperedges{abstractInterfaceClassUid});
    isA(newInterfaceClassUid, validConcreteIfSuperclassUids);
    hasSubInterface(newInterfaceClassUid, myInterfacePartUids);
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
            if (hook.ask("No concrete implementation found for "+access(algorithmSuperclassUid).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IMPLCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateImplementationClassFor(algorithmSuperclassUid, uid, hook);
                validImplementationClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& implUid : implementationSuperclassUids)
            {
                if (hook.ask("Use concrete implementation "+access(implUid).label()+"? [y/n]", GeneratorHook::QuestionType::QUESTION_USE_IMPLCLASS) == "y")
                {
                    uid = implUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        algorithmSuperclassUids = unite(algorithmSuperclassUids, Hyperedges{algorithmSuperclassUid});
        myImplementationSuperclassUids = unite(myImplementationSuperclassUids, Hyperedges{uid});
    }

    // Collect all interface info (and instantiate concrete interfaces)
    Hyperedges _myAbstractInterfaceUids(interfacesOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractInterfaceUids;
    Hyperedges myConcreteInterfaceClassUids;
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
            if (hook.ask("No concrete interface found for "+access(myAbstractInterfaceClassUids[0]).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IFCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateConcreteInterfaceClassFor(myAbstractInterfaceClassUids[0], uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfaceClassUids)
            {
                if (hook.ask("Use concrete interface "+access(ifUid).label()+"? [y/n]", GeneratorHook::QuestionType::QUESTION_USE_IFCLASS) == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractInterfaceUids = unite(myAbstractInterfaceUids, Hyperedges{myAbstractInterfaceUid});
        myConcreteInterfaceClassUids = unite(myConcreteInterfaceClassUids, Hyperedges{uid});
        myConcreteInterfaceUids = unite(myConcreteInterfaceUids, instantiateFrom(Hyperedges{uid}, access(myAbstractInterfaceUid).label()));
    }

    // ... and of our parts (instantiate them as well)
    Hyperedges _myAbstractPartUids(subcomponentsOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractPartUids;
    Hyperedges myImplementationPartClassUids;
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
            if (hook.ask("No concrete implementation found for "+access(myAbstractPartClassUids[0]).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IMPLCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateImplementationClassFor(myAbstractPartClassUids[0], uid, hook);
                validImplementationClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& implUid : implementationClassUids)
            {
                if (hook.ask("Use concrete implementation "+access(implUid).label()+"? [y/n]", GeneratorHook::QuestionType::QUESTION_USE_IMPLCLASS) == "y")
                {
                    uid = implUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractPartUids = unite(myAbstractPartUids, Hyperedges{myAbstractPartUid});
        myImplementationPartClassUids = unite(myImplementationPartClassUids, Hyperedges{uid});
        myImplementationPartUids = unite(myImplementationPartUids, instantiateComponent(Hyperedges{uid}, access(myAbstractPartUid).label()));
    }

    // Collect input/output/bidir info
    Hyperedges myAbstractInputUids(intersect(myAbstractInterfaceUids, inputsOf(Hyperedges{algorithmClassUid})));
    Hyperedges myAbstractOutputUids(intersect(myAbstractInterfaceUids, outputsOf(Hyperedges{algorithmClassUid})));
    Hyperedges myAbstractIOUids(subtract(myAbstractInterfaceUids, unite(myAbstractInputUids, myAbstractOutputUids)));

    result << "#ifndef _ALGORITHM_" << name << "_IMPL\n";
    result << "#define _ALGORITHM_" << name << "_IMPL\n";

    // Copy and paste the needed classes into this definition
    for (const UniqueId& uid : myImplementationSuperclassUids)
    {
        result << access(uid).label() << std::endl;
    }
    for (const UniqueId& uid : myConcreteInterfaceClassUids)
    {
        result << access(uid).label() << std::endl;
    }
    for (const UniqueId& uid : myImplementationPartClassUids)
    {
        result << access(uid).label() << std::endl;
    }

    if (algorithmSuperclassUids.empty())
    {
        result << "class " << name;
    } else {
        result << "class " << name << " : ";
        Hyperedges::const_iterator it(algorithmSuperclassUids.begin());
        while (it != algorithmSuperclassUids.end())
        {
            // TODO: Public or (default) private inheritance?
            result << "public " << access(*it).label();
            it++;
            if (it != algorithmSuperclassUids.end())
                result << ", ";
        }
    }
    result << "\n{\n";
    result << "public:\n";

    // Constructor
    result << "\t" << name << "()\n";
    // NOTE: Base class constructors will be called by default
    result << "\t{\n";
    // Initialize interfaces
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Hyperedges myAbstractInterfaceValueUids(valuesOf(Hyperedges{myAbstractInterfaceUid}));
        for (const UniqueId& myAbstractInterfaceValueUid : myAbstractInterfaceValueUids)
        {
            result << "\t\t";
            result << access(myAbstractInterfaceUid).label() << ".init(\"";
            result << access(myAbstractInterfaceValueUid).label();
            result << "\");\n";
        }
    }
    // TODO: Initialize interfaces of parts
    result << "\t\t// Initialization code starts here\n";
    result << "\t}\n";

    // Instantiate interfaces
    // NOTE: myAbstractInterfaceUid holds the name of the instance, myAbstractInterfaceClassUid holds the name of the class
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            result << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractInterfaceUid).label() << ";\n";
        }
    }

    // Evaluation function
    result << "\tbool operator() ()\n";
    result << "\t{\n";
    // Read in external inputs
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        // Pass external inputs to internal inputs
        Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractInputUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            Hyperedges myAbstractInternalPartUids(inputsOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << " = ";
                result << access(myAbstractInputUid).label();
                result << ";\n";
            }
        }
    }
    // Pass bidirectional data to corresponding internal interfaces
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << " = ";
                result << access(myAbstractIOUid).label();
                result << ";\n";
            }
        }
    }
    // Call base class evaluate operators
    for (const UniqueId& algorithmSuperclassUid : algorithmSuperclassUids)
    {
        result << "\t\t" << access(algorithmSuperclassUid).label() << "::();\n";
    }
    if (myAbstractPartUids.empty())
    {
        result << "\t\t// Implementation starts here\n";
        std::stringstream question;
        question << result.rdbuf() << "Please provide your implementation code";
        result << "\t" << hook.ask(question.str(), GeneratorHook::QuestionType::QUESTION_PROVIDE_CODE) << "\n";
    } else {
        // Evaluate parts
        for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
        {
            result << "\t\t" << access(myAbstractPartUid).label() << "();\n";
        }
    }
    // Pass outputs of internal parts to inputs of connected internal parts
    // TODO: Pass bidirectional interfaces to connected inputs
    for (const UniqueId& myAbstractProducerUid : myAbstractPartUids)
    {
        Hyperedges internalAbstractOutputUids(outputsOf(Hyperedges{myAbstractProducerUid}));
        for (const UniqueId& internalAbstractOutputUid : internalAbstractOutputUids)
        {
            Hyperedges internalAbstractInputUids(endpointsOf(Hyperedges{internalAbstractOutputUid}));
            for (const UniqueId& internalAbstractInputUid : internalAbstractInputUids)
            {
                Hyperedges myAbstractConsumerUids(inputsOf(Hyperedges{internalAbstractInputUid}, "", INVERSE));
                for (const UniqueId& myAbstractConsumerUid : myAbstractConsumerUids)
                {
                    result << "\t\t";
                    result << access(myAbstractConsumerUid).label() << "." << access(internalAbstractInputUid).label() << " = ";
                    result << access(myAbstractProducerUid).label() << "." << access(internalAbstractOutputUid).label();
                    result << ";\n";
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
            Hyperedges myAbstractInternalPartUids(outputsOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t";
                result << access(myAbstractOutputUid).label() << " = ";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                result << ";\n";
            }
        }
    }
    // Pass bidirectional data to corresponding external interface
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
        for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
        {
            Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
            for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
            {
                result << "\t\t";
                result << access(myAbstractIOUid).label() << " = ";
                result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                result << ";\n";
            }
        }
    }
    result << "\t}\n";

    // TODO: Protected or private?
    if (!myAbstractPartUids.empty())
        result << "protected:\n";

    // Instantiate parts
    // NOTE: myAbstractPartUid holds the name of the instance, myAbstractPartClassUid holds the name of the class
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        Hyperedges myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        for (const UniqueId& myAbstractPartClassUid : myAbstractPartClassUids)
        {
            result << "\t" << access(myAbstractPartClassUid).label() << " " << access(myAbstractPartUid).label() << ";\n";
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
