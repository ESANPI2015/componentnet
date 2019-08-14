#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "HypergraphYAML.hpp"
#include <fstream>

TEST_CASE("Construct and operate on a hardware network", "[Hardware::Computational::Network]")
{
    Hardware::Computational::Network hwnet;
    // Create a bunch of devices and interfaces
    // TODO: Include or make these processors if needed
    REQUIRE(hwnet.createDevice("DFKI::iStruct::mdaq2","mdaq2") == Hyperedges{"DFKI::iStruct::mdaq2"});
    hwnet.createDevice("PC","PC");
    hwnet.createDevice("DFKI::iStruct::spine_board","spine_board");
    hwnet.createDevice("DFKI::LVDS2USB", "lvds2usb");
    REQUIRE(hwnet.createInterface("LVDS","lvds") == Hyperedges{"LVDS"});
    hwnet.createInterface("USB","usb");
    // Assign interfaces to devices
    REQUIRE(hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::iStruct::mdaq2"}, Hyperedges{"LVDS"}, "lvds1").size() == 1);
    hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::iStruct::mdaq2"}, Hyperedges{"LVDS"}, "lvds2");
    hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::iStruct::spine_board"}, Hyperedges{"LVDS"}, "lvds1");
    hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::iStruct::spine_board"}, Hyperedges{"LVDS"}, "lvds2");
    hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::LVDS2USB"}, Hyperedges{"LVDS"}, "lvds1");
    REQUIRE(hwnet.instantiateInterfaceFor(Hyperedges{"DFKI::LVDS2USB"}, Hyperedges{"USB"}, "usb1").size() == 1);
    hwnet.instantiateInterfaceFor(Hyperedges{"PC"}, Hyperedges{"USB"}, "/dev/ttyUSB0");
    // Instantiate devices in an experimental setup
    hwnet.instantiateComponent(Hyperedges{"DFKI::iStruct::mdaq2"}, "TEST BOARD");
    REQUIRE(hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::iStruct::mdaq2"})).size() == hwnet.interfacesOf(Hyperedges{"DFKI::iStruct::mdaq2"}).size());
    hwnet.instantiateComponent(Hyperedges{"DFKI::iStruct::spine_board"});
    hwnet.instantiateComponent(Hyperedges{"DFKI::LVDS2USB"});
    hwnet.instantiateComponent(Hyperedges{"PC"}, "My Laptop");
    // Wire the things together
    hwnet.connectInterface(hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::iStruct::mdaq2"}), "lvds1"), hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::iStruct::spine_board"}), "lvds2"));
    hwnet.connectInterface(hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::iStruct::spine_board"}), "lvds1"), hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::LVDS2USB"}), "lvds1"));
    hwnet.connectInterface(hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"DFKI::LVDS2USB"}), "usb1"), hwnet.interfacesOf(hwnet.instancesOf(Hyperedges{"PC"}), "/dev/ttyUSB0"));
    // TODO: Check connections (by making a special traversal)
    // Store the graph (for usage in a real experiment?)
    std::ofstream fout;
    fout.open("hwnet.yml");
    REQUIRE(fout.good());
    fout << YAML::StringFrom(hwnet) << std::endl;
    fout.close();
}
