cmake_minimum_required(VERSION 2.8)

project(componentnet)

include_directories(src)
include_directories(include)

SET(ENV{PKG_CONFIG_PATH}
    ${CMAKE_BINARY_DIR}:$ENV{PKG_CONFIG_PATH}
    )

# Yes, we have to add the directory at least
if(NOT TARGET hypergraph)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../hypergraph hypergraph)
endif(NOT TARGET hypergraph)
if(NOT TARGET yaml-cpp)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../ext/yaml-cpp yaml-cpp)
endif(NOT TARGET yaml-cpp)

find_package(PkgConfig)
pkg_check_modules(componentnet_PKGCONFIG REQUIRED
    hypergraph yaml-cpp
    )

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 11)
include_directories(${componentnet_PKGCONFIG_INCLUDE_DIRS})
#message(${componentnet_PKGCONFIG_INCLUDE_DIRS})
link_directories(${componentnet_PKGCONFIG_LIBRARY_DIRS})
add_definitions(${componentnet_PKGCONFIG_CFLAGS_OTHER} --pedantic -Wall)

set(SOURCES
    src/ComponentNetwork.cpp
    src/HardwareComputationalNetwork.cpp
    src/SoftwareGraph.cpp
    src/ResourceCostModel.cpp
    )
set(HEADERS
    include/ComponentNetwork.hpp
    include/HardwareComputationalNetwork.hpp
    include/SoftwareGraph.hpp
    include/ResourceCostModel.hpp
    )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(componentnet STATIC ${SOURCES})
target_link_libraries(componentnet
    ${componentnet_PKGCONFIG_STATIC_LIBRARIES}
    )

add_executable(testComponentNetwork test/testComponentNetwork.cpp)
target_link_libraries(testComponentNetwork componentnet)
install(TARGETS testComponentNetwork
RUNTIME DESTINATION bin)

add_executable(testHWNetwork test/testHWNetwork.cpp)
target_link_libraries(testHWNetwork componentnet)
install(TARGETS testHWNetwork
RUNTIME DESTINATION bin)

add_executable(testMapping test/testMapping.cpp)
target_link_libraries(testMapping componentnet)
install(TARGETS testMapping
RUNTIME DESTINATION bin)

add_executable(gen_vhdl_entity tools/gen_vhdl_entity.cpp)
target_link_libraries(gen_vhdl_entity componentnet)
install(TARGETS gen_vhdl_entity
RUNTIME DESTINATION bin)

add_executable(gen_cpp_class tools/gen_cpp_class.cpp)
target_link_libraries(gen_cpp_class componentnet)
install(TARGETS gen_cpp_class
RUNTIME DESTINATION bin)

add_executable(sw2hw_map tools/sw2hw_map.cpp)
target_link_libraries(sw2hw_map componentnet)
install(TARGETS sw2hw_map
RUNTIME DESTINATION bin)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)
