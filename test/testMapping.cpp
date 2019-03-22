#include "ComponentNetwork.hpp"
#include "ResourceCostModel.hpp"

#include "SoftwareNetwork.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"

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

    //Software::Network sw;
    //std::cout << "Setup Software Model\n";

    //sw.createAlgorithm("Algorithm::A", "Algorithm A");
    //sw.createImplementation("Implementation::A", "Implementation A");
    //sw.isA(Hyperedges{"Implementation::A"}, Hyperedges{"Algorithm::A"});
    //sw.instantiateInterfaceFor(Hyperedges{"Algorithm::A"}, Hyperedges{Software::Network::InterfaceId}, "out");
    //sw.instantiateInterfaceFor(Hyperedges{"Algorithm::A"}, Hyperedges{Software::Network::InterfaceId}, "in");
    //sw.instantiateComponent(Hyperedges{"Implementation::A"}, "a");
    //sw.instantiateComponent(Hyperedges{"Implementation::A"}, "b");
    //sw.instantiateComponent(Hyperedges{"Implementation::A"}, "c");
    //// Create a loop
    //sw.dependsOn(sw.interfacesOf(sw.components("a"), "in"), sw.interfacesOf(sw.components("c"), "out"));
    //sw.dependsOn(sw.interfacesOf(sw.components("b"), "in"), sw.interfacesOf(sw.components("a"), "out"));
    //sw.dependsOn(sw.interfacesOf(sw.components("c"), "in"), sw.interfacesOf(sw.components("b"), "out"));

    //std::cout << "Setup Hardware Model\n";

    //Hardware::Computational::Network hw(sw); // NOTE: To avoid uid conflicts, import model from sw already here

    //hw.createProcessor("Processor::X", "X");
    //hw.instantiateInterfaceFor(Hyperedges{"Processor::X"}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "usb0");
    //hw.instantiateInterfaceFor(Hyperedges{"Processor::X"}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "usb1");
    //hw.instantiateComponent(Hyperedges{"Processor::X"}, "x");
    //hw.instantiateComponent(Hyperedges{"Processor::X"}, "y");
    //hw.instantiateComponent(Hyperedges{"Processor::X"}, "z");
    //// Create chain
    //hw.connectInterface(hw.interfacesOf(hw.processors("x"), "usb1"), hw.interfacesOf(hw.processors("y"), "usb0"));
    //hw.connectInterface(hw.interfacesOf(hw.processors("y"), "usb1"), hw.interfacesOf(hw.processors("z"), "usb0"));

    //std::cout << "Setup Resource Cost Model\n";

    //ResourceCost::Model sw2hw(hw); // NOTE: Now we have everything from SW and HW and RCM

    //// Define resources and costs
    //sw2hw.isConsumer(Hyperedges{"Implementation::A", Software::Network::InterfaceId});
    //sw2hw.isProvider(Hyperedges{"Processor::X", Hardware::Computational::Network::InterfaceId});

    //sw2hw.defineResource("Resource::Memory", "Memory");
    //sw2hw.instantiateResourceFor(sw2hw.concepts("x"), sw2hw.concepts("Memory"), 32.f);
    //sw2hw.instantiateResourceFor(sw2hw.concepts("y"), sw2hw.concepts("Memory"), 64.f);
    //sw2hw.instantiateResourceFor(sw2hw.concepts("z"), sw2hw.concepts("Memory"), 128.f);

    //// TODO: What do interfaces cost? Maybe something like 'bandwidth'?

    //// One statement to rule them all :)
    //sw2hw.costs(Hyperedges{"Implementation::A"}, Hyperedges{"Processor::X"}, sw2hw.instantiateResource(sw2hw.concepts("Memory"), 32.f));
    //// Define mapping relation
    //sw2hw.relate("Software::Component::MappedTo::Hardware::Component", Hyperedges{Software::Network::ImplementationId}, Hyperedges{Hardware::Computational::Network::ProcessorId}, "MAPPED-TO");
    //sw2hw.relate("Software::Interface::MappedTo::Hardware::Interface", Hyperedges{Software::Network::InterfaceId}, Hyperedges{Hardware::Computational::Network::InterfaceId}, "MAPPED-TO");

    //std::ofstream fout;
    //fout.open("unmapped_rcm_spec.yml");
    //if(fout.good()) {
    //    fout << YAML::StringFrom(sw2hw) << std::endl;
    //} else {
    //    std::cout << "FAILED\n";
    //}
    //fout.close();


    //std::cout << "Network before map()\n";
    //for (const UniqueId& conceptUid : sw2hw.concepts())
    //{
    //    std::cout << sw2hw.access(conceptUid) << std::endl;
    //    for (const UniqueId& relUid : sw2hw.relationsTo(Hyperedges{conceptUid}))
    //    {
    //        std::cout << "\t" << sw2hw.access(relUid) << std::endl;
    //    }
    //}

    //// Define mapping functions
    //auto matchSwHw = [] (const Component::Network& rcm, const UniqueId& a, const UniqueId& b) -> bool {
    //    // We trust, that a is a consumer and b is a provider
    //    // First we check if a is an implementation and b is a processor
    //    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    //    Hyperedges hwUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Hardware::Computational::Network::ProcessorId})));
    //    if ((std::find(swUids.begin(), swUids.end(), a) != swUids.end())
    //        && (std::find(hwUids.begin(), hwUids.end(), b) != hwUids.end()))
    //    {
    //        // Lets check if all connected sw components of a are mapped to neighbours of b or not mapped at all.
    //        Hyperedges swNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{a}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    //        Hyperedges hwNeighbourUids(rcm.interfacesOf(rcm.endpointsOf(rcm.interfacesOf(Hyperedges{b}),"", Hypergraph::TraversalDirection::BOTH),"",Hypergraph::TraversalDirection::INVERSE));
    //        hwNeighbourUids = unite(hwNeighbourUids, Hyperedges{b}); // NOTE: We can map also to the same target!
    //        Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourUids),rcm.factsOf("Software::Component::MappedTo::Hardware::Component"))));
    //        // Condition check: hwTargetUids must be a true subset of hwNeighbourUids. That means that the intersection must be equal to hwTargetUids.
    //        if (intersect(hwTargetUids, hwNeighbourUids).size() != hwTargetUids.size())
    //            return false;
    //        return true;
    //    }
    //    // Second, we check if a is an software interface and b is an hardware interface
    //    Hyperedges swInterfaceUids(rcm.interfacesOf(swUids));
    //    Hyperedges hwInterfaceUids(rcm.interfacesOf(hwUids));
    //    if ((std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
    //        && (std::find(hwInterfaceUids.begin(), hwInterfaceUids.end(), b) != hwInterfaceUids.end()))
    //    {
    //        // Lets check if the owner of a is mapped to the owner of b
    //        Hyperedges swOwnerUids(rcm.interfacesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::INVERSE));
    //        Hyperedges hwOwnerUids(rcm.interfacesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::INVERSE));
    //        Hyperedges hwTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swOwnerUids),rcm.factsOf("Software::Component::MappedTo::Hardware::Component"))));
    //        // Condition check: hwTargetUids must be a true subset of hwOwnerUids and not empty. That means that the intersection must be equal to hwTargetUids.
    //        if (hwTargetUids.empty() || (intersect(hwTargetUids, hwOwnerUids).size() != hwTargetUids.size()))
    //        {
    //            return false;
    //        }
    //        // Lets check if all endpoints of a are mapped to endpoints of b or not mapped at all.
    //        Hyperedges swNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{a},"",Hypergraph::TraversalDirection::BOTH));
    //        Hyperedges hwNeighbourInterfaceUids(rcm.endpointsOf(Hyperedges{b},"",Hypergraph::TraversalDirection::BOTH));
    //        //hwNeighbourInterfaceUids = unite(hwNeighbourInterfaceUids, Hyperedges{b}); // NOTE: We can map also to the same target!
    //        Hyperedges hwTargetInterfaceUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourInterfaceUids),rcm.factsOf("Software::Interface::MappedTo::Hardware::Interface"))));
    //        if (intersect(hwTargetInterfaceUids, hwNeighbourInterfaceUids).size() != hwTargetInterfaceUids.size())
    //        {
    //            return false;
    //        }
    //        // Lets check if the owners of the endpoints of a are mapped to the owners of the endpoints of b
    //        Hyperedges swNeighbourOwnerUids(rcm.interfacesOf(swNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    //        Hyperedges hwNeighbourOwnerUids(rcm.interfacesOf(hwNeighbourInterfaceUids,"",Hypergraph::TraversalDirection::INVERSE));
    //        Hyperedges hwNeighbourTargetUids(rcm.isPointingTo(intersect(rcm.relationsFrom(swNeighbourOwnerUids),rcm.factsOf("Software::Component::MappedTo::Hardware::Component"))));
    //        // Condition check: hwNeighbourTargetUids must be a true subset of hwNeighbourOwnerUids and not empty. That means that the intersection must be equal to hwNeighbourTargetUids.
    //        if (hwNeighbourTargetUids.empty() || (intersect(hwNeighbourTargetUids, hwNeighbourOwnerUids).size() != hwNeighbourTargetUids.size()))
    //        {
    //            return false;
    //        }
    //        return true;
    //    }
    //    // In all other cases, we do not have a match
    //    return false;
    //};

    //auto costSwHw = [] (const ResourceCost::Model& rcm, const UniqueId& a, const UniqueId& b) -> float {
    //    float minimum(std::numeric_limits<float>::infinity());
    //    // For finding the resources we can query b ...
    //    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    //    // ... but for the costs we have to query the classes of a and b
    //    Hyperedges consumerClassUids(rcm.instancesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::FORWARD));
    //    Hyperedges providerClassUids(rcm.instancesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::FORWARD));
    //    Hyperedges resourceCostUids(rcm.costsOf(consumerClassUids, providerClassUids));
    //    for (const UniqueId& resourceUid : resourceUids)
    //    {
    //        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
    //        for (const UniqueId& resourceCostUid : resourceCostUids)
    //        {
    //            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
    //            // Only costs of the same class can be handled
    //            if (intersect(resourceClassUids, resourceCostClassUids).empty())
    //                continue;
    //            const std::string rLabel(rcm.access(resourceUid).label());
    //            const float maxR(std::stof(rLabel.substr(0,rLabel.find("|"))));
    //            const std::size_t lastPipePos(rLabel.rfind("|"));
    //            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : maxR);
    //            const float c(std::stof(rcm.access(resourceCostUid).label()));
    //            // Calculate new minimum
    //            minimum = std::min(minimum, (r - c) / maxR);
    //        }
    //    }
    //    return minimum;
    //};

    //auto mapSwHw = [] (CommonConceptGraph& g, const UniqueId& a, const UniqueId& b) -> void {
    //    ResourceCost::Model& rcm = static_cast< ResourceCost::Model& >(g);
    //    // I. Update all resources
    //    Hyperedges resourceUids(rcm.resourcesOf(Hyperedges{b}));
    //    Hyperedges consumerClassUids(rcm.instancesOf(Hyperedges{a},"",Hypergraph::TraversalDirection::FORWARD));
    //    Hyperedges providerClassUids(rcm.instancesOf(Hyperedges{b},"",Hypergraph::TraversalDirection::FORWARD));
    //    Hyperedges resourceCostUids(rcm.costsOf(consumerClassUids, providerClassUids));
    //    for (const UniqueId& resourceUid : resourceUids)
    //    {
    //        Hyperedges resourceClassUids(rcm.instancesOf(Hyperedges{resourceUid}, "", Hypergraph::TraversalDirection::FORWARD));
    //        for (const UniqueId& resourceCostUid : resourceCostUids)
    //        {
    //            Hyperedges resourceCostClassUids(rcm.instancesOf(Hyperedges{resourceCostUid}, "", Hypergraph::TraversalDirection::FORWARD));
    //            // Only costs of the same class can be handled
    //            if (intersect(resourceClassUids, resourceCostClassUids).empty())
    //                continue;
    //            const std::string rLabel(rcm.access(resourceUid).label());
    //            const std::size_t lastPipePos(rLabel.rfind("|"));
    //            const float r(lastPipePos != std::string::npos ? std::stof(rLabel.substr(rLabel.rfind("|")+1)) : std::stof(rLabel));
    //            const float c(std::stof(rcm.access(resourceCostUid).label()));
    //            // Update resources by appending it! (so we always find initial and current resources
    //            rcm.access(resourceUid).updateLabel(std::to_string(r)+"|"+std::to_string(r - c));
    //        }
    //    }
    //    // II. Map a to b
    //    // a) software implementation -> hardware processor
    //    Hyperedges swUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::ImplementationId})));
    //    if (std::find(swUids.begin(), swUids.end(), a) != swUids.end())
    //    {
    //        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, "Software::Component::MappedTo::Hardware::Component");
    //        return;
    //    }
    //    // b) software interface -> hardware interface
    //    Hyperedges swInterfaceUids(rcm.instancesOf(rcm.subclassesOf(Hyperedges{Software::Network::InterfaceId})));
    //    if (std::find(swInterfaceUids.begin(), swInterfaceUids.end(), a) != swInterfaceUids.end())
    //    {
    //        rcm.factFrom(Hyperedges{a}, Hyperedges{b}, "Software::Interface::MappedTo::Hardware::Interface");
    //        return;
    //    }
    //};

    //fout.open("rcm_spec.yml");
    //if(fout.good()) {
    //    fout << YAML::StringFrom(sw2hw) << std::endl;
    //} else {
    //    std::cout << "FAILED\n";
    //}
    //fout.close();

    //// Finally, call map!
    //ResourceCost::Model sw2hw2(sw2hw.map(ResourceCost::Model::partitionFuncLeft, ResourceCost::Model::partitionFuncRight, matchSwHw, costSwHw, mapSwHw));

    //std::cout << "Network after map()\n";
    //for (const UniqueId& conceptUid : sw2hw2.concepts())
    //{
    //    std::cout << sw2hw2.access(conceptUid) << std::endl;
    //    for (const UniqueId& relUid : sw2hw2.relationsTo(Hyperedges{conceptUid}))
    //    {
    //        std::cout << "\t" << sw2hw2.access(relUid) << std::endl;
    //    }
    //}

    return 0;
}
