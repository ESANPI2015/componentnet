#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "ComponentNetwork.hpp"
#include "HypergraphYAML.hpp"

#include <iostream>

TEST_CASE("Setup and operate on a component network", "[Component::Network]")
{
    Component::Network cnd;
    // Create component and interface classes
    REQUIRE(cnd.createComponent("MyFirstComponent", "A") == Hyperedges{"MyFirstComponent"});
    REQUIRE(cnd.createComponent("MySecondComponent", "B") == Hyperedges{"MySecondComponent"});
    REQUIRE(intersect(cnd.componentClasses(), Hyperedges{"MyFirstComponent", "MySecondComponent"}) == Hyperedges{"MyFirstComponent", "MySecondComponent"});
    REQUIRE(cnd.createInterface("CommonInterface", "Interface") == Hyperedges{"CommonInterface"});
    // Assign interface instances to component classes
    cnd.hasInterface(Hyperedges{"MyFirstComponent"},  cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "x"));
    cnd.hasInterface(Hyperedges{"MyFirstComponent"},  cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "y"));
    cnd.hasInterface(Hyperedges{"MySecondComponent"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "u"));
    cnd.hasInterface(Hyperedges{"MySecondComponent"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "v"));
    REQUIRE(cnd.interfacesOf(Hyperedges{"MyFirstComponent"}).size() == 2);
    REQUIRE(cnd.interfacesOf(Hyperedges{"MySecondComponent"}).size() == 2);
    // Create component and interface instances
    cnd.instantiateComponent(Hyperedges{"MyFirstComponent"}, "a");
    cnd.instantiateComponent(Hyperedges{"MySecondComponent"}, "b");
    REQUIRE(cnd.instancesOf(Hyperedges{"MyFirstComponent"}).size() == 1);
    REQUIRE(cnd.instancesOf(Hyperedges{"MySecondComponent"}).size() == 1);
    // Check inheritance of interfaces
    REQUIRE(cnd.interfacesOf(Hyperedges{"MyFirstComponent"}).size() == cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MyFirstComponent"})).size());
    REQUIRE(cnd.interfacesOf(Hyperedges{"MySecondComponent"}).size() == cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MySecondComponent"})).size());
    // Check connectivity
    cnd.connectInterface(cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MyFirstComponent"}),"y"), cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MySecondComponent"}),"u"));
    REQUIRE(cnd.endpointsOf(cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MyFirstComponent"}),"y")) == cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MySecondComponent"}),"u"));
    // Create a composite component class
    cnd.partOfComponent(cnd.components(), cnd.createComponent("MyFirstNetwork","C"));
    REQUIRE(cnd.componentsOf(Hyperedges{"MyFirstNetwork"}).size() == 2);
    // Export internal interfaces
    cnd.hasInterface(Hyperedges{"MyFirstNetwork"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "a"));
    cnd.hasInterface(Hyperedges{"MyFirstNetwork"}, cnd.instantiateFrom(Hyperedges{"CommonInterface"}, "b"));
    cnd.aliasOf(cnd.interfacesOf(Hyperedges{"MyFirstNetwork"}, "a"), cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MyFirstComponent"}), "x"));
    cnd.aliasOf(cnd.interfacesOf(Hyperedges{"MyFirstNetwork"}, "b"), cnd.interfacesOf(cnd.instancesOf(Hyperedges{"MySecondComponent"}), "v"));
    REQUIRE(cnd.originalInterfacesOf(cnd.interfacesOf(Hyperedges{"MyFirstNetwork"})).size() == 2);
    // TODO: Values & subinterfaces
}
