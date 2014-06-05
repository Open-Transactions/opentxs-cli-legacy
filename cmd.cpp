/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "cmd.hpp"

#include "lib_common2.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

using namespace nUse;

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
	/*
	typedef function< bool ( cUseOT &, cCmdData &, int, const string &  ) > tFuncValid;
	typedef function< vector<string> ( cUseOT &, cCmdData &, int, const string &  ) > tFuncHint;
	cParamInfo(tFuncValid valid, tFuncHint hint);
	*/

	cParamInfo pNymFrom(
		[] (cUseOT & use, cCmdData & data, int, const string &) -> bool { //FIXME 3 argument -> what for?
			_dbg3("Sender Nym validation");
			return use.NymCheckIfExists(data.Var(1));
		} ,
		[] ( cUseOT & use, cCmdData & data, int, const string &  ) -> vector<string> {
			_dbg3("Sender Nym hinting");
			return use.NymGetAllNames();
		}
	);
	cParamInfo pNymTo = pNymFrom; // TODO suggest not the same nym as was used already before
	cParamInfo pNymAny = pNymFrom;

	cParamInfo pOnceInt(
		[] (cUseOT & use, cCmdData & data, int, const string &) -> bool {
			// TODO check if is any integer
			// TODO check if not present in data
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, int, const string &  ) -> vector<string> {
			return vector<string> { "-1", "0", "1", "2", "100" };
		}
	);

	cParamInfo pSubject(
		[] (cUseOT & use, cCmdData & data, int, const string &) -> bool {
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, int, const string &  ) -> vector<string> {
			return vector<string> { "hello","hi","test","subject" };
		}
	);

//	Prepare format for all msg commands:
//	 	 "ot msg ls"
//		,"ot msg ls alice"
//		,"ot msg sendfrom alice bob hello --cc eve --cc mark --bcc john --prio 4"
//		,"ot msg sendto bob hello --cc eve --cc mark --bcc john --prio 4"
//		,"ot msg rm alice 0"
//		,"ot msg-out rm alice 0"


	{ // msg ls alice <-- FIXME alice is an extra variable. exec must call different exec function based on existance of extra argument.
		cCmdExecutable exec(
			[] ( shared_ptr<cCmdData> data, nUse::cUseOT use ) -> cCmdExecutable::tExitCode {
				if ( data->VarDef(1,"") == "" ) {
					_dbg3("Execute MsgGetAll()");
					use.MsgGetAll();
				}else {
					_dbg3("Execute MsgGetforNym(" + data->Var(1) + ")");
					use.MsgGetForNym(data->Var(1));
				}
				return 0;
			}
		);
		cCmdFormat::tVar var;
		cCmdFormat::tVar varExt;
			varExt.push_back( pNymAny );
		cCmdFormat::tOption opt;
		auto format = std::make_shared< cCmdFormat >( exec , var, varExt, opt );
		AddFormat( cCmdName("msg ls") , format );
	}

	{
		// ot msg sendfrom alice bob 
		// ot msg sendfrom NYM_FROM NYM_TO 
		cCmdExecutable exec(
			[] ( shared_ptr<cCmdData> data, nUse::cUseOT use) -> cCmdExecutable::tExitCode {
				_mark("Sending from inside lambda!");
				use.MsgSend( data->Var(1), data->Var(2), data->VarDef(3,"no_subject") );
				return 0;
			}
		);
		cCmdFormat::tVar var;
			var.push_back( pNymFrom );
			var.push_back( pNymTo );
		cCmdFormat::tVar varExt;
			varExt.push_back( pSubject );
		cCmdFormat::tOption opt;
			opt.insert(std::make_pair("--cc" , pNymAny));
			opt.insert(std::make_pair("--bcc" , pNymAny));
			opt.insert(std::make_pair("--prio" , pOnceInt));

		auto format = std::make_shared< cCmdFormat >( exec , var, varExt, opt );
		AddFormat( cCmdName("msg sendfrom") , format );
	}
	
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


cCmdProcessing cCmdParser::StartProcessing(const vector<string> &words, shared_ptr<nUse::cUseOT> use ) {
	return cCmdProcessing( shared_from_this() , words , use );
}

