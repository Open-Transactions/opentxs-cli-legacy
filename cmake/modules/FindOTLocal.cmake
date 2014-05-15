# Find OT lib and include directories
message(STATUS "Looking for OT locally.")
find_path(OTLocal_INCLUDE_DIR
	NAMES OTAPI.hpp
	PATHS "$ENV{HOME}/.local/include/opentxs"
	)

find_library(OTLocal_LIBRARY
	NAMES otapi
	PATHS "$ENV{HOME}/.local/lib"
	)

if (OTLocal_LIBRARY AND OTLocal_INCLUDE_DIR )
	set(OTLocal_LIBRARIES ${OTLocal_LIBRARY})
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
