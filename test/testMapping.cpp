#include "ComponentNetwork.hpp"
#include "ResourceCostModel.hpp"

#include "SoftwareNetwork.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"

#include "Mapper.hpp"

#include <fstream>
#include <iostream>
#include <limits>

int main (void)
{
    std::cout << "Test mapping of Component Networks using Resource Cost Model\n";

    Component::Network cn;

    std::cout << "Create components\n";
    
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

    std::cout << "Merging component network into resource model\n";
    ResourceCost::Model rm(cn);

    std::cout << "Create resources\n";

    rm.defineResource("Resource::Class::A", "Apples");

    std::cout << "Make components A consumers and components B providers\n";
    
    rm.isConsumer(rm.concepts("Component A"));
    rm.isProvider(rm.concepts("Component B"));

    std::cout << "Assign resources to providers\n";
    
    rm.provides(rm.concepts("1"), rm.instantiateResource(rm.concepts("Apples"), 3.f));
    rm.provides(rm.concepts("2"), rm.instantiateResource(rm.concepts("Apples"), 4.f));
    rm.provides(rm.concepts("3"), rm.instantiateResource(rm.concepts("Apples"), 1.f));

    std::cout << "Assign demands to consumers\n";
    
    rm.consumes(rm.concepts("a"), rm.instantiateResource(rm.concepts("Apples"), 1.f));
    rm.consumes(rm.concepts("b"), rm.instantiateResource(rm.concepts("Apples"), 2.f));
    rm.consumes(rm.concepts("c"), rm.instantiateResource(rm.concepts("Apples"), 3.f));

    std::cout << "Query consumers\n";
    std::cout << rm.consumers() << std::endl;
    std::cout << "Query providers\n";
    std::cout << rm.providers() << std::endl;
    std::cout << "Query resources of providers\n";
    for (const UniqueId& providerUid : rm.providers())
    {
        std::cout << providerUid << ": ";
        for (const UniqueId& resourceUid : rm.resourcesOf(Hyperedges{providerUid}))
        {
            std::cout << rm.access(resourceUid).label() << " ";
        }
        std::cout << "\n";
    }

    std::cout << "Map components of type A to components of type B using the resource cost model\n";

    std::cout << "Network before map()\n";
    for (const UniqueId& conceptUid : rm.concepts())
    {
        std::cout << rm.access(conceptUid) << std::endl;
        for (const UniqueId& relUid : rm.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << rm.access(relUid) << std::endl;
        }
    }

    ResourceCost::Model rm2(rm.map(ResourceCost::Model::partitionFuncLeft, ResourceCost::Model::partitionFuncRight, ResourceCost::Model::matchFunc, ResourceCost::Model::mapFunc));

    std::cout << "Network after map()\n";
    for (const UniqueId& conceptUid : rm2.concepts())
    {
        std::cout << rm2.access(conceptUid) << std::endl;
        for (const UniqueId& relUid : rm2.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << rm2.access(relUid) << std::endl;
        }
    }

    //std::cout << "\n\nSetting up a software and a hardware graph and perform a nested mapping\n";

    Software::Network sw;
    std::cout << "Setup Software Model\n";

    sw.createImplementation("Implementation::A", "Implementation A");
    sw.createImplementationInterface("Implementation::Interface::X", "Interface X");
    //sw.instantiateInterfaceFor(Hyperedges{"Algorithm::A"}, Hyperedges{Software::Network::InterfaceId}, "out");
    //sw.instantiateInterfaceFor(Hyperedges{"Algorithm::A"}, Hyperedges{Software::Network::InterfaceId}, "in");
    sw.needsInterface(Hyperedges{"Implementation::A"}, sw.instantiateFrom(Hyperedges{"Implementation::Interface::X"}, "in"));
    sw.providesInterface(Hyperedges{"Implementation::A"}, sw.instantiateFrom(Hyperedges{"Implementation::Interface::X"}, "out"));
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "a");
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "b");
    sw.instantiateComponent(Hyperedges{"Implementation::A"}, "c");
    // Create a loop
    sw.dependsOn(sw.interfacesOf(sw.components("a"), "in"), sw.interfacesOf(sw.components("c"), "out"));
    sw.dependsOn(sw.interfacesOf(sw.components("b"), "in"), sw.interfacesOf(sw.components("a"), "out"));
    sw.dependsOn(sw.interfacesOf(sw.components("c"), "in"), sw.interfacesOf(sw.components("b"), "out"));

    std::cout << "Setup Hardware Model\n";

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

    std::cout << "Setup Resource Cost Model\n";

    ResourceCost::Model sw2hw(hw); // NOTE: Now we have everything from SW and HW and RCM

    // Define resources and costs
    sw2hw.isConsumer(Hyperedges{"Implementation::A", Software::Network::InterfaceId});
    sw2hw.isProvider(Hyperedges{"Processor::X", Hardware::Computational::Network::InterfaceId});

    sw2hw.defineResource("Resource::Memory", "Memory");
    sw2hw.provides(sw2hw.concepts("x"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 64.f));
    sw2hw.provides(sw2hw.concepts("y"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    sw2hw.provides(sw2hw.concepts("z"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 64.f));

    // TODO: What do interfaces cost? Maybe something like 'bandwidth'?

    sw2hw.consumes(sw2hw.concepts("a"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    sw2hw.consumes(sw2hw.concepts("b"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    sw2hw.consumes(sw2hw.concepts("c"), sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));

    std::ofstream fout;
    fout.open("unmapped_rcm_spec.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(sw2hw) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "Network before map()\n";
    for (const UniqueId& conceptUid : sw2hw.concepts())
    {
        std::cout << sw2hw.access(conceptUid) << std::endl;
        for (const UniqueId& relUid : sw2hw.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << sw2hw.access(relUid) << std::endl;
        }
    }

    Software::Hardware::Mapper mapper(sw2hw);

    const float globalCosts(mapper.map());

    std::cout << "\nmap() returned global costs: " << globalCosts << "\n";

    std::cout << "\nNetwork after map()\n";
    for (const UniqueId& conceptUid : mapper.concepts())
    {
        std::cout << mapper.access(conceptUid) << std::endl;
        for (const UniqueId& relUid : mapper.relationsTo(Hyperedges{conceptUid}))
        {
            std::cout << "\t" << mapper.access(relUid) << std::endl;
        }
    }
    
    std::cout << "\nMapping result details\n";
    for (const UniqueId& a : sw.implementations())
    {
        std::cout << mapper.access(a).label() << std::endl;
        Hyperedges otherUids(mapper.providersOf(Hyperedges{a}));
        for (const UniqueId& b : otherUids)
        {
            std::cout << " -> " << mapper.access(b).label() << "\n";
        }
        Hyperedges ifUids(sw.interfacesOf(Hyperedges{a}));
        for (const UniqueId c : ifUids)
        {
            std::cout << " Interface " << mapper.access(c).label() << std::endl;
            Hyperedges otherIfUids(mapper.providersOf(Hyperedges{c}));
            for (const UniqueId& b : otherIfUids)
            {
                std::cout << "  -> " << mapper.access(b).label() << "\n";
            }
        }
        std::cout << "\n";
    }

    fout.open("rcm_spec.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(mapper) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();


    return 0;
}
