# primitives
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/primitives.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 0 c > ${CMAKE_CURRENT_SOURCE_DIR}/primitives.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 0 h > ${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

set_property(
	SOURCE primitives.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
)


# BELs
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bels.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 1 c > ${CMAKE_CURRENT_SOURCE_DIR}/bels.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/anetlist/bels.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 1 h > ${PROJECT_SOURCE_DIR}/include/anetlist/bels.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

set_property(
	SOURCE bels.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/anetlist/bels.h
)

# sites
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sites.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 2 c > ${CMAKE_CURRENT_SOURCE_DIR}/sites.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py 2 h > ${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

set_property(
	SOURCE sites.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
)

#

add_library(anetlist primitives.c bels.c sites.c)
