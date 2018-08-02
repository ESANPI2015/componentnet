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
    // Sanitation of strings to be C++ compatible
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
    return sanitizeString(label)+"_t";
}

std::string genPartIdentifier(const UniqueId& uid)
{
    return "component_"+sanitizeString(uid);
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

// For each algorithm:
// * Create interfaces & types
// * Check parts
//   - If no parts, create atomic class
//   - if parts, call each one but update inputs with connected outputs afterwards (synchronous update)
// * What we have is: an algorithmClass which has algorithmic instances as parts (or none)
//   What we want is: an implementationClass which has implementation instances matching the algorithmic instances class!!!!
// TODO: When we create the C++ interfaces ... why don't we add them as interfaces to the implementation class?
int main (int argc, char **argv)
{
    std::ofstream fout;
    bool generateFiles = false;
    bool overwrite = false;
    UniqueId uid, cppDatatypeUid;
    std::string label;


    // Say hello :)
    std::cout << "C++ class generator for algorithms\n";

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
                cppDatatypeUid=std::string(optarg);
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
    const UniqueId& cppImplementationUid("Software::Graph::Implementation::C++");
    swgraph.createImplementation(cppImplementationUid, "C++Implementation");

    // Find relevant datatypeClasses
    Hyperedges relevantTypeUids;
    if (!cppDatatypeUid.empty())
        relevantTypeUids = swgraph.datatypeClasses("",Hyperedges{cppDatatypeUid});
    else
        relevantTypeUids = swgraph.datatypeClasses();

    // Get some constants
    Hyperedges allInputUids(swgraph.inputs());
    Hyperedges allOutputUids(swgraph.outputs());
    Hyperedges allAlgorithmClasses(swgraph.algorithmClasses());
    Hyperedges allImplementationClasses(swgraph.implementationClasses("",Hyperedges{cppImplementationUid}));

    // For each of these algorithms
    for (const UniqueId& algorithmId : algorithms)
    {
        Hyperedges myInterfaceClassIds;
        std::stringstream result;
        Hyperedge*  algorithm(swgraph.get(algorithmId));

        std::cout << "Generating code for " << algorithm->label() << "\n";

        // Check early if implementation exists and overwrite is not desired
        const UniqueId& implId(cppImplementationUid+"::"+algorithm->label());
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

        // PREAMBLE
        result << "// Algorithm to C++ generator\n";
        result << "#ifndef __" << sanitizeString(algorithm->label()) << "_HEADER\n";
        result << "#define __" << sanitizeString(algorithm->label()) << "_HEADER\n";
        result << "#include <string>\n";
        result << "// Include headers for inheritance\n";
        for (const UniqueId& superclassUid : mySuperclasses)
        {
            result << "#include \"" << sanitizeString(swgraph.get(superclassUid)->label()) << ".hpp\"\n";
        }
        result << "// Include headers of parts\n";
        for (const UniqueId& superclassUid : superclasses)
        {
            result << "#include \"" << sanitizeString(swgraph.get(superclassUid)->label()) << ".hpp\"\n";
        }
        result << "class " << sanitizeString(algorithm->label()) << " : "; 
        Hyperedges::const_iterator it(mySuperclasses.begin());
        while (it != mySuperclasses.end())
        {
            result << "public " << sanitizeString(swgraph.get(*it)->label());
            it++;
            if (it != mySuperclasses.end())
                result << ", ";
        }
        result << "\n{\n";
        result << "\tpublic:\n";
        result << "\n\t\t// Constructor to initialize class\n";
        result << "\t\t" << sanitizeString(algorithm->label()) << "(const std::string& param=\"" << algorithm->label() << "\")\n";
        result << "\t\t{\n";
        result << "\t\t\t// Write your init code here\n";
        result << "\t\t}\n";

        // Handle the collected interface classes
        result << "\n\t\t// Generate interface types\n";
        for (const UniqueId& interfaceClassUid : interfaceClassUids)
        {
            Hyperedge* interface(swgraph.get(interfaceClassUid));
            Hyperedges typeUids(intersect(relevantTypeUids, swgraph.directSubclassesOf(Hyperedges{interfaceClassUid},"",Hypergraph::TraversalDirection::INVERSE)));
            for (const UniqueId& typeUid : typeUids)
            {
                std::string datatypeName(sanitizeString(swgraph.get(typeUid)->label()));
                result << "\t\ttypedef " << datatypeName << " " << genTypeFromLabel(interface->label()) << ";\n";
            }
        }

        // Generate input arguments
        result << "\n\t\t// Input variables\n";
        for (const UniqueId& inputId : inputUids)
        {
            Hyperedges inputClassUids(swgraph.instancesOf(Hyperedges{inputId},"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& classUid : inputClassUids)
            {
                std::string typeOfInput(swgraph.get(classUid)->label());
                result << "\t\t" << genTypeFromLabel(typeOfInput) << " " << genInputIdentifier(swgraph.get(inputId)->label()) << ";\n";
            }
        }
        // Generate output arguments
        result << "\n\t\t// Output variables\n";
        for (const UniqueId& outputId : outputUids)
        {
            Hyperedges outputClassUids(swgraph.instancesOf(Hyperedges{outputId},"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& classUid : outputClassUids)
            {
                std::string typeOfOutput(swgraph.get(classUid)->label());
                result << "\t\t" << genTypeFromLabel(typeOfOutput) << " " << genOutputIdentifier(swgraph.get(outputId)->label()) << ";\n";
            }
        }

        // Instantiate parts (to not confuse C++, partId is also included)
        result << "\n\t\t// Instantiate parts\n";
        Hyperedges partImplementations;
        for (const UniqueId& partId : parts)
        {
            Hyperedges partSuperclassUids(swgraph.instancesOf(partId,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& superUid : partSuperclassUids)
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
                    // Instantiate part from implementation
                    Hyperedges partImplementationUid(swgraph.instantiateComponent(Hyperedges{implClassUid}, genPartIdentifier(partId)));
                    result << "\t\t" << sanitizeString(swgraph.get(superUid)->label()) << " " << genPartIdentifier(partId) << ";\n";
                    partImplementations = unite(partImplementations, partImplementationUid);

                    // If desired, we will output the implementation to file
                    if (generateFiles)
                    {
                        // Skip file creation if it already exists
                        if (std::ifstream(sanitizeString(swgraph.get(superUid)->label())+".hpp"))
                            continue;
                        std::cout << "Writing implementation of " << swgraph.get(superUid)->label() << " to file\n";
                        fout.open(sanitizeString(swgraph.get(superUid)->label())+".hpp");
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

        // Generate main function signature
        result << "\n\t\t// Generate main function\n";
        result << "\t\tbool operator () (const std::string& param=\"" << algorithm->label() << "\")\n";
        result << "\t\t{\n";
        // Close argument list (with a pointer to a context) and start creation of body
        if (!parts.size())
        {
            // Generate a dummy
            result << "\t\t\t// Implement your algorithm here\n";
            result << "\t\t\t// Return true if evaluation has been performed\n";
            result << "\t\t\treturn false;\n";
        } else {
            // I. Copy values from my inputs to the input vars of my parts
            result << "\t\t\t// Pass input values to inputs of internal parts\n";
            for (const UniqueId& inputId : inputUids)
            {
                Hyperedges internalInputs(intersect(allInputUids, swgraph.originalInterfacesOf(Hyperedges{inputId})));
                for (const UniqueId& internalInputId : internalInputs)
                {
                    Hyperedges internalParts(intersect(parts, swgraph.interfacesOf(Hyperedges{internalInputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                    for (const UniqueId& internalPartUid : internalParts)
                    {
                        result << "\t\t\t";
                        result << genPartIdentifier(internalPartUid) << "." << genInputIdentifier(swgraph.get(internalInputId)->label())
                               << " = this->" << genInputIdentifier(swgraph.get(inputId)->label()) << ";\n";
                    }
                }
            }
            result << "\t\t\t// Evaluate internal parts\n";
            // II. Call every part with input vars, producing output vars
            for (const UniqueId& partUid : parts)
            {
                result << "\t\t\t";
                result << genPartIdentifier(partUid) << "(\"" << swgraph.get(partUid)->label() << "\");\n";
            }
            // III. For every edge: Copy output value to corresponding input value
            result << "\t\t\t// Copy results from parts to inputs of connected parts\n";
            for (const UniqueId& producerUid : parts)
            {
                Hyperedges producerOutputUids(intersect(allOutputUids, swgraph.interfacesOf(Hyperedges{producerUid})));
                for (const UniqueId& producerOutputUid : producerOutputUids)
                {
                    Hyperedges consumerInputUids(swgraph.endpointsOf(Hyperedges{producerOutputUid}));
                    for (const UniqueId& consumerInputUid : consumerInputUids)
                    {
                        Hyperedges consumerUids(intersect(parts, swgraph.interfacesOf(Hyperedges{consumerInputUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                        for (const UniqueId& consumerUid : consumerUids)
                        {
                            result << "\t\t\t";
                            result << genPartIdentifier(consumerUid) << "." << genInputIdentifier(swgraph.get(consumerInputUid)->label())
                                   << " = " << genPartIdentifier(producerUid) << "." << genOutputIdentifier(swgraph.get(producerOutputUid)->label()) << ";\n";
                        }
                    }
                }
            }
            // IV. Return output values
            result << "\t\t\t// Return the results from internal computation\n";
            for (const UniqueId& outputId : outputUids)
            {
                Hyperedges internalOutputs(intersect(allOutputUids, swgraph.originalInterfacesOf(Hyperedges{outputId})));
                for (const UniqueId& internalOutputId : internalOutputs)
                {
                    Hyperedges internalParts(intersect(parts, swgraph.interfacesOf(Hyperedges{internalOutputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                    for (const UniqueId& internalPartUid : internalParts)
                    {
                        result << "\t\t\t";
                        result << "this->" << genOutputIdentifier(swgraph.get(outputId)->label()) 
                               << " = " << genPartIdentifier(internalPartUid) << "." << genOutputIdentifier(swgraph.get(internalOutputId)->label()) << ";\n";
                    }
                }
            }
        }
        result << "\t\t}\n";

        // Close class def
        result << "};\n";
        result << "#endif\n";

        // Create implementation class (and register it for later use in allImplementationClasses)
        allImplementationClasses = unite(allImplementationClasses, swgraph.createImplementation(implId, result.str(), Hyperedges{cppImplementationUid}));
        swgraph.isA(Hyperedges{implId}, Hyperedges{algorithmId});
        // Make part implementations part of this implementation
        swgraph.partOfNetwork(partImplementations, Hyperedges{implId});

        // If desired, write implementation to file
        if (generateFiles)
        {
            std::cout << "Writing implementation of " << algorithm->label() << " to file\n";
            fout.open(sanitizeString(algorithm->label())+".hpp");
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
