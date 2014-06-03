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

	typedef map< cCmdName , shared_ptr<cCmdFormat> >::value_type tTreePair; // type of element (pair) in tree-map. TODO: will be not needed in C+11 map emplace

	private:
		map< cCmdName , shared_ptr<cCmdFormat> > mTree;
};

cCmdParser::cCmdParser() 
: mI( new cCmdParser_pimpl )
{ }

cCmdExecutable::tExitCode Execute1( shared_ptr<cCmdData> , nUse::cUseOT ) {
	_mark("***Execute1***");
	return cCmdExecutable::sSuccess;
}

void cCmdParser::AddFormat( const cCmdName &name, shared_ptr<cCmdFormat> format ) {
	mI->mTree.insert( cCmdParser_pimpl::tTreePair ( name , format ) );
	_info("Add format for command name (" << (string)name << "), now size=" << mI->mTree.size());
}

void cCmdParser::Init() {
	_mark("Init tree");

/*	 std::map<char,int> mymap;
	   mymap.emplace('x',100);
		 */

	cCmdExecutable exec( Execute1 );
	cCmdFormat::tVar var;
	// 1) cCmdFormat * format = new ... ;
	// 2) shared_ptr<cCmdFormat> format( new cCmdFormat(exec, var ) )   
	auto format = std::make_shared< cCmdFormat >( exec , var );
	AddFormat( cCmdName("msg ls") , format );

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

shared_ptr<cCmdFormat> cCmdParser::FindFormat( const cCmdName &name ) 
	throw(cErrCommandNotFound)
{
	auto it = mI->mTree.find( name );
	if (it == mI->mTree.end()) {
		throw cErrCommandNotFound("No such ot command can be parsed by this parser: (" + (string)name + ")");
	}

	return it->second;
}

// ========================================================================================================================

cCmdName::cCmdName(const string &name) : mName(name) { }

bool cCmdName::operator<(const cCmdName &other) const { return mName < other.mName; }

cCmdName::operator std::string() const { return mName; }

// ========================================================================================================================


cCmdProcessing::cCmdProcessing(shared_ptr<cCmdParser> parser, vector<string> commandLine)
	: mParser(parser), mCommandLine(commandLine)
{ 
	_dbg3("Creating processing of: " << DbgVector(commandLine) );
}


void cCmdProcessing::Parse() {
	// mCommandLine = ot, msg, sendfrom, alice, bob, hello

	// mFormat.erase ? // remove old format, we parse something new [doublecheck]

	if (!mCommandLine.empty()) {
		if (mCommandLine.at(0) != "ot") {
			_warn("Command for processing is mallformed");
		}
		mCommandLine.erase( mCommandLine.begin() ); // delete the first "ot"
	} else {
		_warn("Command for processing is empty");
	}
	// mCommandLine = msg, sendfrom, alice, bob, hello
	_dbg1("Parsing: " << DbgVector(mCommandLine) );

	// msg send
	// msg ls
	// always 2 words are the command (we assume there are no sub-command)
	string name = mCommandLine.at(0) + " " + mCommandLine.at(1) ; // "msg send"
	try {
		mFormat = mParser->FindFormat( name );

		// ...
		_info("Got format for name="<<name);

	} catch (cErrCommandNotFound &e) {
		_warn("Command not found: " << e.what());
	}
}

vector<string> cCmdProcessing::UseComplete(nUse::cUseOT &use) {
	vector<string> ret;
	return ret;
}

void cCmdProcessing::UseExecute(nUse::cUseOT &use) {
	cCmdExecutable exec = mFormat->getExec();
	auto data = std::make_shared<cCmdData>() ; // TODO get real data from myself (from processing)
	exec( data , use ); 
	
	// test case, first create both users using:
	// ot nym new bob x
	// ot nym register bob x
	// use.MsgSend("alice", "bob", "from-code-1");
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

cCmdExecutable cCmdFormat::getExec() const {
	return mExec;
}

cCmdExecutable::tExitCode cCmdExecutable::operator()( shared_ptr<cCmdData> data, nUse::cUseOT & useOt) {
	_info("Executing function");
	int ret = mFunc( data , useOt );
	_info("Execution ret="<<ret);
	return ret;
}

cCmdExecutable::cCmdExecutable(tFunc func) : mFunc(func) { }

const cCmdExecutable::tExitCode cCmdExecutable::sSuccess = 0; 

// ========================================================================================================================

void cmd_test() {
	_mark("TEST TREE");

	shared_ptr<cCmdParser> parser(new cCmdParser);

	auto alltest = vector<string>{ "ot msg ls" , "ot msg senfrom alice bob", "ot msg show", "ot msg show alice" };  
	for (auto cmd : alltest) {
		parser->Init();
		auto processing = parser->StartProcessing(cmd);
		processing.Parse();
		processing.UseExecute( nUse::useOT );
	}

}

}; // namespace nNewcli
}; // namespace OT


