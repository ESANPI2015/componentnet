# Component Networks

Library &amp; tools based on concept graphs to
* define components and networks thereof
* derive more specialized subdomains e.g. processor networks

Starting from the common concept graphs defined in the hypergraph library
the concepts of
* components
* interfaces
* networks
are introduced.
Having these classes, a vast amount of more specific models can be embedded into either of the introduced classes.
These models can encode e.g. task networks, processor networks and other domains.

## State

* The concept of component networks has been introduced and conventient methods have been grouped into ComponentNetwork.hpp
* Derived classes from component networks:
  - SoftwareGraph: Introduces abstract algorithms, their inputs and outputs and implementations realizing the algorithms (and are themselves algorithms)
  - HardwareComputationalNetwork: Introduces devices, processors and their interfaces for specification of processing networks
  - ResourceCostModel: Introduces resources, their providers and consumers as well as costs given a pair (provider, consumer). Can be used for mapping.
* Tools:
  - gen_cpp_class: Given an abstract algorithm, this generator tries to create a C++ class skeleton for it
  - gen_vhdl_entity: Analogously to gen_cpp_class, this generator tries to produce VHDL entity skeletons
  - gen_impl_networks: Given a algorithm network, this generator generates all possible implementation networks from it.
  - sw2hw_map: Given an implementation network, a processor network and a resource cost model, this tool tries to map implementations to processors greedily.

## TODO

* The tools should be just calling classes ... e.g. the gen\_\* tools should make use of a generator class.
* Do not use implicit information: No clk, rst in VHDL, simple interface classes/component classes in C++.
* Add artifical intelligence approaches? e.g. learn good selection/pruning strategies during pattern matching?
