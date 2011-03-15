cmake_minimum_required(VERSION 2.8.2)

# a little deGNU'ing is always good
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Antares)

set(CMAKE_C_FLAGS "-O2 -Wall -D_GNU_SOURCE -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

set(ANTARES_VERSION_MAJOR 0)
set(ANTARES_VERSION_MINOR 0)
set(ANTARES_VERSION_PATCH 1)

configure_file(
	"${PROJECT_SOURCE_DIR}/include/config.h.in"
	"${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_BINARY_DIR}")

include_directories("${PROJECT_SOURCE_DIR}/include")

# system libraries

find_package(ZLIB REQUIRED)
include_directories("${ZLIB_INCLUDE_DIRS}")

# subdirectories

add_subdirectory(libbanner)
add_subdirectory(libanetlist)
add_subdirectory(libchip)

add_subdirectory(antares-mkdb)
add_subdirectory(antares-pack)
add_subdirectory(antares-place)
add_subdirectory(antares-route)
add_subdirectory(test-router)

# packaging

include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Physical implementation tools for Xilinx FPGAs")
set(CPACK_PACKAGE_VENDOR "Sebastien Bourdeauducq")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "${ANTARES_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${ANTARES_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${ANTARES_VERSION_PATCH}")
include(CPack)

