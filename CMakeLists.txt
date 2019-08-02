cmake_minimum_required(VERSION 2.8)

project(componentnet)

# Use a .in file to get a correct installation of yaml-cpp
configure_file(CMakeLists.hypergraph.txt.in ext/hypergraph/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ext/hypergraph" )
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ext/hypergraph" )
add_subdirectory("${CMAKE_CURRENT_BINARY_DIR}/hypergraph-src"
                 "${CMAKE_CURRENT_BINARY_DIR}/hypergraph-build")

set(CMAKE_CXX_STANDARD 11)
include_directories(include)
add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PUBLIC include)
add_subdirectory(tools)
enable_testing()
add_subdirectory(test)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)