cCmdProcessing cCmdParser::StartProcessing(const string &words, shared_ptr<nUse::cUseOT> use ) {
	_dbg3("Will split words: [" << words << "]");
	return cCmdProcessing( shared_from_this() , nUtils::SplitString(words) , use );
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


cCmdProcessing::cCmdProcessing(shared_ptr<cCmdParser> parser, vector<string> commandLine, shared_ptr<nUse::cUseOT> &use )
	: mParser(parser), mCommandLine(commandLine), mUse(use)
{ 
	_dbg2("Creating processing of: " << DbgVector(commandLine) );
	_dbg2("Working on use=" << use->DbgName() );
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
	const string name = mCommandLine.at(0) + " " + mCommandLine.at(1) ; // "msg send"
	const size_t words_count = mCommandLine.size();

	_dbg3("Alloc data");  
	mData = std::make_shared<cCmdData>();

	int phase=0; // 0: cmd name  1:var, 2:varExt  3:opt  //FIXME How to check if variable is varExt?
	try {
		mFormat = mParser->FindFormat( name );
		_info("Got format for name="<<name);

		phase=1;
		int pos=2; // msg send -->

		while (true) { // parse var
			if (pos >= words_count) { _dbg1("reached end, pos="<<pos);
				break;
			}

			_dbg2("phase="<<phase<<" pos="<<pos);
			string word = mCommandLine.at(pos);
			_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
			++pos;

			if ( nUtils::CheckIfBegins("\"", word) ) {
				_dbg1("Quotes detected in: " + word);
				word.erase(0,1);
				while ( !nUtils::CheckIfEnds("\"", word) ) {
					word += mCommandLine.at(pos);
					++pos;
				}
				word.erase(word.end(), word.end()-1);
			}
			if (nUtils::CheckIfBegins("--", word)) { // --bcc foo
				phase=3;
				break; // continue to phase 3 - the options
			}

			_dbg1("adding var "<<word);  mData->mVar.push_back( word ); 
		} // parse var

		_note("mVar parsed:    " + DbgVector(mData->mVar));
		_note("mVarExt parsed: " + DbgVector(mData->mVarExt));
		_note("mOption parsed  " + DbgMap(mData->mOption));
 
	} catch (cErrCommandNotFound &e) {
		_warn("Command not found: " << e.what());
	}
}

vector<string> cCmdProcessing::UseComplete() {
	vector<string> ret;
	return ret;
}

void cCmdProcessing::UseExecute() {
	if (!mFormat) { _warn("Can not execute this command - mFormat is empty"); return; }
	cCmdExecutable exec = mFormat->getExec();
	exec( mData , *mUse ); 
}

// ========================================================================================================================

cParamInfo::cParamInfo(tFuncValid valid, tFuncHint hint) 
	: funcValid(valid), funcHint(hint)
{ }

// ========================================================================================================================

cCmdFormat::cCmdFormat(cCmdExecutable exec, tVar var, tVar varExt, tOption opt) 
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

string cCmdData::VarAccess(int nr, const string &def, bool doThrow) const throw(cErrArgNotFound) { // see [nr] ; if doThrow then will throw on missing var, else returns def
	if (nr <= 0) throw cErrArgIllegal("Illegal number for var, nr="+ToStr(nr)+" (1,2,3... is expected)");
	const int ix = nr - 1;
	if (ix >= mVar.size()) { // then this is an extra argument
		const int ix_ext = ix - mVar.size();
		if (ix_ext >= mVarExt.size()) { // then this var number does not exist - out of range
			if (doThrow) {
				throw cErrArgMissing("Missing argument: out of range number for var, nr="+ToStr(nr)+" ix="+ToStr(ix)+" ix_ext="+ToStr(ix_ext)+" vs size="+ToStr(mVarExt.size()));
			}
			return def; // just return the default
		}
		return mVarExt.at(ix_ext);
	}
	return mVar.at(ix);
}

vector<string> cCmdData::OptIf(const string& name) const throw(cErrArgIllegal) {
	auto find = mOption.find( name );
	if (find == mOption.end()) { 
		return vector<string>{};
	} 
	return find->second;
}

string cCmdData::VarDef(int nr, const string &def, bool doThrow) const throw(cErrArgIllegal) {
	return VarAccess(nr, def, true);
}

string cCmdData::Var(int nr) const throw(cErrArgNotFound) { // nr: 1,2,3,4 including both arg and argExt
	static string nothing;
	return VarAccess(nr, nothing, true);
}

vector<string> cCmdData::Opt(const string& name) const throw(cErrArgNotFound) {
	auto find = mOption.find( name );
	if (find == mOption.end()) { _warn("Map was: [TODO]"); throw cErrArgMissing("Option " + name + " was missing"); } 
	return find->second;
}

// ========================================================================================================================


void cmd_test( shared_ptr<cUseOT> use ) {
	_mark("TEST TREE");

	shared_ptr<cCmdParser> parser(new cCmdParser);

	auto alltest = vector<string>{ 
		"ot msg ls"
		,"ot msg ls alice"
//	,"ot msg sendfrom alice bob hello --cc eve --cc mark --bcc john --prio 4"
//	,"ot msg sendto bob hello --cc eve --cc mark --bcc john --prio 4"
//	,"ot msg rm alice 0"
//	,"ot msg-out rm alice 0"
	};
	for (auto cmd : alltest) {
		_mark("====== Testing command: " << cmd );
		parser->Init();
		auto processing = parser->StartProcessing(cmd, use);
		processing.Parse();
		processing.UseExecute();
	}

}

}; // namespace nNewcli
}; // namespace OT


