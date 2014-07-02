/* See other files here for the LICENCE that applies here. */
/*
Tools for writting a daemon
*/

#ifndef INCLUDE_OT_NEWCLI_daemon_tools
#define INCLUDE_OT_NEWCLI_daemon_tools

#include "lib_common1.hpp"

namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

class cDaemoninfo {
	public:
		virtual bool IsRunning() const =0;
		virtual string GetPathIn() const =0;
		virtual string GetPathOut() const =0;
};

class cDaemoninfoComplete : public cDaemoninfo {
	public:
		virtual bool IsRunning() const ;
		virtual string GetPathIn() const ;
		virtual string GetPathOut() const ;
};


}; // namespace nOT



#endif

