set(xdlrc)
foreach(_chip ${SUPPORTED_CHIPS})
	set(_xdlrc "${_chip}.xdlrc")
	add_custom_command(
		OUTPUT ${_xdlrc} 
		COMMAND xdl -report -pips ${_chip} > /dev/null
	) 
	list(APPEND xdlrc ${_xdlrc})
endforeach()

set(acg)
foreach(_chip ${SUPPORTED_CHIPS})
	set(_xdlrc "${_chip}.xdlrc")
	set(_acg "${_chip}.acg.gz")
	add_custom_command(
		OUTPUT ${_acg} 
		DEPENDS ${PROJECT_BINARY_DIR}/antares-mkdb/antares-mkdb ${_xdlrc}
		COMMAND ${PROJECT_BINARY_DIR}/antares-mkdb/antares-mkdb ${_xdlrc}
	) 
	list(APPEND acg ${PROJECT_BINARY_DIR}/data/${_acg})
endforeach()

add_custom_target(data ALL DEPENDS ${acg})
install(FILES ${acg} DESTINATION share/antares)
