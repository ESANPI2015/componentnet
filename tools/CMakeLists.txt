cmake_minimum_required(VERSION 3.2)

add_executable(gen_vhdl_entity gen_vhdl_entity.cpp)
target_link_libraries(gen_vhdl_entity ${PROJECT_NAME})
install(TARGETS gen_vhdl_entity RUNTIME DESTINATION bin)

add_executable(gen_cpp_class gen_cpp_class.cpp)
target_link_libraries(gen_cpp_class ${PROJECT_NAME})
install(TARGETS gen_cpp_class RUNTIME DESTINATION bin)

add_executable(sw2hw_map sw2hw_map.cpp)
target_link_libraries(sw2hw_map ${PROJECT_NAME})
install(TARGETS sw2hw_map RUNTIME DESTINATION bin)

add_executable(gen_impl_networks gen_impl_networks.cpp)
target_link_libraries(gen_impl_networks ${PROJECT_NAME})
install(TARGETS gen_impl_networks RUNTIME DESTINATION bin)

