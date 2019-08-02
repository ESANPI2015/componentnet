add_definitions(--pedantic -Wall)
set(SOURCES
    ComponentNetwork.cpp
    HardwareComputationalNetwork.cpp
    SoftwareNetwork.cpp
    ResourceCostModel.cpp
    Mapper.cpp
    Generator.cpp
    VHDLGenerator.cpp
    )
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME} hypergraph)
