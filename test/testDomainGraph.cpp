#include "DomainSpecificGraph.hpp"
#include "HyperedgeYAML.hpp"

#include <fstream>
#include <iostream>
#include <cassert>

int main(void)
{
    std::cout << "*** DOMAIN SPECIFIC GRAPH TEST ***\n";
    Domain::Graph dgraph;

    std::cout << "> Create two subdomains of a common domain\n";
    dgraph.createDomain("EXECUTION","EXECUTION");
    dgraph.createDomain("SOFTWARE", "SOFTWARE");
    dgraph.createDomain("HARDWARE", "HARDWARE");
    std::cout << dgraph.partOfDomain(Hyperedges{"SOFTWARE", "HARDWARE"}, Hyperedges{"EXECUTION"}) << std::endl;

    std::cout << "> Add parts to the domains\n";
    std::cout << dgraph.partOfDomain(dgraph.create("Algorithm"), dgraph.findDomainBy("SOFTWARE")) << std::endl;
    std::cout << dgraph.partOfDomain(dgraph.create("Processor"), dgraph.findDomainBy("HARDWARE")) << std::endl;

    std::cout << "> Are the parts also in upper domain?\n";
    std::cout << dgraph.partsOfDomain(dgraph.findDomainBy("EXECUTION")) << "\n";

    std::cout << "> Store domain specific graph using YAML" << std::endl;
    YAML::Node test;
    test = static_cast<Hypergraph*>(&dgraph);
    std::ofstream fout;
    fout.open("dgraph.yml");
    if(fout.good()) {
        fout << test;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "*** TEST DONE ***\n";
}
