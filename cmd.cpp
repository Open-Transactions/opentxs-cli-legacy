/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "cmd.hpp"

#include "lib_common2.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

// ========================================================================================================================

class cCmdParser_pimpl {
	friend class cCmdParser;

	private:
		map< cCmdName , cCmdFormat > tree;
};

cCmdParser::cCmdParser() 
: mI( new cCmdParser_pimpl )
{ }

void Execute1() {
	_mark("***Execute1***");
}

void Execute2() {
	_mark("***Execute2***");
}


void cCmdParser::AddFormat( const cCmdName &name, const cCmdFormat &format ) {
	typedef map< cCmdName , cCmdFormat >::value_type tMapPair; // type of element (pair) in tree-map. TODO: will be not needed in C+11 map emplace
	mI->tree.insert( tMapPair ( name , format ) );
}

void cCmdParser::Init() {
	_mark("Init tree");

/*	 std::map<char,int> mymap;
	   mymap.emplace('x',100);
		 */

	cCmdExecutable exec( Execute1 );
	cCmdFormat::tVar var;
	cCmdFormat msg_send_format( exec, var );
	AddFormat( cCmdName("msg send") , msg_send_format );

	//mI->tree.emplace( cCmdName("msg send") , msg_send_format );
	
//	mI->tree[ cCmdName("msg send") ] = msg_send_format;

	// msg sendfrom bob alice
	// msg sendfrom bob alice HelloThisIsATest // TODO, other call to OTUse, just pass the message
	// msg sendfrom bob alice "Hello This Is A Test" // TODO, need parser+editline support for quotes

/*	mI->tree[ cCmdName("msg send") ] = cCmdFormat( 
			vector<cParamInfo>{ ARG_STR, ARG_STR, ARG_STR }, map<string,cParamInfo>{{"subject",ARG_STR}}, map<string,cParamInfo>{{"cc",ARG_STR}} ,
	 		[]( nUse::cUseOT &useOt , cCmdData &data ) { 
				string msg=data.arg(3,""); if (0==msg.length()) msg=nUtils::GetMultiline(); 
				useOt->msgSend( data.arg(1), data.arg(2), msg ); }
			);

	
	typedef vector<cParamInfo> vpar;
	typedef map<string,cParamInfo> mopt;

	mI->tree[ cCmdName("msg send") ] = cCmdFormat( useOt::msgList, 
		vpar{ ARG_STR, ARG_STR, ARG_STR },  vpar{},  vopt{{"subject",ARG_STR}},  mopt{{"cc",ARG_STR}, {"bcc",ARG_STR}} );
*/
}


cCmdProcessing cCmdParser::StartProcessing(const vector<string> &words) {
	return cCmdProcessing( shared_from_this() , words );
}

cCmdProcessing cCmdParser::StartProcessing(const string &words) {
	return cCmdProcessing( shared_from_this() , nUtils::SplitString(words) );
}

// ========================================================================================================================

cCmdName::cCmdName(const string &name) : mName(name) { }

bool cCmdName::operator<(const cCmdName &other) const { return mName < other.mName; }


cCmdProcessing::cCmdProcessing(shared_ptr<cCmdParser> _parser, vector<string> _commandLine)
	: parser(_parser), commandLine(_commandLine)
{ }

vector<string> cCmdProcessing::UseComplete(nUse::cUseOT &use) {
	vector<string> ret;
	return ret;
}

void cCmdProcessing::UseExecute(nUse::cUseOT &use) {
	use.msgSend("bob", "alice", "from-code-1");
}

// ========================================================================================================================

cParamInfo::cParamInfo(tFuncValid valid, tFuncHint hint) 
	: funcValid(valid), funcHint(hint)
{ }

// ========================================================================================================================

cCmdFormat::cCmdFormat(cCmdExecutable exec, tVar var)
	:	mExec(exec), mVar(var)
{
}

// ========================================================================================================================

cCmdExecutable::cCmdExecutable(tFunc func) : mFunc(func) { }

// ========================================================================================================================

void cmd_test() {
	_mark("TEST TREE");

	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();
	auto processing = parser->StartProcessing("ot msg ls");

	_mark("STARTING EXEC");
	processing.UseExecute( nUse::useOT );
	_mark("DONE TEST");
}

}; // namespace nNewcli
}; // namespace OT


