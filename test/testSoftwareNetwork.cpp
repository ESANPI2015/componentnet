#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "SoftwareNetwork.hpp"

TEST_CASE("Setup and operate on a software network", "[Software::Network]")
{
    Software::Network swn;
    // Create an algorithm class
    REQUIRE(swn.createAlgorithm("Algorithm A", "A") == Hyperedges{"Algorithm A"});
    // Create an interface class
    REQUIRE(swn.createInterface("Interface X", "X") == Hyperedges{"Interface X"});
    // Instantiate an interface & make it an input
    REQUIRE(swn.needsInterface(Hyperedges{"Algorithm A"}, swn.instantiateInterfaceFor(Hyperedges{"Algorithm A"}, Hyperedges{"Interface X"}, "in")).size() > 0);
    REQUIRE(swn.inputsOf(Hyperedges{"Algorithm A"}).size() == 1);
    // Make an output
    REQUIRE(swn.providesInterface(Hyperedges{"Algorithm A"}, swn.instantiateInterfaceFor(Hyperedges{"Algorithm A"}, Hyperedges{"Interface X"}, "out")).size() > 0);
    REQUIRE(swn.inputsOf(Hyperedges{"Algorithm A"}).size() == 1);
    REQUIRE(swn.outputsOf(Hyperedges{"Algorithm A"}).size() == 1);
    REQUIRE(swn.interfacesOf(Hyperedges{"Algorithm A"}).size() == 2);
    // Create two instances of Algorithm A
    REQUIRE(swn.instantiateComponent(Hyperedges{"Algorithm A"}, "1").size() == 1);
    REQUIRE(swn.inputsOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "1")).size() == 1);
    REQUIRE(swn.outputsOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "1")).size() == 1);
    REQUIRE(swn.interfacesOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "1")).size() == 2);
    REQUIRE(swn.instantiateComponent(Hyperedges{"Algorithm A"}, "2").size() == 1);
    // Connect both algorithm instances
    REQUIRE(swn.dependsOn(swn.inputsOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "2")), swn.outputsOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "1"))).size() > 0);
    REQUIRE(swn.endpointsOf(swn.outputsOf(swn.instancesOf(Hyperedges{"Algorithm A"}, "1"))).size() == 1);
    // Additional checks
    REQUIRE(swn.algorithms().size() == 2);
    REQUIRE(swn.interfaces().size() == 6);
}
