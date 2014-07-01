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

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces
// Data for hinting, e.g. cached or local information.

extern shared_ptr<nNewcli::cCmdParser> gReadlineHandleParser;

class cHintData {
	public:
		cHintData();
};

// ====================================================================

// The Manager to access OT-hint (autocompletion) functionality
class cHintManager {

	public:
		cHintManager();

		vector<string> AutoComplete(const string &sofar_str) const; // the main function to auto-complete. The command line after "ot ", e.g. "msg send al"
		vector<string> AutoCompleteEntire(const string &sofar_str) const; // the same, but takes entire command line including "ot ", e.g. "ot msg send al"

		void TestNewFunction_Tree(); // testing new code [wip]

	protected:
		vector<string> BuildTreeOfCommandlines(const string &sofar_str, bool show_all) const; // return command lines tree that is possible from this place
		unique_ptr<cHintData> mHintData;
};

// ====================================================================

class cInteractiveShell {
	protected:
		void _runEditline(shared_ptr<nUse::cUseOT> use);
		void _runOnce(const string line, shared_ptr<nUse::cUseOT> use);

	public:
		cInteractiveShell();
		void runOnce(const string line, shared_ptr<nUse::cUseOT> use);
		void runEditline(shared_ptr<nUse::cUseOT> use);

	protected:
		bool dbg;
};


}; // namespace nOTHint
}; // namespace nOT



#endif

