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

    // For each of these algorithms
    for (const UniqueId& algorithmId : algorithms)
    {
        Hyperedges myInterfaceClassIds;
        std::stringstream result;
        Hyperedge*  algorithm(swgraph.get(algorithmId));

        // Find interfaces
        Hyperedges interfaceUids(swgraph.interfacesOf(Hyperedges{algorithmId}));
        Hyperedges interfaceClassUids(swgraph.instancesOf(interfaceUids,"",Hypergraph::TraversalDirection::FORWARD));
        Hyperedges inputUids(intersect(swgraph.inputs(), interfaceUids));
        Hyperedges outputUids(intersect(swgraph.outputs(), interfaceUids));

        // Find subcomponents
        Hyperedges parts(swgraph.componentsOf(Hyperedges{algorithmId}));

        // PREAMBLE
        result << "// Algorithm to C++ generator\n";
        result << "#ifndef __" << algorithm->label() << "_HEADER\n";
        result << "#define __" << algorithm->label() << "_HEADER\n";
        result << "class " << algorithm->label() << " {\n";
        result << "\tpublic:\n";

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

        // Instantiate parts (to not confuse C++, partId is also included)
        result << "\n\t\t// Instantiate parts\n";
        for (const UniqueId& partId : parts)
        {
            Hyperedges superclasses(swgraph.instancesOf(partId,"",Hypergraph::TraversalDirection::FORWARD));
            for (const UniqueId& superUid : superclasses)
            {
                result << "\t\t" << swgraph.get(superUid)->label() << " " << swgraph.get(partId)->label() << partId << "\n";
            }
        }

        // Generate main function signature
        result << "\n\t\t// Generate main function\n";
        result << "\t\tbool operator () (\n";
        // Generate input arguments
        for (const UniqueId& inputId : inputUids)
        {
            Hyperedges inputClassUids(swgraph.instancesOf(Hyperedges{inputId},"",Hypergraph::TraversalDirection::FORWARD));
            std::string typeOfInput("UNDEFINED");
            if (!inputClassUids.size())
            {
                result << "\t\t\tconst " << genTypeFromLabel(typeOfInput) << "& " << swgraph.get(inputId)->label() << ",\n";
                continue;
            }
            for (const UniqueId& classUid : inputClassUids)
            {
                typeOfInput = swgraph.get(classUid)->label();
                result << "\t\t\tconst " << genTypeFromLabel(typeOfInput) << "& " << swgraph.get(inputId)->label() << ",\n";
            }
        }
        // Generate output arguments (pass-by-reference)
        for (const UniqueId& outputId : outputUids)
        {
            Hyperedges outputClassUids(swgraph.instancesOf(Hyperedges{outputId},"",Hypergraph::TraversalDirection::FORWARD));
            std::string typeOfOutput("UNDEFINED");
            if (!outputClassUids.size())
            {
                result << "\t\t\t" << genTypeFromLabel(typeOfOutput) << "& " << swgraph.get(outputId)->label() << ",\n";
                continue;
            }
            for (const UniqueId& classUid : outputClassUids)
            {
                typeOfOutput = swgraph.get(classUid)->label();
                result << "\t\t\t" << genTypeFromLabel(typeOfOutput) << "& " << swgraph.get(outputId)->label() << ",\n";
            }
        }
        // Close argument list (with a pointer to a context) and start creation of body
        result << "\t\t\tvoid *ctx)\n";
        result << "\t\t{\n";
        if (!parts.size())
        {
            // Generate a dummy
            result << "\t\t\t// Implement your algorithm here\n";
            result << "\t\t\t// Return true if evaluation has been performed\n";
            result << "\t\t\treturn false;\n";
        } else {
            // Since we have parts, we should call them.
            // That means, that we have to traverse the network of parts starting at inputs and cumulating at the outputs
            // THIS IS A BFS TRAVERSAL!
            result << "\t\t\t// TODO: Would do BFS traversal starting from parts at inputs to parts generating output\n";
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
