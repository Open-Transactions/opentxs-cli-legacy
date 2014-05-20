# Find OT lib and include directories
message(STATUS "Looking for OT locally.")
find_path(OTLocal_INCLUDE_DIR
	NAMES OTAPI.hpp
	PATHS "$ENV{HOME}/.local/include/opentxs"
	)

find_library(OTLocal_ot
	NAMES ot
	PATHS "$ENV{HOME}/.local/lib"
	)
	
find_library(OTLocal_otapi
	NAMES otapi
	PATHS "$ENV{HOME}/.local/lib"
	)

if (OTLocal_ot AND OTLocal_otapi AND OTLocal_INCLUDE_DIR )
	set( OTLocal_LIBRARIES ${OTLocal_ot} ${OTLocal_otapi} )
	set(OTLocal_FOUND "YES")
else ()
	set(OTLocal_FOUND "NO")
endif ()

if (OTLocal_FOUND)
	if (NOT OTLocal_FIND_QUIETLY)
		message(STATUS "OT include directory: ${OTLocal_INCLUDE_DIR}")
		message(STATUS "OT libraries found: ${OTLocal_LIBRARIES}")
	endif ()
else ()
	if (OTLocal_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find required OT libraries")
	endif ()
endif ()

mark_as_advanced(
	OTLocal_INCLUDE_DIR
	OTLocal_LIBRARY
	)
