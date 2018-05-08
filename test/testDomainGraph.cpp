#include "DomainSpecificGraph.hpp"
#include "HypergraphYAML.hpp"

#include <fstream>
#include <iostream>
#include <cassert>

int main(void)
{
    std::cout << "*** DOMAIN SPECIFIC GRAPH TEST ***\n";
    Domain::Graph dgraph("MyDomainId", "MyDomain");

    std::cout << "> Create concepts in domain\n";
    std::cout << dgraph.create("FirstConcept") << "\n";
    std::cout << dgraph.create("SecondConcept") << "\n";

    std::cout << "> Show parts of domain\n";
    std::cout << dgraph.partsOfDomain() << "\n";

    std::cout << "> Make a second domain graph\n";
    Domain::Graph dgraph2("MyOtherDomainId", "MyOtherDomain");

    std::cout << "> Create concepts in domain\n";
    std::cout << dgraph2.create("OtherFirstConcept") << "\n";
    std::cout << dgraph2.create("OtherSecondConcept") << "\n";

    std::cout << "> Merge the graphs\n";
    Hypergraph merged(dgraph, dgraph2);
    Conceptgraph cg(merged);
    CommonConceptGraph ccg(cg);
    Domain::Graph dg(ccg, "MergedDomainId", "MergedDomain");

    std::cout << "> Add the subdomains to the upper domain\n";
    std::cout << dg.addToDomain(Hyperedges{"MyDomainId", "MyOtherDomainId"}) << "\n";

    std::cout << "> Show parts of domain\n";
    std::cout << dg.partsOfDomain() << "\n";

    std::cout << "> Store domain specific graph using YAML" << std::endl;
    std::ofstream fout;
    fout.open("dgraph.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(dg) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "*** TEST DONE ***\n";
}
