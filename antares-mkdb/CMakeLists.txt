add_executable(antares-mkdb main.c xdlrc.c typelist.c connectivity.c chip.c)
target_link_libraries(antares-mkdb banner chip)
install(TARGETS antares-mkdb DESTINATION bin)
