add_executable(antares-pack main.c transform.c)
target_link_libraries(antares-pack banner anetlist)
install(TARGETS antares-pack DESTINATION bin)
