cmake_minimum_required(VERSION 2.8)

project(domaingraph)

include_directories(src)
include_directories(include)

SET(ENV{PKG_CONFIG_PATH}
    ${CMAKE_BINARY_DIR}:$ENV{PKG_CONFIG_PATH}
    )

# Yes, we have to add the directory at least
if(NOT TARGET hypergraph)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../hypergraph hypergraph)
endif(NOT TARGET hypergraph)

find_package(PkgConfig)
pkg_check_modules(domaingraph_PKGCONFIG REQUIRED
    hypergraph yaml-cpp
    )

include_directories(${domaingraph_PKGCONFIG_INCLUDE_DIRS})
#message(${domaingraph_PKGCONFIG_INCLUDE_DIRS})
link_directories(${domaingraph_PKGCONFIG_LIBRARY_DIRS})
add_definitions(${domaingraph_PKGCONFIG_CFLAGS_OTHER} -ggdb --pedantic -Wall -std=c++11)

set(SOURCES
    src/DomainSpecificGraph.cpp
    src/ComponentNetwork.cpp
    )
set(HEADERS
    include/DomainSpecificGraph.hpp
    include/ComponentNetwork.hpp
    )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(domaingraph STATIC ${SOURCES})
target_link_libraries(domaingraph
    ${domaingraph_PKGCONFIG_STATIC_LIBRARIES}
    )

add_executable(testDomainGraph test/testDomainGraph.cpp)
target_link_libraries(testDomainGraph domaingraph)
install(TARGETS testDomainGraph
RUNTIME DESTINATION bin)

add_executable(testComponentNetwork test/testComponentNetwork.cpp)
target_link_libraries(testComponentNetwork domaingraph)
install(TARGETS testComponentNetwork
RUNTIME DESTINATION bin)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)
