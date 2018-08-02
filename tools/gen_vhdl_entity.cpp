#include "SoftwareGraph.hpp"
#include "HypergraphYAML.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <getopt.h>

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"uid", required_argument, 0, 'u'},
    {"label", required_argument, 0, 'l'},
    {"type-uid", required_argument, 0, 't'},
    {"generate-files", no_argument, 0, 'g'},
    {"overwrite", no_argument, 0, 'o'},
    {0,0,0,0}
};

void usage (const char *myName)
{
    std::cout << "Usage:\n";
    std::cout << myName << " --uid=<UID> --label=<label> <yaml-file-in> <yaml-file-out>\n\n";
    std::cout << "Options:\n";
    std::cout << "--help\t" << "Show usage\n";
    std::cout << "--uid=<uid>\t" << "Specify the algorithm to be used to generate code by UID\n";
    std::cout << "--label=<label>\t" << "Specify the algorithm(s) to be used to generate code by label\n";
    std::cout << "--type-uid=<uid>\t" << "Specify the datatype class which hosts compatible types\n";
    std::cout << "--generate-files\t" << "If given, the generator will produce the file(s) needed for compilation\n";
    std::cout << "--overwrite\t" << "If given, the generator will overwrite existing implementation(s) with the same uid\n";
    std::cout << "\nExample:\n";
    std::cout << myName << " --label=\"MyAlgorithm\" initial_model.yml new_model.yml\n";
}

std::string sanitizeString(const std::string& in)
{
    // Sanitation of strings to be VHDL compatible
    std::string result(in);
    std::size_t index(0);
    while (true)
    {
        index = result.find(":", index);
        if (index == std::string::npos) break;
        result.replace(index, 1, "_");
        index++;
    }
    index = 0;
    while (true)
    {
        index = result.find(";", index);
        if (index == std::string::npos) break;
        result.replace(index, 1, "_");
        index++;
    }
    index = 0;
    while (true)
    {
        index = result.find(".", index);
        if (index == std::string::npos) break;
        result.replace(index, 1, "_");
        index++;
    }
    return result;
}

std::string genTypeFromLabel(const std::string& label)
{
    return sanitizeString(label)+"_type";
}

std::string genPartIdentifier(const UniqueId& partUid)
{
    return "component_"+sanitizeString(partUid);
}

std::string genInterfaceIdentifier(const std::string& label)
{
    return "interface_"+sanitizeString(label);
}

std::string genInputIdentifier(const std::string& label)
{
    return "input_"+sanitizeString(label);
}

std::string genOutputIdentifier(const std::string& label)
{
    return "output_"+sanitizeString(label);
}

bool isVHDLSubType(const std::string& type)
{
    // Unfortunately, if we subtype built-in types, we have to use 'subtype' otherwise we need to use 'type'
    // When we have an array, we have to use 'type'
    if (type.find("array") != std::string::npos)
        return false;
    // When we have some built-in type but no array, we have to use 'subtype'
    if (type.find("bit") != std::string::npos)
        return true;
    if (type.find("std_logic") != std::string::npos)
        return true;
    if (type.find("bit_vector") != std::string::npos)
        return true;
    if (type.find("std_logic_vector") != std::string::npos)
        return true;
    if (type.find("integer") != std::string::npos)
        return true;
    if (type.find("boolean") != std::string::npos)
        return true;
    // When we have no array and no built-in type, we have to use 'type' as well (enum)
    return false;
}

