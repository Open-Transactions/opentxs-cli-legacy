/* See other files here for the LICENCE that applies here. */

#ifndef INCLUDE_OT_NEWCLI_COMMON3
#define INCLUDE_OT_NEWCLI_COMMON3

#include "lib_common2.hpp"

// OT - like in Moneychanger
#ifdef _WIN32
	// TODO CmakeLists
	// TODO: return to otapi/OTAPI.hpp (when cmake is corrected to find the right path)
	#include <OTAPI.hpp>
	#include <OTAPI_Exec.hpp>
	#include <OT_ME.hpp>
#else
	// TODO: return to opentxs/OTAPI.hpp (when cmake is corrected to find the right path)
	#include <OTAPI.hpp>
	#include <OTAPI_Exec.hpp>
	#include <OT_ME.hpp>
#endif

#define INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3 \
	INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2 


#endif

