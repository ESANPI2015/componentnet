cmake_minimum_required(VERSION 3.2)

add_executable(testComponentNetwork testComponentNetwork.cpp)
target_link_libraries(testComponentNetwork ${PROJECT_NAME})
install(TARGETS testComponentNetwork RUNTIME DESTINATION bin)
add_test(testComponentNetwork testComponentNetwork)
add_custom_command(TARGET testComponentNetwork POST_BUILD COMMAND testComponentNetwork)

add_executable(testHWNetwork testHWNetwork.cpp)
target_link_libraries(testHWNetwork ${PROJECT_NAME})
install(TARGETS testHWNetwork RUNTIME DESTINATION bin)

add_executable(testMapping testMapping.cpp)
target_link_libraries(testMapping ${PROJECT_NAME})
install(TARGETS testMapping RUNTIME DESTINATION bin)

add_executable(createEmptyNetworks createEmptyNetworks.cpp)
target_link_libraries(createEmptyNetworks ${PROJECT_NAME})
install(TARGETS createEmptyNetworks RUNTIME DESTINATION bin)

add_executable(testGenerator testGenerator.cpp)
target_link_libraries(testGenerator ${PROJECT_NAME})
install(TARGETS testGenerator RUNTIME DESTINATION bin)

