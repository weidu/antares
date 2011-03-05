add_library(chip db.c store.c load.c)
target_link_libraries(chip ${ZLIB_LIBRARIES})
