/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "daemon_tools.hpp"

#include "lib_common2.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

bool cDaemoninfoComplete::IsRunning() const {
	_info("Testing is daemon running");

	struct stat buf;
	int stat_err = stat(  GetPathIn().c_str() , & buf);

	/*
	std::ofstream fs( GetPathIn().c_str() );

	_note("fs good" << fs.good());
	_note("fs bad" << fs.bad());
	_note("fs eof" << fs.eof());
*/
	return stat_err == 0;
}

string cDaemoninfoComplete::GetPathIn() const {
	return "/tmp/ot.in";
}

string cDaemoninfoComplete::GetPathOut() const {
	return "/tmp/ot." + ToStr(rand()%10000) + ToStr(time(NULL)) + ".out";
}


}; // namespace OT


