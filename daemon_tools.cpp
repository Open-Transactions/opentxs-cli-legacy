/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "daemon_tools.hpp"

#include "lib_common2.hpp"

namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

bool cDaemoninfoComplete::IsRunning() const {
	std::ifstream ifs( GetPathIn().c_str() );

	_note("ifs good" << ifs.good());
	_note("ifs bad" << ifs.bad());
	_note("ifs eof" << ifs.eof());

	return ifs.good();
}

string cDaemoninfoComplete::GetPathIn() const {
	return "/tmp/ot.in";
}

string cDaemoninfoComplete::GetPathOut() const {
	return "/tmp/ot." + ToStr(rand()%10000) + ToStr(time(NULL)) + ".out";
}


}; // namespace OT


