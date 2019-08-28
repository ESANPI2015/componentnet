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

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges _abstractIfSuperclassUids(directSubclassesOf(Hyperedges{abstractInterfaceClassUid}, "", Hypergraph::TraversalDirection::FORWARD));
    std::string name(access(abstractInterfaceClassUid).label());

    result << "#ifndef _INTERFACE_" << name << "_IMPL\n";
    result << "#define _INTERFACE_" << name << "_IMPL\n";

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
    //for (const UniqueId& validConcreteIfSuperclassUid : validConcreteIfSuperclassUids)
    //{
    //    result << access(validConcreteIfSuperclassUid).label();
    //}
    //for (const UniqueId& validInterfacePartClassUid : validInterfacePartClassUids)
    //{
    //    result << access(validInterfacePartClassUid).label();
    //}
    // Alternative: Say, that we want to include another header file
    for (const UniqueId& abstractIfSuperclassUid : abstractIfSuperclassUids)
    {
        result << "#include \"";
        result << access(abstractIfSuperclassUid).label();
        result << ".hpp\"\n";
    }
    for (const UniqueId& myAbstractInterfacePartClassUid : validInterfacePartClassUids)
    {
        result << "#include \"";
        result << access(myAbstractInterfacePartClassUid).label();
        result << ".hpp\"\n";
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
    if (myInterfacePartUids.empty())
    {
        std::string type(hook.ask("Please provide a C++ type for interface class " + name, GeneratorHook::QuestionType::QUESTION_PROVIDE_PLAIN_TYPE));
        result << "\t// Storage\n";
        result << "\t" << type << " value;\n";
        // Only atomic interfaces can have an initialization function!
        result << "\t// Initialization\n";
        result << "\tvoid init(const " << type << "& initial_value)\n";
        result << "\t{\n";
        result << "\t\tvalue = initial_value;\n";
        result << "\t}\n";
    } else {
        result << "\t// Subinterfaces\n";
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
    Hyperedges myAbstractInterfaceClassUids;
    Hyperedges myConcreteInterfaceClassUids;
    Hyperedges myConcreteInterfaceUids;
    for (const UniqueId& myAbstractInterfaceUid : _myAbstractInterfaceUids)
    {
        // Get concrete interface
        Hyperedges abstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        Hyperedges concreteInterfaceClassUids(intersect(validConcreteInterfaceClassUids, encodersOf(abstractInterfaceClassUids)));
        UniqueId uid;
        if (concreteInterfaceClassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("No concrete interface found for "+access(abstractInterfaceClassUids[0]).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IFCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateConcreteInterfaceClassFor(abstractInterfaceClassUids[0], uid, hook);
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
        myAbstractInterfaceClassUids = unite(myAbstractInterfaceClassUids, abstractInterfaceClassUids);
        myConcreteInterfaceClassUids = unite(myConcreteInterfaceClassUids, Hyperedges{uid});
        myConcreteInterfaceUids = unite(myConcreteInterfaceUids, instantiateFrom(Hyperedges{uid}, access(myAbstractInterfaceUid).label()));
    }

    // ... and of our parts (instantiate them as well)
    Hyperedges _myAbstractPartUids(subcomponentsOf(Hyperedges{algorithmClassUid}));
    Hyperedges myAbstractPartUids;
    Hyperedges myAbstractPartClassUids;
    Hyperedges myImplementationPartClassUids;
    Hyperedges myImplementationPartUids;
    for (const UniqueId& myAbstractPartUid : _myAbstractPartUids)
    {
        // Get concrete implementation from myAbstractPartUid superclass
        Hyperedges abstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
        Hyperedges implementationClassUids(intersect(validImplementationClassUids, implementationsOf(abstractPartClassUids)));
        UniqueId uid;
        if (implementationClassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("No concrete implementation found for "+access(abstractPartClassUids[0]).label()+". Generate it? [y/n]", GeneratorHook::QuestionType::QUESTION_GENERATE_IMPLCLASS) == "y")
            {
                uid = hook.ask("Please provide a unique id", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid", GeneratorHook::QuestionType::QUESTION_PROVIDE_UID);
                generateImplementationClassFor(abstractPartClassUids[0], uid, hook);
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
        myAbstractPartClassUids = unite(myAbstractPartClassUids, abstractPartClassUids);
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
    //for (const UniqueId& uid : myImplementationSuperclassUids)
    //{
    //    result << access(uid).label() << std::endl;
    //}
    //for (const UniqueId& uid : myConcreteInterfaceClassUids)
    //{
    //    result << access(uid).label() << std::endl;
    //}
    //for (const UniqueId& uid : myImplementationPartClassUids)
    //{
    //    result << access(uid).label() << std::endl;
    //}
    // Alternative: Say, that we want to include another header file
    for (const UniqueId& uid : algorithmSuperclassUids)
    {
        result << "#include \"";
        result << access(uid).label();
        result << ".hpp\"\n";
    }
    for (const UniqueId& uid : myAbstractInterfaceClassUids)
    {
        result << "#include \"";
        result << access(uid).label();
        result << ".hpp\"\n";
    }
    for (const UniqueId& uid : myAbstractPartClassUids)
    {
        result << "#include \"";
        result << access(uid).label();
        result << ".hpp\"\n";
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
    result << "\t//Constructor\n";
    result << "\t" << name << "()\n";
    // NOTE: Base class constructors will be called by default
    result << "\t{\n";
    result << "\t\t// Initialize atomic interfaces\n";
    // Initialize atomic interfaces only
    // For each atomic (sub-) interface call init
    // Complex case: we now have to make something like a depth-first-search to identify atomic subinterfaces and also track the path to it
    // Define a prefix (used later)
    std::string prefix("");
    auto cf = [&](const Conceptgraph& cg, const UniqueId& c, const Hyperedges& p) -> bool {
        const Component::Network& cn(static_cast<const Component::Network&>(cg));
        // Check if interface has subinterfaces
        Hyperedges subUids(cn.subinterfacesOf(Hyperedges{c}));
        if (subUids.empty())
        {
            // If not, get values & use path to update result
            Hyperedges valueUids(cn.valuesOf(Hyperedges{c}));
            for (const UniqueId& valueUid : valueUids)
            {
                result << "\t\t" << prefix;
                for (const UniqueId& pUid : p)
                    result << cn.access(pUid).label() << ".";
                result << "init(";
                result << cn.access(valueUid).label();
                result << ");\n";
            }
        }
        // If it does, do nothing
        return false;
    };
    auto rf = [](const Conceptgraph& cg, const UniqueId& c, const UniqueId& r) -> bool {
        const Component::Network& cn(static_cast<const Component::Network&>(cg));
        // Check r <- FACT-OF -> subrelationsOf(HasASubInterfaceId)
        Hyperedges toSearch(cn.isPointingTo(cn.relationsFrom(Hyperedges{r}, cn.access(CommonConceptGraph::FactOfId).label())));
        if (intersect(toSearch, cn.subrelationsOf(Hyperedges{Component::Network::HasASubInterfaceId})).empty())
            return false;
        return true;
    };
    // We use the traverse function to generate the correct code
    for (const UniqueId& myAbstractInterfaceUid : myAbstractInterfaceUids)
    {
        Conceptgraph::traverse(myAbstractInterfaceUid, cf, rf);
    }
    // Initialize atomic interfaces of parts (see above)
    result << "\t\t// Initialize atomic interfaces of parts\n";
    // Fill with partInterfaceUid and "<partLabel>.<partInterfaceLabel>"
    for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
    {
        prefix = access(myAbstractPartUid).label() + ".";
        Hyperedges myAbstractPartInterfaceUids(interfacesOf(Hyperedges{myAbstractPartUid}));
        for (const UniqueId& myAbstractPartInterfaceUid : myAbstractPartInterfaceUids)
        {
            Conceptgraph::traverse(myAbstractPartInterfaceUid, cf, rf);
        }
    }
    result << "\t\t// TODO: Custom initialization code starts here\n";
    result << "\t}\n";

    // Instantiate input interfaces
    if (!myAbstractInputUids.empty())
        result << "\t// Input interfaces\n";
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInputUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            result << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractInputUid).label() << ";\n";
        }
    }
    // Instantiate bidirectional interfaces
    // NOTE: myAbstractIOUid holds the name of the instance, myAbstractInterfaceClassUid holds the name of the class
    if (!myAbstractIOUids.empty())
        result << "\t// Bidirectional interfaces\n";
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractIOUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            result << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractIOUid).label() << ";\n";
        }
    }
    // Instantiate output interfaces
    if (!myAbstractOutputUids.empty())
        result << "\t// Output interfaces\n";
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractOutputUid},"",FORWARD));
        for (const UniqueId& myAbstractInterfaceClassUid : myAbstractInterfaceClassUids)
        {
            result << "\t" << access(myAbstractInterfaceClassUid).label() << " " << access(myAbstractOutputUid).label() << ";\n";
        }
    }

    if (myAbstractPartUids.empty() && algorithmSuperclassUids.empty())
    {
        // NOTE: This will force the developer to implement that function
        result << "\t// Evaluation function\n";
        result << "\tvoid operator() ()\n";
        result << "\t{\n";
        result << "\t#error \"Implement me\"\n";
        result << "\t}\n";
    } else {
        // Evaluation function
        result << "\t// Evaluation function\n";
        result << "\tvoid operator() ()\n";
        result << "\t{\n";
        if (!myAbstractInputUids.empty())
        {
            result << "\t\t// Pass external inputs to internal inputs\n";
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
        }
        if (!myAbstractIOUids.empty())
        {
            result << "\t\t// Pass external bidirectional interfaces to internal bidirectional interfaces/inputs\n";
            // Pass bidirectional data to corresponding internal interfaces
            for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
            {
                Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
                for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
                {
                    Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
                    myAbstractInternalPartUids = subtract(myAbstractInternalPartUids, outputsOf(Hyperedges{myOriginalAbstractInputUid}, "", INVERSE));
                    for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
                    {
                        result << "\t\t";
                        result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractInputUid).label() << " = ";
                        result << access(myAbstractIOUid).label();
                        result << ";\n";
                    }
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
            result << "\t\t// TODO: Implementation starts here\n";
            // TODO: This is questionable ... we should leave the implementation to the specialist!
            //std::stringstream question;
            //question << result.rdbuf() << "Please provide your implementation code";
            //result << "\t" << hook.ask(question.str(), GeneratorHook::QuestionType::QUESTION_PROVIDE_CODE) << "\n";
        } else {
            result << "\t\t// Call inner parts\n";
            // Evaluate parts
            for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
            {
                result << "\t\t" << access(myAbstractPartUid).label() << "();\n";
            }
        }
        if (!myAbstractPartUids.empty())
        {
            result << "\t\t// Pass internal outputs/bidirectional interfaces to internal inputs/bidirectional interfaces\n";
            // Pass outputs of internal parts to inputs of connected internal parts
            for (const UniqueId& myAbstractProducerUid : myAbstractPartUids)
            {
                Hyperedges internalAbstractOutputUids(interfacesOf(Hyperedges{myAbstractProducerUid})); // these are ALL interfaces
                internalAbstractOutputUids = subtract(internalAbstractOutputUids, inputsOf(Hyperedges{myAbstractProducerUid})); // ... so we subtract the inputs from it :)
                for (const UniqueId& internalAbstractOutputUid : internalAbstractOutputUids)
                {
                    Hyperedges internalAbstractInterfaceUids(endpointsOf(Hyperedges{internalAbstractOutputUid})); // could be any interface (including outputs)
                    for (const UniqueId& internalAbstractInterfaceUid : internalAbstractInterfaceUids)
                    {
                        Hyperedges myAbstractConsumerUids(interfacesOf(Hyperedges{internalAbstractInterfaceUid},"",INVERSE)); // this could contain invalid consumers (connections to outputs)
                        myAbstractConsumerUids = subtract(myAbstractConsumerUids, outputsOf(Hyperedges{internalAbstractInterfaceUid}, "", INVERSE)); // ... so we subtract the ones with outputs
                        for (const UniqueId& myAbstractConsumerUid : myAbstractConsumerUids)
                        {
                            result << "\t\t";
                            result << access(myAbstractConsumerUid).label() << "." << access(internalAbstractInterfaceUid).label() << " = ";
                            result << access(myAbstractProducerUid).label() << "." << access(internalAbstractOutputUid).label();
                            result << ";\n";
                        }
                    }
                }
            }
        }
        if (!myAbstractOutputUids.empty())
        {
            result << "\t\t// Pass internal outputs to external outputs\n";
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
        }
        if (!myAbstractIOUids.empty())
        {
            result << "\t\t// Pass internal bidirectional interfaces to external bidirectional interfaces/outputs\n";
            // Pass bidirectional data to corresponding external interface
            for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
            {
                Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractIOUid}));
                for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
                {
                    Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
                    myAbstractInternalPartUids = subtract(myAbstractInternalPartUids, inputsOf(Hyperedges{myOriginalAbstractOutputUid}, "", INVERSE));
                    for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
                    {
                        result << "\t\t";
                        result << access(myAbstractIOUid).label() << " = ";
                        result << access(myAbstractInternalPartUid).label() << "." << access(myOriginalAbstractOutputUid).label();
                        result << ";\n";
                    }
                }
            }
        }
        result << "\t}\n";
    }

    // Parts are protected so subclasses can access them
    if (!myAbstractPartUids.empty())
    {
        result << "\t// Instantiate parts\n";
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
