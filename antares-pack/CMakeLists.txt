add_executable(antares-pack main.c)
target_link_libraries(antares-pack banner anetlist)
install(TARGETS antares-pack DESTINATION bin)
