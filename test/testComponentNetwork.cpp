#include "ComponentNetwork.hpp"
#include "HyperedgeYAML.hpp"

#include <fstream>
#include <iostream>
#include <cassert>

int main(void)
{
    std::cout << "*** COMPONENT NETWORK TEST ***\n";
    Component::Network cnd;

    std::cout << "> Create two component classes\n";
    std::cout << cnd.createComponent("MyFirstComponent", "A") << "\n";
    std::cout << cnd.createComponent("MySecondComponent", "B") << "\n";

    std::cout << "> Create common interface class\n";
    std::cout << cnd.createInterface("CommonInterface", "x") << "\n";

    std::cout << "> Each component shall get two common interfaces\n";
    std::cout << cnd.hasInterface(Hyperedges{"MyFirstComponent"},  cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "x")) << "\n";
    std::cout << cnd.hasInterface(Hyperedges{"MyFirstComponent"},  cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "y")) << "\n";
    std::cout << cnd.hasInterface(Hyperedges{"MySecondComponent"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "u")) << "\n";
    std::cout << cnd.hasInterface(Hyperedges{"MySecondComponent"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "v")) << "\n";

    std::cout << "> Store component network using YAML" << std::endl;
    YAML::Node test;
    test = static_cast<Hypergraph*>(&cnd);
    std::ofstream fout;
    fout.open("cnd.yml");
    if(fout.good()) {
        fout << test;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "*** TEST DONE ***\n";
}
