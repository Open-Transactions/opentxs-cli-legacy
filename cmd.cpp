/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "cmd.hpp"

#include "lib_common2.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

class cCmdParser_pimpl {
	private:
		map< cCmdName , cCmdFormat > tree;
};

// ========================================================================================================================

void cCmdParser::Init() {
	mI->tree[ cCmdName("msg send") ] = cCmdFormat( 
			vector<cParamInfo>{ ARG_STR, ARG_STR, ARG_STR }, map<string,cParamInfo>{{"subject",ARG_STR}}, map<string,cParamInfo>{{"cc",ARG_STR}} ,
	 		[]( nUse::cUseOT &useOt , cCmdData &data ) { 
				string msg=data.arg(3,""); if (0==msg.length()) msg=nUtils::GetMultiline(); 
				useOt->msgSend( data.arg(1), data.arg(2), msg ); }
			);

	
	typedef vector<cParamInfo> vpar;
	typedef map<string,cParamInfo> mopt;

	mI->tree[ cCmdName("msg send") ] = cCmdFormat( useOt::msgList, 
		vpar{ ARG_STR, ARG_STR, ARG_STR },  vpar{},  vopt{{"subject",ARG_STR}},  mopt{{"cc",ARG_STR}, {"bcc",ARG_STR}} );

}

cCmdProcessing cCmdParser::StartProcessing(const vector<string> &words) {
	return cCmdProcessing( shared_from_this() , words );
}

cCmdProcessing cCmdParser::StartProcessing(const string &words) {
	return cCmdProcessing( shared_from_this() , nUtils::SplitString(words) );
}

void cCmdParser::Test() {
	cout << "Test test, object at " << (void*)this << endl;
}


cCmdProcessing::cCmdProcessing(shared_ptr<cCmdParser> _parser, vector<string> _commandLine)
	: parser(_parser), commandLine(_commandLine)
{ }

cParamInfo::cParamInfo(tFuncValid valid, tFuncHint hint) 
	: funcValid(valid), funcHint(hint)
{ }


void cmd_test() {
	_note("TEST TREE");

	cCmdParser parser;
	parser.Prepare();
	auto processing = parser.StartProcessing("ot msg ls");

}

}; // namespace nNewcli
}; // namespace OT


