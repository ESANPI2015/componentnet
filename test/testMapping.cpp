#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "ComponentNetwork.hpp"
#include "ResourceCostModel.hpp"

#include "SoftwareNetwork.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"

#include "Mapper.hpp"

#include <fstream>
#include <iostream>
#include <limits>

TEST_CASE("Perform simple mapping of component networks using resource cost model", "[SimpleMapping]")
{
    Component::Network cn;
    // Define meta model
    cn.createComponent("Component::Class::A", "Component A");
    cn.createComponent("Component::Class::B", "Component B");

    // Create instances
    cn.instantiateComponent(cn.concepts("Component A"), "a");
    cn.instantiateComponent(cn.concepts("Component A"), "b");
    cn.instantiateComponent(cn.concepts("Component A"), "c");
    cn.instantiateComponent(cn.concepts("Component B"), "1");
    cn.instantiateComponent(cn.concepts("Component B"), "2");
    cn.instantiateComponent(cn.concepts("Component B"), "3");

    // Define resource cost model
    ResourceCost::Model rm(cn);
    REQUIRE(rm.defineResource("Resource::Class::A", "Apples") == Hyperedges{"Resource::Class::A"});
    rm.isConsumer(rm.concepts("Component A"));
    rm.isProvider(rm.concepts("Component B"));
    REQUIRE(rm.consumers().size() == 3);
    REQUIRE(rm.providers().size() == 3);
    rm.provides(rm.concepts("1"), rm.instantiateResource(rm.concepts("Apples"), 3.f));
    rm.provides(rm.concepts("2"), rm.instantiateResource(rm.concepts("Apples"), 4.f));
    rm.provides(rm.concepts("3"), rm.instantiateResource(rm.concepts("Apples"), 1.f));
    REQUIRE(rm.resourcesOf(rm.providers()).size() == 3);
    rm.consumes(rm.concepts("a"), rm.instantiateResource(rm.concepts("Apples"), 1.f));
    rm.consumes(rm.concepts("b"), rm.instantiateResource(rm.concepts("Apples"), 2.f));
    rm.consumes(rm.concepts("c"), rm.instantiateResource(rm.concepts("Apples"), 3.f));
    ResourceCost::Model rm2(rm.map(ResourceCost::Model::partitionFuncLeft, ResourceCost::Model::partitionFuncRight, ResourceCost::Model::matchFunc, ResourceCost::Model::mapFunc));
    // Check mapping result
    // Does every consumer have a provider?
    for (const UniqueId& consumerUid : rm2.consumers())
    {
        REQUIRE(rm2.providersOf(Hyperedges{consumerUid}).size() == 1);
    }
}

TEST_CASE("Perform a real world exemplary mapping of software to hardware components", "[SWHWMapping]")
{
    // Setup software model
    Software::Network sw;
    // Create software implementations
    sw.createImplementation("Implementation::A", "Implementation A");
    sw.createImplementationInterface("Implementation::Interface::X", "Interface X");
    sw.needsInterface(Hyperedges{"Implementation::A"}, sw.instantiateFrom(Hyperedges{"Implementation::Interface::X"}, "in"));
    sw.providesInterface(Hyperedges{"Implementation::A"}, sw.instantiateFrom(Hyperedges{"Implementation::Interface::X"}, "out"));
    // ... and instantiate them
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "a");
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "b");
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "c");
    // Create a loop
    sw.dependsOn(sw.interfacesOf(sw.components("a"), "in"), sw.interfacesOf(sw.components("c"), "out"));
    sw.dependsOn(sw.interfacesOf(sw.components("b"), "in"), sw.interfacesOf(sw.components("a"), "out"));
    sw.dependsOn(sw.interfacesOf(sw.components("c"), "in"), sw.interfacesOf(sw.components("b"), "out"));
    // Check connections
    // a -> b
    REQUIRE(sw.endpointsOf(sw.interfacesOf(sw.components("a"))).size() == 1);
    // b -> c
    REQUIRE(sw.endpointsOf(sw.interfacesOf(sw.components("b"))).size() == 1);
    // c -> a
    REQUIRE(sw.endpointsOf(sw.interfacesOf(sw.components("c"))).size() == 1);

    // Setup hardware model
    Hardware::Computational::Network hw(sw); // NOTE: To avoid uid conflicts, import model from sw already here
    hw.createProcessor("Processor::X", "X");
    hw.instantiateInterfaceFor(Hyperedges{"Processor::X"}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "usb0");
    hw.instantiateInterfaceFor(Hyperedges{"Processor::X"}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "usb1");
    hw.instantiateComponent(Hyperedges{"Processor::X"}, "x");
    hw.instantiateComponent(Hyperedges{"Processor::X"}, "y");
    hw.instantiateComponent(Hyperedges{"Processor::X"}, "z");
    // Create chain
    hw.connectInterface(hw.interfacesOf(hw.processors("x"), "usb1"), hw.interfacesOf(hw.processors("y"), "usb0"));
    hw.connectInterface(hw.interfacesOf(hw.processors("y"), "usb1"), hw.interfacesOf(hw.processors("z"), "usb0"));
    // Check connections
    // y -> z
    REQUIRE(hw.endpointsOf(hw.interfacesOf(hw.processors("y"))).size() == 1);
    // x -> y
    REQUIRE(hw.endpointsOf(hw.interfacesOf(hw.processors("x"))).size() == 1);

    // Setup resource cost model
    ResourceCost::Model sw2hw(hw); // NOTE: Now we have everything from SW and HW and RCM
    // Define resources and costs
    sw2hw.isConsumer(Hyperedges{"Implementation::A", "Implementation::Interface::X"});
    sw2hw.isProvider(Hyperedges{"Processor::X", Hardware::Computational::Network::InterfaceId});
    REQUIRE(sw2hw.consumers().size() == 11);
    REQUIRE(sw2hw.providers().size() == 11);

    sw2hw.defineResource("Resource::Memory", "Memory");
    sw2hw.provides(sw2hw.concepts("x"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 64.f));
    REQUIRE(sw2hw.resourcesOf(sw2hw.concepts("x")).size() == 1);
    sw2hw.provides(sw2hw.concepts("y"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    sw2hw.provides(sw2hw.concepts("z"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 64.f));
    sw2hw.consumes(sw2hw.concepts("a"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    REQUIRE(sw2hw.demandsOf(sw2hw.concepts("a")).size() == 1);
    sw2hw.consumes(sw2hw.concepts("b"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    sw2hw.consumes(sw2hw.concepts("c"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    // Store for potential later use
    std::ofstream fout;
    fout.open("unmapped_rcm_spec.yml");
    REQUIRE(fout.good() == true);
    fout << YAML::StringFrom(sw2hw) << std::endl;
    fout.close();

    // Start the mapping
    Software::Hardware::Mapper mapper(sw2hw);
    const float globalCosts(mapper.map());
    REQUIRE(globalCosts >= 0.f);
    // Check mapping
    // Every implementation needs a processor to execute
    for (const UniqueId& consumerUid : sw.implementations())
    {
        REQUIRE(mapper.providersOf(Hyperedges{consumerUid}).size() == 1);
    }

    // Store for potential later user
    fout.open("rcm_spec.yml");
    REQUIRE(fout.good() == true);
    fout << YAML::StringFrom(mapper) << std::endl;
    fout.close();
}

