#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"

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
    auto sets = hwnet.concepts();
    for (auto setId : sets)
    {
        std::cout << setId << " " << hwnet.access(setId).label() << std::endl;
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
        std::cout << setId << " " << hwnet.access(setId).label() << std::endl;
    }

    std::cout << "> Query interfaceClasses\n";
    auto ifsId = hwnet.interfaceClasses();
    for (auto setId : ifsId)
    {
        std::cout << setId << " " << hwnet.access(setId).label() << std::endl;
    }

    std::cout << "> Query busClasses\n";
    auto bussesId = hwnet.busClasses();
    for (auto setId : bussesId)
    {
        std::cout << setId << " " << hwnet.access(setId).label() << std::endl;
    }

    std::cout << "> Store hwnet using YAML" << std::endl;

    std::ofstream fout;
    fout.open("hwnet.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(hwnet) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();

    std::cout << "> Cleanup the hwnet\n";
    hwnet = Hardware::Computational::Network();

    std::cout << "> Create real world example\n";
    // Create classes
    Hyperedges mdaqSC = hwnet.createDevice("DFKI::iStruct::mdaq2","mdaq2");
    Hyperedges pcSC = hwnet.createDevice("PC","PC");
    Hyperedges spineSC = hwnet.createDevice("DFKI::iStruct::spine_board","spine_board");
    Hyperedges convSC = hwnet.createDevice("DFKI::LVDS2USB", "lvds2usb");
    Hyperedges lvdsSC = hwnet.createInterface("LVDS","lvds");
    Hyperedges usbSC = hwnet.createInterface("USB","usb");
    Hyperedges ndlcomSC = hwnet.createBus("DFKI::iStruct::NDLCom", "NDLCom");
    Hyperedges usbBusSC = hwnet.createBus("USB::P2P", "usb");

    // Create interfaces
    // NOTE: The following reads as "Every mdaq2 HAS a LVDS1 and a LVDS2 interface of class LVDS"
    Hyperedges id1 = hwnet.instantiateFrom(lvdsSC, "lvds1");
    Hyperedges id2 = hwnet.instantiateFrom(lvdsSC, "lvds2");
    hwnet.hasInterface(mdaqSC, unite(id1,id2));
    id1 = hwnet.instantiateFrom(lvdsSC, "lvds1");
    id2 = hwnet.instantiateFrom(lvdsSC, "lvds2");
    hwnet.hasInterface(spineSC, unite(id1,id2));
    id1 = hwnet.instantiateFrom(lvdsSC, "lvds1");
    id2 = hwnet.instantiateFrom(usbSC, "usb1");
    hwnet.hasInterface(convSC, unite(id1,id2));
    id2 = hwnet.instantiateFrom(usbSC, "/dev/ttyUSB0");
    hwnet.hasInterface(pcSC, id2);

    // Define bus domains
    //hwnet.connects(usbBusSC, usbSC);
    //hwnet.connects(ndlcomSC, lvdsSC);
    // TODO: The busses have one interface to connect all others. Is this the way to go? Or should we make a BUS a component AND an interface?
    hwnet.hasInterface(ndlcomSC, hwnet.instantiateFrom(lvdsSC, "lvds"));
    hwnet.hasInterface(usbBusSC, unite(hwnet.instantiateFrom(usbSC, "usb0"), hwnet.instantiateFrom(usbSC, "usb1")));

    // The hardware graph now contains the models of the devices we want to instantiate and connect in the following
    // We start with instantiating one device, the interfaces we want to connect
    // NOTE: The following reads as "There is a TEST BOARD of type mdaq2"
    Hyperedges mdaqId = hwnet.instantiateComponent(mdaqSC, "TEST BOARD");
    Hyperedges spineId = hwnet.instantiateComponent(spineSC);
    Hyperedges convId = hwnet.instantiateComponent(convSC);
    Hyperedges laptopId = hwnet.instantiateComponent(pcSC, "My Laptop");

    // We should now have the devices and the needed interfaces. It is time to connect them
    // NOTE: The following reads as "There is a bus of type NDLCom which connects a LVDS1 of TEST BOARD and LVDS1 of spine_board"
    //hwnet.instantiateBus(ndlcomSC, unite(hwnet.interfaces(mdaqId, "LVDS1"),hwnet.interfaces(spineId,"LVDS1")));
    //hwnet.instantiateBus(ndlcomSC, unite(hwnet.interfaces(convId, "LVDS1"),hwnet.interfaces(spineId,"LVDS2")));
    //hwnet.instantiateBus(usbBusSC, unite(hwnet.interfaces(convId, "USB1"),hwnet.interfaces(laptopId,"/dev/ttyUSB0")));
    // TODO: We need a convenience function to make connections over busses more easy
    Hyperedges ndlcomBus = hwnet.instantiateComponent(ndlcomSC);
    Hyperedges usbBus = hwnet.instantiateComponent(usbBusSC);
    hwnet.connectInterface(hwnet.interfacesOf(ndlcomBus), hwnet.interfacesOf(unite(convId,unite(mdaqId,spineId)), "lvds1"));
    hwnet.connectInterface(hwnet.interfacesOf(ndlcomBus), hwnet.interfacesOf(spineId, "lvds2"));
    hwnet.connectInterface(hwnet.interfacesOf(usbBus,"usb0"), hwnet.interfacesOf(convId,"usb1"));
    hwnet.connectInterface(hwnet.interfacesOf(usbBus,"usb1"), hwnet.interfacesOf(laptopId,"/dev/ttyUSB0"));

    // We could now make the specific instances and the network they form PART-OF some X. This X would then represent all occurences of this setting/network.

    // Store real world example
    std::cout << "> Store hwnet using YAML" << std::endl;
    fout.open("demo.yml");
    if(fout.good()) {
        fout << YAML::StringFrom(hwnet) << std::endl;
    } else {
        std::cout << "FAILED\n";
    }
    fout.close();


    std::cout << "*** TEST DONE ***\n";
}
