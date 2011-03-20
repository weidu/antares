add_executable(antares-place main.c rtree.c resmgr.c constraints.c ucf.c initial.c)
target_link_libraries(antares-place banner anetlist chip ucfparse mtwist)
add_executable(rtreetest rtreetest.c rtree.c)
install(TARGETS antares-place DESTINATION bin)
