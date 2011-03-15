add_executable(antares-route main.c)
target_link_libraries(antares-route banner anetlist chip)
install(TARGETS antares-route DESTINATION bin)
