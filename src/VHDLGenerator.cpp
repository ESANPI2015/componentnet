#include "VHDLGenerator.hpp"
#include <iostream>

namespace Software {

Hyperedges VHDLGenerator::generateConcreteInterfaceClassFor(const UniqueId& abstractInterfaceClassUid, const UniqueId& concreteInterfaceClassUid, const GeneratorHook& hook)
{
    /*
       Here we generate a concrete interface in VHDL.
       That means, that we have to assemble an interface type:

        subtype <name> is <built-in-type>;

       OR (composite interface)

        type <name> is record
            <part-name> : <part-type>;
        end record;

        NOTES:
        * subtype for encapsulating build in types (e.g. subtype float32 is std_logic_vector(31 downto 0))
        * type for higher order types (e.g. type vector3f is array(0 to 2) of float32)
    */

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges _abstractIfSuperclassUids(directSubclassesOf(Hyperedges{abstractInterfaceClassUid}, "", Hypergraph::TraversalDirection::FORWARD));
    std::string name(access(abstractInterfaceClassUid).label());

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
            if (hook.ask("A. No concrete interface found for "+access(abstractIfSuperclassUid).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(abstractIfSuperclassUid, uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteIfSuperclassUids)
            {
                if (hook.ask("A. Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        abstractIfSuperclassUids = unite(abstractIfSuperclassUids, Hyperedges{abstractIfSuperclassUid});
        validConcreteIfSuperclassUids = unite(validConcreteIfSuperclassUids, Hyperedges{uid});
    }

    // Collect part information
    Hyperedges _myAbstractInterfacePartUids(subinterfacesOf(Hyperedges{abstractInterfaceClassUid}));
    Hyperedges myAbstractInterfacePartUids;
    Hyperedges myInterfacePartUids;
    for (const UniqueId& myAbstractInterfacePartUid : _myAbstractInterfacePartUids)
    {
        // Get concrete interface from superclass
        Hyperedges myAbstractInterfacePartClassUids(instancesOf(Hyperedges{myAbstractInterfacePartUid},"",FORWARD));
        Hyperedges concreteInterfacePartClassUids(intersect(validConcreteInterfaceClassUids, implementationsOf(myAbstractInterfacePartClassUids)));
        UniqueId uid;
        if (concreteInterfacePartClassUids.empty())
        {
            // If none given, generate it or ignore it
            if (hook.ask("B. No concrete interface found for "+access(myAbstractInterfacePartClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(myAbstractInterfacePartClassUids[0], uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfacePartClassUids)
            {
                if (hook.ask("B. Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
                {
                    uid = ifUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractInterfacePartUids = unite(myAbstractInterfacePartUids, Hyperedges{myAbstractInterfacePartUid});
        myInterfacePartUids = unite(myInterfacePartUids, instantiateFrom(Hyperedges{uid}, access(myAbstractInterfacePartUid).label()));
    }

    // Define either a plain or a composite type
    if (myInterfacePartUids.empty())
    {
        // plain type
        std::string type(hook.ask("C. Please provide a built-in VHDL type for interface class " + name));
        result << "subtype " << name << " is " << type << ";";
    } else {
        // composite type
        result << "type " << name << " is record\n";
        result << "end record;";
    }

    Hyperedges newInterfaceClassUid(createInterface(concreteInterfaceClassUid, result.str(), Hyperedges{ifClassUid}));
    encodes(newInterfaceClassUid, Hyperedges{abstractInterfaceClassUid});
    isA(newInterfaceClassUid, validConcreteIfSuperclassUids);
    partOfInterface(myInterfacePartUids, newInterfaceClassUid);
    return newInterfaceClassUid;
}


Hyperedges VHDLGenerator::generateImplementationClassFor(const UniqueId& algorithmClassUid, const UniqueId& concreteImplementationClassUid, const GeneratorHook& hook)
{
    /*
        Here we want to generate a complete VHDL entity
        The template looks like this:
        -- include standard libs
        -- package def --
        package <name>_types is
        <interface-type-def>
        ...
        end package <name>_types;
        -- include standard libs again
        use work.<name>_types.all;
        entity <name> is;
        port(
        <input-name> : in <input-type-def>;
        ...
        <output-name> : in <output-type-def>;
        ...
        <interface-name> : inout <interface-type-def>;
        ...
        clk : in std_logic;
        rst : in std_logic
        );
        end;
        architecture BEHAVIOURAL of <name> is
        -- signals
        -- part signals
        -- top-lvl inputs to internal inputs
        -- internal outputs to top-lvl outputs
        -- part instantiation & wiring
        -- main process skeleton (atomic entities only)
        end BEHAVIOURAL;

        NOTES:
        * VHDL does not support inheritance. But we can do it, by collecting ALL interfaces, including the interfaces of the superclasses!
        * Also, if we have valid direct superclasses, we have to 1) instantiate them (like parts) 2) wire them to the corresponding interfaces
        * Currently, we have strings for interface initialization ... it is open how to automatically convert these values. For now, we ask the user.
          To automatize this, we could provide a special GeneratorHook which tries to automatically handle these strings for some cases.
    */

    std::stringstream result;
    Hyperedges validConcreteInterfaceClassUids(concreteInterfaceClasses());
    Hyperedges validImplementationClassUids(concreteImplementationClasses());
    // TODO: If the implementations of the superclasses have been generated already, they would have inherited the interfaces of the direct superclasses already!!!
    // So, we would NOT need to use subclassesOf here? However, for the interfaces we still have to look at every superclass, right?
    Hyperedges _algorithmSuperclassUids(subclassesOf(Hyperedges{algorithmClassUid}, "", FORWARD)); // For VHDL, we have to do the inheritance!

    std::string name(access(algorithmClassUid).label());

    // Collect all valid superclass info
    Hyperedges algorithmSuperclassUids;
    Hyperedges myImplementationSuperclassUids;
    for (const UniqueId& algorithmSuperclassUid : _algorithmSuperclassUids)
    {
        // Exclude algorithmClassUid to prevent infinite loop
        if (algorithmSuperclassUid == algorithmClassUid)
            continue;
        // Get concrete implementation
        Hyperedges implementationSuperclassUids(intersect(validImplementationClassUids, implementationsOf(Hyperedges{algorithmSuperclassUid})));
        UniqueId uid;
        if (implementationSuperclassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("1. No concrete implementation found for "+access(algorithmSuperclassUid).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateImplementationClassFor(algorithmSuperclassUid, uid, hook);
                validImplementationClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& implUid : implementationSuperclassUids)
            {
                if (hook.ask("1. Use concrete implementation "+access(implUid).label()+"? [y/n]") == "y")
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
    if (!algorithmSuperclassUids.empty())
        _myAbstractInterfaceUids = unite(_myAbstractInterfaceUids, interfacesOf(algorithmSuperclassUids));
    Hyperedges myAbstractInterfaceUids;
    Hyperedges myConcreteInterfaceUids;
    Hyperedges myConcreteInterfaceClassUids;
    for (const UniqueId& myAbstractInterfaceUid : _myAbstractInterfaceUids)
    {
        // Get concrete interface
        Hyperedges myAbstractInterfaceClassUids(instancesOf(Hyperedges{myAbstractInterfaceUid},"",FORWARD));
        Hyperedges concreteInterfaceClassUids(intersect(validConcreteInterfaceClassUids, encodersOf(Hyperedges{myAbstractInterfaceClassUids})));
        UniqueId uid;
        if (concreteInterfaceClassUids.empty())
        {
            // If none exists, ignore or generate it
            if (hook.ask("2. No concrete interface found for "+access(myAbstractInterfaceClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateConcreteInterfaceClassFor(myAbstractInterfaceClassUids[0], uid, hook);
                validConcreteInterfaceClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& ifUid : concreteInterfaceClassUids)
            {
                if (hook.ask("2. Use concrete interface "+access(ifUid).label()+"? [y/n]") == "y")
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
    // NOTE: To establish inheritance, we have to make the superclasses become PARTS and wire them to the corresponding interfaces (done later)
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
            if (hook.ask("3. No concrete implementation found for "+access(myAbstractPartClassUids[0]).label()+". Generate it? [y/n]") == "y")
            {
                uid = hook.ask("Please provide a unique id");
                while (exists(uid))
                    uid = hook.ask(uid + " already exists. Provide a different uid");
                generateImplementationClassFor(myAbstractPartClassUids[0], uid, hook);
                validImplementationClassUids.push_back(uid);
            }
        } else {
            for (const UniqueId& implUid : implementationClassUids)
            {
                if (hook.ask("3. Use concrete implementation "+access(implUid).label()+"? [y/n]") == "y")
                {
                    uid = implUid;
                    break;
                }
            }
        }
        if (uid.empty())
            continue;
        myAbstractPartUids = unite(myAbstractPartUids, Hyperedges{myAbstractPartUid});
        myImplementationPartUids = unite(myImplementationPartUids, instantiateComponent(Hyperedges{uid}, access(myAbstractPartUid).label()));
    }

    // Collect valid direct superclasses and interface info
    Hyperedges myAbstractDirectSuperclassUids(intersect(algorithmSuperclassUids, directSubclassesOf(Hyperedges{algorithmClassUid},"",FORWARD)));
    Hyperedges myAbstractInputUids(intersect(myAbstractInterfaceUids, inputsOf(algorithmSuperclassUids)));
    Hyperedges myAbstractOutputUids(intersect(myAbstractInterfaceUids, outputsOf(algorithmSuperclassUids)));
    Hyperedges myAbstractIOUids(subtract(myAbstractInterfaceUids, unite(myAbstractInputUids, myAbstractOutputUids)));

    // PACKAGE DEF
    result << "-- " << name << "_VHDL_entity\n";
    result << "library IEEE;\n";
    result << "use IEEE.STD_LOGIC_1164.ALL;\n";
    result << "\n-- Package def --\n";
    result << "package " << name << "_types is\n";
    // TODO: Remove duplicates
    for (const UniqueId& myConcreteInterfaceClassUid : myConcreteInterfaceClassUids)
    {
        result << access(myConcreteInterfaceClassUid).label() << "\n";
    }
    result << "end " << name << "_types;\n";
    // INCLUDE LIBS & PACKAGE
    result << "library IEEE;\n";
    result << "use IEEE.STD_LOGIC_1164.ALL;\n";
    // TODO: Do we need to use package defs of parts as well?
    result << "use work." << name << "_types.all;\n";
    // ENTITY DEF
    result << "\nentity " << name << " is\n";
    result << "port(\n";
    result << "\n\t-- Inputs --\n";
    for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
    {
        Hyperedges myAbstractInputClassUids(instancesOf(Hyperedges{myAbstractInputUid},"",FORWARD));
        for (const UniqueId& myAbstractInputClassUid : myAbstractInputClassUids)
        {
            result << "\t" << access(myAbstractInputUid).label() << " : in " << access(myAbstractInputClassUid).label() << ";\n";
        }
    }
    result << "\n\t-- Outputs --\n";
    for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
    {
        Hyperedges myAbstractOutputClassUids(instancesOf(Hyperedges{myAbstractOutputUid},"",FORWARD));
        for (const UniqueId& myAbstractOutputClassUid : myAbstractOutputClassUids)
        {
            result << "\t" << access(myAbstractOutputUid).label() << " : out " << access(myAbstractOutputClassUid).label() << ";\n";
        }
    }
    result << "\n\t-- Bidirectional Signals --\n";
    for (const UniqueId& myAbstractIOUid : myAbstractIOUids)
    {
        Hyperedges myAbstractIOClassUids(instancesOf(Hyperedges{myAbstractIOUid},"",FORWARD));
        for (const UniqueId& myAbstractIOClassUid : myAbstractIOClassUids)
        {
            result << "\t" << access(myAbstractIOUid).label() << " : inout " << access(myAbstractIOClassUid).label() << ";\n";
        }
    }
    result << "\n\t-- Standard Signals --\n";
    result << "\tclk : in std_logic;\n";
    result << "\trst : in std_logic\n";
    result << ");\n";
    result << "end;\n";
    // ARCHITECTURE DEF
    result << "\n-- Architecture def --\n";
    result << "architecture BEHAVIOURAL of " << name << " is\n";
    if (myAbstractPartUids.empty() && myAbstractDirectSuperclassUids.empty())
    {
        result << "-- signals here --\n";
        result << "\nbegin\n";
        result << "-- processes here --\n";
        result << "main : process(clk)\n";
        result << "\t-- variables here --\n";
        result << "\tbegin\n";
        result << "\t\tif rising_edge(clk) then\n";
        result << "\t\t\tif (rst='1') then\n";
        result << "\t\t\t\t-- init here --\n";
        result << "\t\t\telse\n";
        result << "\t\t\t\t-- computation here --\n";
        result << "\t\t\tend if;\n";
        result << "\t\tend if;\n";
        result << "end process main;\n";
    } else {
        result << "\n-- signals\n";
        result << "\n-- signals of parts\n";
        for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
        {
            Hyperedges myAbstractPartInterfaceUids(interfacesOf(Hyperedges{myAbstractPartUid}));
            for (const UniqueId& myAbstractPartInterfaceUid : myAbstractPartInterfaceUids)
            {
                Hyperedges myAbstractPartInterfaceClassUids(instancesOf(Hyperedges{myAbstractPartInterfaceUid}, "", FORWARD));
                for (const UniqueId& myAbstractPartInterfaceClassUid : myAbstractPartInterfaceClassUids)
                {
                    result << "signal " << access(myAbstractPartUid).label() << "_" << access(myAbstractPartInterfaceUid).label();
                    result << " : " << access(myAbstractPartInterfaceClassUid).label() << ";\n";
                    // TODO: Add values?
                }
            }
        }
        result << "\nbegin\n";
        result << "\n-- assignment of toplvl inputs to internal inputs --\n";
        for (const UniqueId& myAbstractInputUid : myAbstractInputUids)
        {
            Hyperedges myOriginalAbstractInputUids(originalInterfacesOf(Hyperedges{myAbstractInputUid}));
            for (const UniqueId& myOriginalAbstractInputUid : myOriginalAbstractInputUids)
            {
                Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractInputUid},"",INVERSE));
                for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
                {
                    result << access(myAbstractInternalPartUid).label() << "_" << access(myOriginalAbstractInputUid).label();
                    result << " <= ";
                    result << access(myAbstractInputUid).label();
                    result << ";\n";
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
                    result << access(myAbstractInternalPartUid).label() << "_" << access(myOriginalAbstractInputUid).label();
                    result << " <= ";
                    result << access(myAbstractIOUid).label();
                    result << ";\n";
                }
            }
        }
        result << "\n-- assignment of internal outputs to toplvl outputs --\n";
        for (const UniqueId& myAbstractOutputUid : myAbstractOutputUids)
        {
            Hyperedges myOriginalAbstractOutputUids(originalInterfacesOf(Hyperedges{myAbstractOutputUid}));
            for (const UniqueId& myOriginalAbstractOutputUid : myOriginalAbstractOutputUids)
            {
                Hyperedges myAbstractInternalPartUids(interfacesOf(Hyperedges{myOriginalAbstractOutputUid},"",INVERSE));
                for (const UniqueId& myAbstractInternalPartUid : myAbstractInternalPartUids)
                {
                    result << access(myAbstractOutputUid).label();
                    result << " <= ";
                    result << access(myAbstractInternalPartUid).label() << "_" << access(myOriginalAbstractOutputUid).label();
                    result << ";\n";
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
                    result << access(myAbstractIOUid).label();
                    result << " <= ";
                    result << access(myAbstractInternalPartUid).label() << "_" << access(myOriginalAbstractOutputUid).label();
                    result << ";\n";
                }
            }
        }
        result << "\n-- part entity instantiation & wiring--\n";
        for (const UniqueId& myAbstractPartUid : myAbstractPartUids)
        {
            Hyperedges myAbstractPartInterfaceUids(interfacesOf(Hyperedges{myAbstractPartUid}));
            for (const UniqueId& myAbstractPartInterfaceUid : myAbstractPartInterfaceUids)
            {
                Hyperedges abstractEndpointInterfaceUids(endpointsOf(Hyperedges{myAbstractPartInterfaceUid}));
                for (const UniqueId& abstractEndpointInterfaceUid : abstractEndpointInterfaceUids)
                {
                    Hyperedges abstractEndpointPartUids(interfacesOf(Hyperedges{abstractEndpointInterfaceUid}, "", INVERSE));
                    for (const UniqueId& abstractEndpointPartUid : abstractEndpointPartUids)
                    {
                        result << access(abstractEndpointPartUid).label() << "_" << access(abstractEndpointInterfaceUid).label();
                        result << " <= ";
                        result << access(myAbstractPartUid).label() << "_" << access(myAbstractPartInterfaceUid).label();
                        result << ";\n";
                    }
                }
            }
            Hyperedges myAbstractPartClassUids(instancesOf(Hyperedges{myAbstractPartUid},"",FORWARD));
            for (const UniqueId& myAbstractPartClassUid : myAbstractPartClassUids)
            {
                result << access(myAbstractPartUid).label() << ": entity work." << access(myAbstractPartClassUid).label() << "\n";
                result << "port map (\n";
                // Map interfaces to signals
                for (const UniqueId& myAbstractPartInterfaceUid : myAbstractPartInterfaceUids)
                {
                    result << "\t";
                    result << access(myAbstractPartInterfaceUid).label();
                    result << " => ";
                    result << access(myAbstractPartUid).label() << "_" << access(myAbstractPartInterfaceUid).label();
                    result << ",\n";
                }
                result << "\tclk => clk,\n";
                result << "\trst => rst\n";
                result << ");\n";
            }
        }

        result << "\n-- direct superclass entity instantiation & wiring--\n";
        for (const UniqueId& myAbstractDirectSuperclassUid : myAbstractDirectSuperclassUids)
        {
            Hyperedges superDuperUids(intersect(algorithmSuperclassUids, subclassesOf(Hyperedges{myAbstractDirectSuperclassUid},"",FORWARD)));
            Hyperedges myAbstractDirectSuperclassInterfaceUids(interfacesOf(superDuperUids));
            // TODO: What name do we assign to the superclass part?
            result << "TODO" << ": entity work." << access(myAbstractDirectSuperclassUid).label() << "\n";
            result << "port map (\n";
            // We wire the superclass directly through port mapping
            for (const UniqueId& myAbstractDirectSuperclassInterfaceUid : myAbstractDirectSuperclassInterfaceUids)
            {
                result << "\t";
                result << access(myAbstractDirectSuperclassInterfaceUid).label();
                result << " => ";
                result << access(myAbstractDirectSuperclassInterfaceUid).label();
                result << ",\n";
            }
            result << "\tclk => clk,\n";
            result << "\trst => rst\n";
            result << ");\n";
            // TODO: We have to instantiate the superclass as a part
        }
    }
    result << "end BEHAVIOURAL;\n";

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
