/* See other files here for the LICENCE that applies here. */
/*
All for ot hint functionality goes here
*/

#ifndef INCLUDE_OT_NEWCLI_othint
#define INCLUDE_OT_NEWCLI_othint

#include "cmd.hpp"
#include "lib_common2.hpp"

namespace nOT {
namespace nOTHint{

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2 // <=== namespaces
// Data for hinting, e.g. cached or local information.

extern shared_ptr<nNewcli::cCmdParser> gReadlineHandleParser;

// ====================================================================

class cInteractiveShell {
	protected:
		void _RunEditline(shared_ptr<nUse::cUseOT> use);
		void _RunOnce(const string line, shared_ptr<nUse::cUseOT> use);
		void _CompleteOnce(const string line, shared_ptr<nUse::cUseOT> use);

		bool Execute(const string cmd);

	public:
		cInteractiveShell();
		void RunOnce(const string line, shared_ptr<nUse::cUseOT> use);
		void CompleteOnce(const string line, shared_ptr<nUse::cUseOT> use);
		void RunEditline(shared_ptr<nUse::cUseOT> use);

		void CompleteOnceWithDaemon(const string & line);

	protected:
		bool dbg;
};


} // namespace nOTHint
} // namespace nOT



#endif

