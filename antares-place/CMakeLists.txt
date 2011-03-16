add_executable(antares-place main.c resmgr.c)
target_link_libraries(antares-place banner anetlist chip mtwist)
install(TARGETS antares-place DESTINATION bin)
