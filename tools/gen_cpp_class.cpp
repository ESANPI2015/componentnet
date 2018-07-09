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
    std::cout << "\nExample:\n";
    std::cout << myName << "--label=MyAlgorithm initial_model.yml new_model.yml\n";
}

std::string genTypeFromLabel(const std::string& label)
{
    return label+"_t";
}

std::string genPartIdentifier(const UniqueId& uid)
{
    return "component_"+uid;
}

std::string genInterfaceIdentifier(const std::string& label)
{
    return "interface_"+label;
}

// For each algorithm:
// * Create interfaces & types
// * Check parts
//   - If no parts, create atomic class
//   - if parts, call each one in topological order/in BFS order from inputs to outputs TODO
int main (int argc, char **argv)
{
    UniqueId uid, cppDatatypeUid;
    std::string label;
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

    // For each of these algorithms
    for (const UniqueId& algorithmId : algorithms)
    {
        Hyperedges myInterfaceClassIds;
        std::stringstream result;
        Hyperedge*  algorithm(swgraph.get(algorithmId));

        // Find interfaces
        Hyperedges interfaceUids(swgraph.interfacesOf(Hyperedges{algorithmId}));
        Hyperedges interfaceClassUids(swgraph.instancesOf(interfaceUids,"",Hypergraph::TraversalDirection::FORWARD));
        Hyperedges inputUids(intersect(allInputUids, interfaceUids));
        Hyperedges outputUids(intersect(allOutputUids, interfaceUids));

        // Find subcomponents
        Hyperedges parts(swgraph.componentsOf(Hyperedges{algorithmId}));

        // PREAMBLE
        result << "// Algorithm to C++ generator\n";
        result << "#ifndef __" << algorithm->label() << "_HEADER\n";
        result << "#define __" << algorithm->label() << "_HEADER\n";
        // TODO: We need includes!!!!
        result << "class " << algorithm->label() << " {\n";
        result << "\tpublic:\n";
        result << "\n\t\t// Constructor to initialize class\n";
        result << "\t\tvoid " << algorithm->label() << "()\n";
        result << "\t\t{\n";
        result << "\t\t\t// Write your init code here\n";
        result << "\t\t}\n";

        // Handle the collected interface classes
        result << "\n\t\t// Generate interface types\n";
        for (const UniqueId& interfaceClassUid : interfaceClassUids)
        {
            Hyperedge* interface(swgraph.get(interfaceClassUid));
            std::string datatypeName("UNKNOWN");
            Hyperedges typeUids(intersect(relevantTypeUids, swgraph.directSubclassesOf(Hyperedges{interfaceClassUid},"",Hypergraph::TraversalDirection::INVERSE)));
            if (!typeUids.size())
            {
                result << "\t\ttypedef " << datatypeName << " " << genTypeFromLabel(interface->label()) << ";\n";
                continue;
            }
            for (const UniqueId& typeUid : typeUids)
            {
                datatypeName = swgraph.get(typeUid)->label();
                result << "\t\ttypedef " << datatypeName << " " << genTypeFromLabel(interface->label()) << ";\n";
            }
        }

        // Generate input arguments
        result << "\n\t\t// Input variables\n";
        for (const UniqueId& inputId : inputUids)
        {
            Hyperedges inputClassUids(swgraph.instancesOf(Hyperedges{inputId},"",Hypergraph::TraversalDirection::FORWARD));
            std::string typeOfInput("UNDEFINED");
            if (!inputClassUids.size())
            {
                result << "\t\t" << genTypeFromLabel(typeOfInput) << " " << genInterfaceIdentifier(swgraph.get(inputId)->label()) << ";\n";
                continue;
            }
            for (const UniqueId& classUid : inputClassUids)
            {
                typeOfInput = swgraph.get(classUid)->label();
                result << "\t\t" << genTypeFromLabel(typeOfInput) << " " << genInterfaceIdentifier(swgraph.get(inputId)->label()) << ";\n";
            }
        }
        // Generate output arguments
        result << "\n\t\t// Output variables\n";
        for (const UniqueId& outputId : outputUids)
        {
            Hyperedges outputClassUids(swgraph.instancesOf(Hyperedges{outputId},"",Hypergraph::TraversalDirection::FORWARD));
            std::string typeOfOutput("UNDEFINED");
            if (!outputClassUids.size())
            {
                result << "\t\t" << genTypeFromLabel(typeOfOutput) << " " << genInterfaceIdentifier(swgraph.get(outputId)->label()) << ";\n";
                continue;
            }
            for (const UniqueId& classUid : outputClassUids)
            {
                typeOfOutput = swgraph.get(classUid)->label();
                result << "\t\t" << genTypeFromLabel(typeOfOutput) << " " << genInterfaceIdentifier(swgraph.get(outputId)->label()) << ";\n";
            }
        }

        // Instantiate parts (to not confuse C++, partId is also included)
        result << "\n\t\t// Instantiate parts\n";
        for (const UniqueId& partId : parts)
        {
            Hyperedges superclasses(swgraph.instancesOf(partId,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& superUid : superclasses)
            {
                result << "\t\t" << swgraph.get(superUid)->label() << " " << genPartIdentifier(partId) << "\n";
            }
        }

        // Generate main function signature
        result << "\n\t\t// Generate main function\n";
        result << "\t\tbool operator () (void)\n";
        result << "\t\t{\n";
        // Close argument list (with a pointer to a context) and start creation of body
        if (!parts.size())
        {
            // Generate a dummy
            result << "\t\t\t// Implement your algorithm here\n";
            result << "\t\t\t// Return true if evaluation has been performed\n";
            result << "\t\t\treturn false;\n";
        } else {
            // I. Copy values from my inputs to the input vars of my parts (TODO: Follow aliasOf relations or get other owners)
            result << "\t\t\t// Copy input values to inputs of internal parts\n";
            for (const UniqueId& inputId : inputUids)
            {
                Hyperedges internalParts(intersect(parts, swgraph.interfacesOf(Hyperedges{inputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                for (const UniqueId& internalPartUid : internalParts)
                {
                    result << "\t\t\t";
                    result << genPartIdentifier(internalPartUid) << "." << genInterfaceIdentifier(swgraph.get(inputId)->label())
                           << " = this->" << genInterfaceIdentifier(swgraph.get(inputId)->label()) << ";\n";
                }
            }
            result << "\t\t\t// Evaluate internal parts\n";
            // II. Call every part with input vars, producing output vars
            for (const UniqueId& partUid : parts)
            {
                result << "\t\t\t";
                result << genPartIdentifier(partUid) << "();\n";
            }
            // III. For every edge: Copy output value to corresponding input value
            result << "\t\t\t// Copy results from parts to inputs of connected parts\n";
            for (const UniqueId& producerUid : parts)
            {
                Hyperedges producerOutputUids(intersect(allOutputUids, swgraph.interfacesOf(Hyperedges{producerUid})));
                for (const UniqueId& producerOutputUid : producerOutputUids)
                {
                    Hyperedges consumerInputUids(swgraph.endpointsOf(Hyperedges{producerOutputUid},"",Hypergraph::TraversalDirection::INVERSE));
                    for (const UniqueId& consumerInputUid : consumerInputUids)
                    {
                        Hyperedges consumerUids(intersect(parts, swgraph.interfacesOf(Hyperedges{consumerInputUid}, "", Hypergraph::TraversalDirection::INVERSE)));
                        for (const UniqueId& consumerUid : consumerUids)
                        {
                            result << "\t\t\t";
                            result << genPartIdentifier(consumerUid) << "." << genInterfaceIdentifier(swgraph.get(consumerInputUid)->label())
                                   << " = " << genPartIdentifier(producerUid) << "." << genInterfaceIdentifier(swgraph.get(producerOutputUid)->label()) << ";\n";
                        }
                    }
                }
            }
            // IV. Return output values (TODO: Follow aliasOf relations or get other owners)
            result << "\t\t\t// Return the results from internal computation\n";
            for (const UniqueId& outputId : outputUids)
            {
                Hyperedges internalParts(intersect(parts, swgraph.interfacesOf(Hyperedges{outputId}, "", Hypergraph::TraversalDirection::INVERSE)));
                for (const UniqueId& internalPartUid : internalParts)
                {
                    result << "\t\t\t";
                    result << "this->" << genInterfaceIdentifier(swgraph.get(outputId)->label()) 
                           << " = " << genPartIdentifier(internalPartUid) << "." << genInterfaceIdentifier(swgraph.get(outputId)->label()) << ";\n";
                }
            }
        }
        result << "\t\t}\n";

        // Close class def
        result << "};\n";
        result << "#endif\n";

        // Create implementation for algorithm with this result
        const UniqueId& implId(cppImplementationUid+"::"+algorithm->label());
        swgraph.createImplementation(implId, result.str(), Hyperedges{cppImplementationUid});
        swgraph.isA(Hyperedges{implId}, Hyperedges{algorithmId});
    }

    // Store graph
    std::ofstream fout;
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(swgraph) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}
