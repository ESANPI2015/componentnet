#include "Generator.hpp"
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
    std::cout << "--generate-files\t" << "If given, the generator will produce the file(s) needed for compilation\n";
    std::cout << "--overwrite\t" << "If given, the generator will overwrite existing implementation(s) with the same uid\n";
    std::cout << "\nExample:\n";
    std::cout << myName << " --label=\"MyAlgorithm\" initial_model.yml new_model.yml\n";
}

// For each algorithm:
// * Create interfaces & types
// * Check parts
//   - If no parts, create atomic class
//   - if parts, call each one but update inputs with connected outputs afterwards (synchronous update)
// * What we have is: an algorithmClass which has algorithmic instances as parts (or none)
//   What we want is: an implementationClass which has implementation instances matching the algorithmic instances class!!!!
int main (int argc, char **argv)
{
    std::ofstream fout;
    bool generateFiles = false;
    bool overwrite = false;
    UniqueId uid;
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
    Software::Generator gen(YAML::LoadFile(fileNameIn).as<Hypergraph>());

    // Get all algorithm classes (without overall superclass)
    // and find the candidate(s)
    Hyperedges algorithmUids(gen.algorithmClasses(label));
    algorithmUids = subtract(algorithmUids, gen.implementationClasses()); // Remove implementation classes
    if (!uid.empty())
        algorithmUids = intersect(algorithmUids, Hyperedges{uid});

    if (!algorithmUids.size())
    {
        std::cout << "No algorithm found.\n";
        return 2;
    }

    // For each of these algorithms
    for (const UniqueId& algorithmUid : algorithmUids)
    {
        const UniqueId implUid("Software::Generator::C++::Implementation::"+gen.access(algorithmUid).label());
        if (gen.exists(implUid) && !overwrite)
        {
            std::cout << "Implementation for " << gen.access(algorithmUid).label() << " with UID " << implUid << " already exists!\n";
            continue;
        }
        if (gen.exists(implUid) && overwrite)
        {
            std::cout << "Overwrite mechanism not implemented yet :(\n";
            continue;
        }
        std::cout << "Generating code for " << gen.access(algorithmUid).label() << "\n";
        gen.generateImplementationClassFor(algorithmUid, implUid);

        // If desired, write implementation to file
        if (generateFiles)
        {
            std::cout << "Writing implementation of " << gen.access(algorithmUid).label() << " to file\n";
            fout.open(gen.access(algorithmUid).label()+".hpp");
            if(fout.good()) {
                fout << gen.access(implUid).label() << std::endl;
            } else {
                std::cout << "FAILED\n";
            }
            fout.close();
        }
    }

    // Store graph
    fout.open(fileNameOut);
    if(fout.good()) {
        fout << YAML::StringFrom(gen) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    return 0;
}
