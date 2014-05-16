/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "runoptions.hpp"

#include "lib_common1.hpp"

namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

// (no debug)
// +debug ...... --asdf
// +debug +debugcerr .... --asfs
// +debug +debugfile .... --asfs

cRunOptions::cRunOptions()
	: mRunMode(eRunModeCurrent)
{ }

cRunOptions gRunOptions;

cRunOptions gRunOptions; // (extern)

}; // namespace OT


