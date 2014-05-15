/* See other files here for the LICENCE that applies here. */
/*
This class represents an application of otcli,
that runs either in mode:
- mode to execute commands
- mode to hint/complete/shell-complete commands
*/

#ifndef INCLUDE_OT_NEWCLI
#define INCLUDE_OT_NEWCLI

#include "lib_common2.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

class cOTCli {
	public:
		int Run(const int argc, const char **argv);

		bool LoadScript_Main(const std::string &thefile_name);
		void LoadScript(const std::string &script_filename, const std::string &title);
};


}; // namespace nNewcli
}; // namespace nOT



#endif

