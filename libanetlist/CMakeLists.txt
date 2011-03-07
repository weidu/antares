add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/primitives.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py pc > ${CMAKE_CURRENT_SOURCE_DIR}/primitives.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py ph > ${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

set_property(
	SOURCE primitives.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/anetlist/primitives.h
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sites.c
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py sc > ${CMAKE_CURRENT_SOURCE_DIR}/sites.c
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py sh > ${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mk_entities.py
)

set_property(
	SOURCE sites.c
	PROPERTY OBJECT_DEPENDS
	${PROJECT_SOURCE_DIR}/include/anetlist/sites.h
)

add_library(anetlist primitives.c sites.c)