int main (int argc, char **argv)
{
    std::ofstream fout;
    bool generateFiles = false;
    bool overwrite = false;
    UniqueId uid, vhdlDatatypeUid;
    std::string label;

    // Say hello :)
    std::cout << "VHDL entity generator for algorithms\n";

    // Parse command line
    int c;
    while (1)
    {
        int option_index = 0;
        c = getopt_long(argc, argv, "h", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'o':
                overwrite = true;
                break;
            case 'g':
                generateFiles = true;
                break;
            case 't':
                vhdlDatatypeUid=std::string(optarg);
                break;
            case 'u':
                uid=std::string(optarg);
                break;
            case 'l':
                label=std::string(optarg);
                break;
            case 'h':
            case '?':
                break;
            default:
                std::cout << "W00t?!\n";
                return 1;
        }
    }

    if ((argc - optind) < 2)
    {
        usage(argv[0]);
        return 1;
    }

    // Set vars
    std::string fileNameIn(argv[optind]);
    std::string fileNameOut(argv[optind+1]);
    Software::Graph swgraph(YAML::LoadFile(fileNameIn).as<Hypergraph>());

    // Get all algorithm classes (without overall superclass)
    // and find the candidate(s)
    Hyperedges algorithms(swgraph.algorithmClasses(label));
    if (!uid.empty())
        algorithms = intersect(algorithms, Hyperedges{uid});

    if (!algorithms.size())
    {
        std::cout << "No algorithm found.\n";
        return 2;
    }

    // Set our language
    const UniqueId& vhdlImplementationUid("Software::Graph::Implementation::VHDL");
    swgraph.createImplementation(vhdlImplementationUid, "VHDLImplementation");

    // Find relevant datatypeClasses
    Hyperedges relevantTypeUids;
    if (!vhdlDatatypeUid.empty())
        relevantTypeUids = swgraph.datatypeClasses("",Hyperedges{vhdlDatatypeUid});
    else
        relevantTypeUids = swgraph.datatypeClasses();

    // Get some constants
    Hyperedges allInputUids(swgraph.inputs());
    Hyperedges allOutputUids(swgraph.outputs());
    Hyperedges allAlgorithmClasses(swgraph.algorithmClasses());
    Hyperedges allImplementationClasses(swgraph.implementationClasses("",Hyperedges{vhdlImplementationUid}));

    // For each of these algorithms
    for (const UniqueId& algorithmId : algorithms)
    {
        Hyperedges myInterfaceClassIds;
        std::stringstream result;
        Hyperedge*  algorithm(swgraph.get(algorithmId));
        std::cout << "Generating code for " << algorithm->label() << "\n";

        // Check early if implementation exists and overwrite is not desired
        const UniqueId& implId(vhdlImplementationUid+"::"+algorithm->label());
        bool implFound = (std::find(allImplementationClasses.begin(), allImplementationClasses.end(), implId) != allImplementationClasses.end());
        if (implFound && !overwrite)
        {
            std::cout << "Implementation for " << algorithm->label() << "already exists. Skipping\n";
            continue;
        }
        if (implFound && overwrite)
        {
            // TODO: delete class and all its parts
            std::cout << "Overwrite mechanism not implemented yet :(\n";
            continue;
        }

        // Find interfaces
        Hyperedges interfaceUids(swgraph.interfacesOf(Hyperedges{algorithmId}));
        Hyperedges interfaceClassUids(swgraph.instancesOf(interfaceUids,"",Hypergraph::TraversalDirection::FORWARD));
        Hyperedges inputUids(intersect(allInputUids, interfaceUids));
        Hyperedges outputUids(intersect(allOutputUids, interfaceUids));

        // Find my superclasses
        Hyperedges mySuperclasses(intersect(allAlgorithmClasses, swgraph.directSubclassesOf(Hyperedges{algorithmId}, "", Hypergraph::TraversalDirection::FORWARD)));

        // Find subcomponents
        Hyperedges parts(swgraph.componentsOf(Hyperedges{algorithmId}));
        Hyperedges superclasses(swgraph.instancesOf(parts,"",Hypergraph::TraversalDirection::FORWARD));

        // In VHDL we have no intrinsic inheritance, so we have to inherit the interfaces manually by checking all superclass interfaces as well!
        interfaceUids = unite(interfaceUids, swgraph.interfacesOf(mySuperclasses));
        interfaceClassUids = unite(interfaceClassUids, swgraph.instancesOf(interfaceUids,"",Hypergraph::TraversalDirection::FORWARD));
        inputUids = unite(inputUids, intersect(allInputUids, interfaceUids));
        outputUids = unite(outputUids, intersect(allOutputUids, interfaceUids));

        result << "-- Algorithm to VHDL entity generator --\n";
        result << "library IEEE;\n";
        result << "use IEEE.STD_LOGIC_1164.ALL;\n";

        // Handle the collected interface classes
        result << "\n-- Package def --\n";
        result << "package " << sanitizeString(algorithm->label()) << "_types is\n";
        for (const UniqueId& interfaceClassId : interfaceClassUids)
        {
            Hyperedge* interface(swgraph.get(interfaceClassId));
            Hyperedges typeUids(intersect(relevantTypeUids, swgraph.directSubclassesOf(Hyperedges{interfaceClassId},"",Hypergraph::TraversalDirection::INVERSE)));
            for (const UniqueId& typeUid : typeUids)
            {
                std::string datatypeName(sanitizeString(swgraph.get(typeUid)->label()));
                if (isVHDLSubType(datatypeName))
                    result << "\tsubtype " << genTypeFromLabel(interface->label()) << " is " << datatypeName << ";\n";
                else
                    result << "\ttype " << genTypeFromLabel(interface->label()) << " is " << datatypeName << ";\n";
            }
        }
        result << "end " << sanitizeString(algorithm->label()) << "_types;\n";

        // Reinsert library statement because the previous one only applies to package def
        result << "library IEEE;\n";
        result << "use IEEE.STD_LOGIC_1164.ALL;\n";
        result << "use work." << sanitizeString(algorithm->label()) << "_types.all;\n";
        result << "\nentity " << sanitizeString(algorithm->label()) << " is\n";
        result << "port(\n";

        // Handle Inputs (input instances which are interfaces of algorithmId
        result << "\n\t-- Inputs --\n";
        for (const UniqueId& inputId : inputUids)
        {
            // This input is of a certain type we have to find
            Hyperedges inputClassUids(swgraph.instancesOf(inputId,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& classUid : inputClassUids)
            {
                std::string typeOfInput(sanitizeString(swgraph.get(classUid)->label()));
                result << "\t" << genInputIdentifier(swgraph.get(inputId)->label()) << " : in " << genTypeFromLabel(typeOfInput) << ";\n";
            }
        }
        // Handle Outputs
        result << "\n\t-- Outputs --\n";
        for (const UniqueId& outputId : outputUids)
        {
            // This output is of a certain type we have to find
            Hyperedges outputClassUids(swgraph.instancesOf(outputId,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& classUid : outputClassUids)
            {
                std::string typeOfOutput(sanitizeString(swgraph.get(classUid)->label()));
                result << "\t" << genOutputIdentifier(swgraph.get(outputId)->label()) << " : out " << genTypeFromLabel(typeOfOutput) << ";\n";
            }
        }
        result << "\n\t-- Standard Signals --\n";
        //result << "\tstart : in std_logic;\n";
        //result << "\tvalid : out std_logic;\n";
        result << "\tclk : in std_logic;\n";
        result << "\trst : in std_logic\n";
        result << ");\n";
        result << "end;\n";

        // Handle architecture
        Hyperedges partImplementations;
        if (!parts.size())
        {
            // Architecture for atomic entity
            result << "\n-- Architecture def --\n";
            result << "architecture BEHAVIOURAL of " << sanitizeString(algorithm->label()) << " is\n";
            result << "-- signals here --\n";
            result << "\nbegin\n";
            result << "-- processes here --\n";
            result << "compute : process(clk)\n";
            result << "\t-- variables here --\n";
            result << "\tbegin\n";
            result << "\t\tif rising_edge(clk) then\n";
            result << "\t\t\tif (rst='1') then\n";
            result << "\t\t\t\t-- init here --\n";
            result << "\t\t\telse\n";
            result << "\t\t\t\t-- computation here --\n";
            result << "\t\t\tend if;\n";
            result << "\t\tend if;\n";
            result << "end process compute;\n";
            result << "end BEHAVIOURAL;\n";
        } else {
            // * For each component, create ONE signal per input and ONE signal per OUTPUT!!!!
            result << "\n-- Architecture def --\n";
            result << "architecture BEHAVIOURAL of " << sanitizeString(algorithm->label()) << " is\n";
            result << "-- signals here --\n";
            // I: Create signals for each component
            result << "-- signals of parts --\n";
            for (const UniqueId& partUid : parts)
            {
                Hyperedges partInputUids(intersect(allInputUids, swgraph.interfacesOf(Hyperedges{partUid})));
                for (const UniqueId& partInputUid : partInputUids)
                {
                    Hyperedges interfaceClassUids(swgraph.instancesOf(partInputUid,"",Hypergraph::TraversalDirection::FORWARD));
                    for (const UniqueId& classUid : interfaceClassUids)
                    {
                        result << "signal ";
                        result << genPartIdentifier(partUid) << "_" << genInputIdentifier(swgraph.get(partInputUid)->label());
                        result << " : " << genTypeFromLabel(swgraph.get(classUid)->label()) << ";\n";
                    }
                }
                Hyperedges partOutputUids(intersect(allOutputUids, swgraph.interfacesOf(Hyperedges{partUid})));
                for (const UniqueId& partOutputUid : partOutputUids)
                {
                    Hyperedges interfaceClassUids(swgraph.instancesOf(partOutputUid,"",Hypergraph::TraversalDirection::FORWARD));
                    for (const UniqueId& classUid : interfaceClassUids)
                    {
                        result << "signal ";
                        result << genPartIdentifier(partUid) << "_" << genOutputIdentifier(swgraph.get(partOutputUid)->label());
                        result << " : " << genTypeFromLabel(swgraph.get(classUid)->label()) << ";\n";
                    }
                }
            }
            result << "\nbegin\n";
            // II. Wire toplvl inputs to internal inputs
            result << "-- assignment of toplvl inputs to internal inputs --\n";
            for (const UniqueId& inputId : inputUids)
            {
                Hyperedges internalInputs(intersect(allInputUids, swgraph.originalInterfacesOf(Hyperedges{inputId})));
                for (const UniqueId& internalInputId : internalInputs)
                {
                    Hyperedges partUids(intersect(parts, swgraph.interfacesOf(Hyperedges{internalInputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                    for (const UniqueId& partUid : partUids)
                    {
                        result << genPartIdentifier(partUid) << "_" << genInputIdentifier(swgraph.get(internalInputId)->label());
                        result << " <= ";
                        result << genInputIdentifier(swgraph.get(inputId)->label());
                        result << ";\n";
                    }
                }
            }
            // III. Wire internal outputs to toplvl outputs
            result << "-- assignment of internal outputs to toplvl outputs --\n";
            for (const UniqueId& outputId : outputUids)
            {
                Hyperedges internalOutputs(intersect(allOutputUids, swgraph.originalInterfacesOf(Hyperedges{outputId})));
                for (const UniqueId& internalOutputId : internalOutputs)
                {
                    Hyperedges partUids(intersect(parts, swgraph.interfacesOf(Hyperedges{internalOutputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                    for (const UniqueId& partUid : partUids)
                    {
                        result << genOutputIdentifier(swgraph.get(outputId)->label());
                        result << " <= ";
                        result << genPartIdentifier(partUid) << "_" << genOutputIdentifier(swgraph.get(internalOutputId)->label());
                        result << ";\n";
                    }
                }
            }
            // IV. Instantiate and wire parts
            result << "-- part entity instantiation & wiring--\n";
            for (const UniqueId& partUid : parts)
            {
                Hyperedges partInterfaceUids(swgraph.interfacesOf(Hyperedges{partUid}));
                Hyperedges partInputUids(intersect(allInputUids, partInterfaceUids));
                Hyperedges partOutputUids(intersect(allOutputUids, partInterfaceUids));
                Hyperedges superclasses(swgraph.instancesOf(partUid,"",Hypergraph::TraversalDirection::FORWARD));
                // V. Assign component input signals to other components output signals
                result << "-- assignment of internal outputs to internal inputs --\n";
                for (const UniqueId& partInputUid : partInputUids)
                {
                    Hyperedges partOutputUids(swgraph.endpointsOf(Hyperedges{partInputUid},"",Hypergraph::TraversalDirection::INVERSE));
                    for (const UniqueId& partOutputUid : partOutputUids)
                    {
                        Hyperedges producerUids(intersect(parts, swgraph.interfacesOf(Hyperedges{partOutputUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                        for (const UniqueId& producerUid : producerUids)
                        {
                            result << genPartIdentifier(partUid) << "_" << genInputIdentifier(swgraph.get(partInputUid)->label());
                            result << " <= ";
                            result << genPartIdentifier(producerUid) << "_" << genOutputIdentifier(swgraph.get(partOutputUid)->label());
                            result << ";\n";
                        }
                    }
                }
                result << "-- instantiate entity/-ies --\n";
                for (const UniqueId& superUid : superclasses)
                {
                    Hyperedges partImplementationclassUids(intersect(allImplementationClasses, swgraph.directSubclassesOf(Hyperedges{superUid})));
                    if (!partImplementationclassUids.size())
                    {
                        // TODO: we could add it to the list of algorithms to be generated? :)
                        std::cout << "No implementation found for " << swgraph.get(superUid)->label() << ". Skipping\n";
                        continue;
                    }
                    for (const UniqueId& implClassUid : partImplementationclassUids)
                    {
                        result << genPartIdentifier(partUid) << ": entity work." << sanitizeString(swgraph.get(superUid)->label()) << "\n";
                        result << "port map (\n";
                        result << "\t-- inputs --\n";
                        // VI. Wire to corresponding signals
                        for (const UniqueId& partInputUid : partInputUids)
                        {
                                result << "\t";
                                result << genInputIdentifier(swgraph.get(partInputUid)->label()) << " => ";
                                result << genPartIdentifier(partUid) << "_" << genInputIdentifier(swgraph.get(partInputUid)->label());
                                result << ",\n";
                        }
                        result << "\t-- outputs --\n";
                        for (const UniqueId& partOutputUid : partOutputUids)
                        {
                                result << "\t";
                                result << genOutputIdentifier(swgraph.get(partOutputUid)->label()) << " => ";
                                result << genPartIdentifier(partUid) << "_" << genOutputIdentifier(swgraph.get(partOutputUid)->label());
                                result << ",\n";
                        }
                        result << "\tclk => clk,\n";
                        result << "\trst => rst\n";
                        result << ");\n";

                        // instantiate implementation and register it to be made a part of the current implementation
                        Hyperedges partImplementationUid(swgraph.instantiateComponent(Hyperedges{implClassUid}, genPartIdentifier(partUid)));
                        partImplementations = unite(partImplementations, partImplementationUid);
                        // If desired, we will output the implementation to file
                        if (generateFiles)
                        {
                            // Skip file creation if it already exists
                            if (std::ifstream(sanitizeString(swgraph.get(superUid)->label())+".vhdl"))
                                continue;
                            std::cout << "Writing implementation of " << swgraph.get(superUid)->label() << " to file\n";
                            fout.open(sanitizeString(swgraph.get(superUid)->label())+".vhdl");
                            if(fout.good()) {
                                fout << swgraph.get(implClassUid)->label() << std::endl;
                            } else {
                                std::cout << "FAILED\n";
                            }
                            fout.close();
                        }
                    }
                }
            }
            result << "-- processes here --\n";
            result << "end BEHAVIOURAL;\n";
        }

        // Create implementation class (and register it for later use in allImplementationClasses)
        allImplementationClasses = unite(allImplementationClasses, swgraph.createImplementation(implId, result.str(), Hyperedges{vhdlImplementationUid}));
        swgraph.isA(Hyperedges{implId}, Hyperedges{algorithmId});
        // Make part implementations part of this implementation
        swgraph.partOfNetwork(partImplementations, Hyperedges{implId});

        // If desired, write implementation to file
        if (generateFiles)
        {
            std::cout << "Writing implementation of " << algorithm->label() << " to file\n";
            fout.open(sanitizeString(algorithm->label())+".vhdl");
            if(fout.good()) {
                fout << swgraph.get(implId)->label() << std::endl;
            } else {
                std::cout << "FAILED\n";
            }
            fout.close();
        }
    }

    // Store graph
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(swgraph) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}
