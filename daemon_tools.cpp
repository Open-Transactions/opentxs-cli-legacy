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
	// _dbg3("Testing is daemon running");
	struct stat buf;
	int stat_err = stat(  GetPathIn().c_str() , & buf);
	return stat_err == 0;
}

string cDaemoninfoComplete::GetPathIn() const {
	return "/tmp/ot.in";
}

string cDaemoninfoComplete::GetPathOut() const {
	if (!mOutCreated) throw std::runtime_error("Trying to read PatchOut before creating it");
	return mOutFilename;
}

string cDaemoninfoComplete::GetPathOutFlag() const {
	return GetPathOut() + ".ready";
}

void cDaemoninfoComplete::CreateOut() {
	// TODO mktemp
	mOutFilename = "/tmp/ot." + ToStr(rand()%10000) + ToStr(time(NULL)) + ".out";
	mOutCreated = true;
}

bool cDaemoninfoComplete::IsReadyPatchOut() const {
	string fname = GetPathOutFlag();
	struct stat buf;
	int stat_err = stat( fname.c_str() , & buf);
	return stat_err == 0;
}

}; // namespace OT


