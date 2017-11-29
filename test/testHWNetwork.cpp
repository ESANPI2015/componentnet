#include "HardwareComputationalNetwork.hpp"
#include "HyperedgeYAML.hpp"

#include <fstream>
#include <iostream>
#include <cassert>

int main(void)
{
    std::cout << "*** HW NETWORK TEST ***\n";

    Hardware::Computational::Network hwnet;

    std::cout << "> Create device\n";
    std::cout << hwnet.createDevice("First device", "A") << "\n";

    std::cout << "> Create interface\n";
    std::cout << hwnet.createInterface("First interface", "x") << "\n";

    std::cout << "> Assign interface to device\n";
    std::cout << hwnet.hasInterface(Hyperedges{"First device"}, hwnet.instantiateFrom(Hyperedges{"First interface"}));

    std::cout << "> All concepts" << std::endl;
    auto sets = hwnet.find();
    for (auto setId : sets)
    {
        std::cout << setId << " " << hwnet.get(setId)->label() << std::endl;
    }

    std::cout << "> Create 2nd device\n";
    std::cout << hwnet.createDevice("Second device", "B") << "\n";

    std::cout << "> Create 2nd interface\n";
    std::cout << hwnet.createInterface("Second interface", "y") << "\n";

    std::cout << "> Assign interface to device\n";
    std::cout << hwnet.hasInterface(Hyperedges{"Second device"}, hwnet.instantiateFrom(Hyperedges{"Second interface"})) << "\n";

    std::cout << "> Connect the two interfaces by a bus\n";
    std::cout << hwnet.hasInterface(hwnet.createBus("MyBus","x2y"), hwnet.instantiateFrom(Hyperedges{"First interface", "Second interface"})) << "\n";
    std::cout << hwnet.connectInterface(hwnet.interfacesOf(Hyperedges{"First device"},"x"), hwnet.interfacesOf(Hyperedges{"MyBus"},"x")) << "\n";
    std::cout << hwnet.connectInterface(hwnet.interfacesOf(Hyperedges{"Second device"},"y"), hwnet.interfacesOf(Hyperedges{"MyBus"},"y")) << "\n";

    std::cout << "> Query deviceClasses\n";
    auto devsId = hwnet.deviceClasses();
    for (auto setId : devsId)
    {
        std::cout << setId << " " << hwnet.get(setId)->label() << std::endl;
    }

    std::cout << "> Query interfaceClasses\n";
    auto ifsId = hwnet.interfaceClasses();
    for (auto setId : ifsId)
    {
        std::cout << setId << " " << hwnet.get(setId)->label() << std::endl;
    }

    std::cout << "> Query busClasses\n";
    auto bussesId = hwnet.busClasses();
    for (auto setId : bussesId)
    {
        std::cout << setId << " " << hwnet.get(setId)->label() << std::endl;
    }

    std::cout << "> Store hwnet using YAML" << std::endl;

    YAML::Node test;
    test = static_cast<Hypergraph*>(&hwnet);

    std::ofstream fout;
    fout.open("hwnet.yml");
    if(fout.good()) {
        fout << test;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "> Cleanup the hwnet\n";
    hwnet = Hardware::Computational::Network();

    std::cout << "> Create real world example\n";
    // Create classes
    // NOTE: The following reads as "Every mdaq2 is a DEVICE"
    //Hyperedges mdaqSC = hwnet.createDevice("mdaq2");
    //Hyperedges pcSC = hwnet.createDevice("PC");
    //Hyperedges spineSC = hwnet.createDevice("spine_board");
    //Hyperedges convSC = hwnet.createDevice("LVDS2USB");
    //Hyperedges lvdsSC = hwnet.createInterface("LVDS");
    //Hyperedges usbSC = hwnet.createInterface("USB");
    //Hyperedges ndlcomSC = hwnet.createBus("NDLCom");
    //Hyperedges usbBusSC = hwnet.createBus("USB");
    // Define bus domains
    //hwnet.connects(usbBusSC, usbSC);
    //hwnet.connects(ndlcomSC, lvdsSC);

    // Create interfaces
    // NOTE: The following reads as "Every mdaq2 HAS a LVDS1 and a LVDS2 interface of class LVDS"
    //Hyperedges id1 = hwnet.instantiateFrom(lvdsSC, "LVDS1");
    //Hyperedges id2 = hwnet.instantiateFrom(lvdsSC, "LVDS2");
    //hwnet.has(mdaqSC, unite(id1,id2));
    //id1 = hwnet.instantiateFrom(lvdsSC, "LVDS1");
    //id2 = hwnet.instantiateFrom(lvdsSC, "LVDS2");
    //hwnet.has(spineSC, unite(id1,id2));
    //id1 = hwnet.instantiateFrom(lvdsSC, "LVDS1");
    //id2 = hwnet.instantiateFrom(usbSC, "USB1");
    //hwnet.has(convSC, unite(id1,id2));
    //id2 = hwnet.instantiateFrom(usbSC, "/dev/ttyUSB0");
    //hwnet.has(pcSC, id2);

    // The hardware graph now contains the models of the devices we want to instantiate and connect in the following
    // We start with instantiating one device, the interfaces we want to connect
    // NOTE: The following reads as "There is a TEST BOARD of type mdaq2"
    //Hyperedges mdaqId = hwnet.instantiateDevice(mdaqSC, "TEST BOARD");
    //Hyperedges spineId = hwnet.instantiateDevice(spineSC);
    //Hyperedges convId = hwnet.instantiateDevice(convSC);
    //Hyperedges laptopId = hwnet.instantiateDevice(pcSC, "My Laptop");

    // We should now have the devices and the needed interfaces. It is time to connect them
    // NOTE: The following reads as "There is a bus of type NDLCom which connects a LVDS1 of TEST BOARD and LVDS1 of spine_board"
    //hwnet.instantiateBus(ndlcomSC, unite(hwnet.interfaces(mdaqId, "LVDS1"),hwnet.interfaces(spineId,"LVDS1")));
    //hwnet.instantiateBus(ndlcomSC, unite(hwnet.interfaces(convId, "LVDS1"),hwnet.interfaces(spineId,"LVDS2")));
    //hwnet.instantiateBus(usbBusSC, unite(hwnet.interfaces(convId, "USB1"),hwnet.interfaces(laptopId,"/dev/ttyUSB0")));

    // We could now make the specific instances and the network they form PART-OF some X. This X would then represent all occurences of this setting/network.

    //std::cout << "> Store hwnet using YAML" << std::endl;

    //test = static_cast<Hypergraph*>(&hwnet);
    //fout.open("demo.yml");
    //if(fout.good()) {
    //    fout << test;
    //} else {
    //    std::cout << "FAILED\n";
    //}
    //fout.close();


    std::cout << "*** TEST DONE ***\n";
}
