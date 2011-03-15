add_executable(antares-place main.c)
target_link_libraries(antares-place banner anetlist chip)
install(TARGETS antares-place DESTINATION bin)
