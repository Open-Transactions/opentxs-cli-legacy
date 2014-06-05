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
	_info("Add format for command name (" << (string)name << "), now size=" << mI->mTree.size() << " new format is: ");
	format->Debug();
}

void cCmdParser::Init() {
	_mark("Init tree");
	/*
	typedef function< bool ( cUseOT &, cCmdData &, int, const string &  ) > tFuncValid;
	typedef function< vector<string> ( cUseOT &, cCmdData &, int, const string &  ) > tFuncHint;
	cParamInfo(tFuncValid valid, tFuncHint hint);
	*/

	cParamInfo pNymFrom(
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Sender Nym validation");
			return use.NymCheckIfExists(data.Var(1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Sender Nym hinting");
			return use.NymGetAllNames();
		}
	);
	cParamInfo pNymTo = pNymFrom; // TODO suggest not the same nym as was used already before
	cParamInfo pNymAny = pNymFrom;

	cParamInfo pOnceInt(
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			// TODO check if is any integer
			// TODO check if not present in data
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> { "-1", "0", "1", "2", "100" };
		}
	);

	cParamInfo pSubject(
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
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
			[] ( shared_ptr<cCmdData> data, nUse::cUseOT & use ) -> cCmdExecutable::tExitCode {
				_mark("Try vardef");
				string s = data->VarDef(1,"");
				_mark("... s="<<s);

				
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
		// ot msg sendfrom alice bob subj
		// ot msg sendfrom NYM_FROM NYM_TO SUBJ
		cCmdExecutable exec(
			[] ( shared_ptr<cCmdData> data, nUse::cUseOT & use) -> cCmdExecutable::tExitCode {
				_fact("Sending (msg sendfrom) inside lambda!");
				string from = data->Var(1);
				string to = data->Var(2);
				string subj = data->VarDef(3,"");
				string prio = data->Opt1If("prio", "0");

				_note("from " << from << " to " << to << " subj=" << subj << " prio="<<prio);
				if (data->IsOpt("dryrun")) _note("Option dryrun is set");
				for(auto cc : data->OptIf("cc")) _note("--cc to " << cc);
				//use.MsgSend( data->Var(1), data->Var(2), data->VarDef(3,"no_subject") );
				return 0;
			}
		);
		cCmdFormat::tVar var;
			var.push_back( pNymFrom );
			var.push_back( pNymTo );
		cCmdFormat::tVar varExt;
			varExt.push_back( pSubject );
		cCmdFormat::tOption opt;
			opt.insert(std::make_pair("--dryrun" , pNymAny)); // TODO should be global option
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
	throw(cErrParseName)
{
	auto it = mI->mTree.find( name );
	if (it == mI->mTree.end()) {
		throw cErrParseName("No such ot command="+(string)name);
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
	int _dbg_ignore=50;

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

	_dbg3("Alloc data");  
	mData = std::make_shared<cCmdData>();

	int phase=0; // 0: cmd name  1:var, 2:varExt  3:opt   9:end
	try {

		const string name = mCommandLine.at(0) + " " + mCommandLine.at(1) ; // "msg send"
		mFormat = mParser->FindFormat( name );
		_info("Got format for name="<<name);

		// msg send
		// msg ls
		// always 2 words are the command (we assume there are no sub-command)
		const size_t words_count = mCommandLine.size();
		const cCmdFormat & format = * mFormat; // const to be sure to just read from it (we are friends of this class)
		const size_t var_size_normal = format.mVar.size(); // number of the normal (mandatory) part of variables
		const size_t var_size_all = format.mVar.size() + format.mVarExt.size(); // number of the size of all variables (normal + extra)
		_dbg2("Format: size of vars: " << var_size_normal << " normal, and all is: " << var_size_all);
		int pos = 2; // "msg send"

		phase=1;
		const size_t offset_to_var = pos; // skip this many words before we have first var, to conver pos(word number) to var number

		if (phase==1) {
			while (true) { // parse var normal
				const int var_nr = pos - offset_to_var;
				_dbg2("phase="<<phase<<" pos="<<pos<<" var_nr="<<var_nr);
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}
				if (var_nr >= var_size_normal) { _dbg1("reached end of var normal, var_nr="<<var_nr); phase=2;	break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
				++pos;

				if ( nUtils::CheckIfBegins("\"", word) ) { // TODO review memory access
					_dbg1("Quotes detected in: " + word);
					word.erase(0,1);
					while ( !nUtils::CheckIfEnds("\"", word) ) {
						word += " " + mCommandLine.at(pos);
						++pos;
					}
					word.erase(word.end(), word.end()-1); // ease the closing " of last mCommandLine[..] that is not at end of word
					_dbg1("Quoted word is:"<<word);
				}
				if (nUtils::CheckIfBegins("--", word)) { // --bcc foo
					phase=3; --pos; // this should be re-prased in proper phase
					_dbg1("Got an --option, so jumping to phase="<<phase);
					break; // continue to phase 3 - the options
				}

				_dbg1("adding var "<<word);  mData->mVar.push_back( word ); 
			}
		} // parse var phase 1

		if (phase==2) {
			while (true) { // parse var normal
				const int var_nr = pos - offset_to_var;
				_dbg2("phase="<<phase<<" pos="<<pos<<" var_nr="<<var_nr);
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}
				if (var_nr >= var_size_all) { _dbg1("reached end of var ALL, var_nr="<<var_nr); phase=3;	break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
				++pos;

				if ( nUtils::CheckIfBegins("\"", word) ) { // TODO review memory access
					_dbg1("Quotes detected in: " + word);
					word.erase(0,1);
					while ( !nUtils::CheckIfEnds("\"", word) ) {
						word += " " + mCommandLine.at(pos);
						++pos;
					}
					word.erase(word.end(), word.end()-1); // ease the closing " of last mCommandLine[..] that is not at end of word
					_dbg1("Quoted word is:"<<word);
				}
				if (nUtils::CheckIfBegins("--", word)) { // --bcc foo
					phase=3; --pos; // this should be re-prased in proper phase
					_dbg1("Got an --option, so jumping to phase="<<phase);
					break; // continue to phase 3 - the options
				}

				_dbg1("adding var ext "<<word);  mData->mVarExt.push_back( word ); 
			}
		} // phase 2

		if (phase==3) {
			string prev_name="";  bool inside_opt=false; // are we now in middle of --option ?  curr_name is the opt name like "--cc"
			while (true) { // parse options
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
				++pos;

				bool is_newopt =  nUtils::CheckIfBegins("--", word); // word is opt name like "--cc"

				if (is_newopt) { // some new option like --private or --cc
					if (inside_opt) { // finish the previos option, that didn't got a value then.  --fast [--private]
						inside_opt=false;
						mData->AddOpt(prev_name , "");
						_dbg1("got option "<<prev_name<<" (empty)");
					}
					inside_opt=true; prev_name=word; // we now started the new option (and next iteration will finish it)
					_dbg3("started new option: prev_name="<<prev_name);
				}
				else { // not an --option, so should be a value to finish previous one
					if (inside_opt) { // we are in middle of option, now we have the argment that ends it: --cc [alice]
						string value=word; // like "alice"
						inside_opt=false;
						mData->AddOpt(prev_name , value);
						_dbg1("got option "<<prev_name<<" with value="<<value);
					}
					else { // we have a word like "bob", but we are not in middle of an option - syntax error
						throw cErrParseSyntax("Expected an --option here, but got a word=" + ToStr(word) + " at pos=" + ToStr(pos));
					}
				}
			} // all words
			if (inside_opt) { // finish the previos LAST option, that didn't got a value then.  --fast [--private] (END)
				inside_opt=false;
				mData->AddOpt(prev_name , "");
				_dbg1("got option "<<prev_name<<" (empty) - on end");
			}
		} // phase 3

		_note("mVar parsed:    " + DbgVector(mData->mVar));
		_note("mVarExt parsed: " + DbgVector(mData->mVarExt));
		_note("mOption parsed  " + DbgMap(mData->mOption));
 
	} catch (cErrParse &e) {
		_warn("Command can not be parsed " << e.what());
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
	:	mExec(exec), mVar(var), mVarExt(varExt), mOption(opt)
{
	_dbg1("Created new format");
}

cCmdExecutable cCmdFormat::getExec() const {
	return mExec;
}

void cCmdFormat::Debug() const {
	_info("Format at " << (void*)this );
	_info(".. mVar size=" << mVar.size());
	_info(".. mVarExt size=" << mVarExt.size());
	_info(".. mOption size=" << mOption.size());
}

// ========================================================================================================================

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

void cCmdData::AssertLegalOptName(const string &name) const throw(cErrArgIllegal) {
	if (name.size()<1) throw cErrArgIllegal("option name can not be empty");
	const size_t maxlen=100;
	if (name.size()>maxlen) throw cErrArgIllegal("option name too long, over" + ToStr(maxlen));
	// TODO test [a-zA-Z0-9_.-]*
}

vector<string> cCmdData::OptIf(const string& name) const throw(cErrArgIllegal) {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) { 
		return vector<string>{};
	} 
	return find->second;
}

string cCmdData::Opt1If(const string& name, const string &def) const throw(cErrArgIllegal) { // same but requires the 1st element; therefore we need def argument again
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) { 
		return def;
	} 
	const auto &vec = find->second;
	if (vec.size()<1) { _warn("Not normalized opt for name="<<name); return def; }
	return vec.at(0);
}


string cCmdData::VarDef(int nr, const string &def, bool doThrow) const throw(cErrArgIllegal) {
	return VarAccess(nr, def, false);
}

string cCmdData::Var(int nr) const throw(cErrArgNotFound) { // nr: 1,2,3,4 including both arg and argExt
	static string nothing;
	return VarAccess(nr, nothing, true);
}

vector<string> cCmdData::Opt(const string& name) const throw(cErrArgNotFound) {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) { _warn("Map was: [TODO]"); throw cErrArgMissing("Option " + name + " was missing"); } 
	return find->second;
}

string cCmdData::Opt1(const string& name) const throw(cErrArgNotFound) {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) {  throw cErrArgMissing("Option " + name + " was missing"); } 
	const auto &vec = find->second;
	if (vec.size()<1) { _warn("Not normalized opt for name="<<name); throw cErrArgMissing("Option " + name + " was missing (not-normalized empty vector)"); }
	return vec.at(0);
}

bool cCmdData::IsOpt(const string &name) const throw(cErrArgIllegal) {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) { 
		return false; // no such option entry
	} 
	auto &vect = find->second;
	if (vect.size()) {
		return true; // yes, there is an option
	}

	_warn("Not normalized options for name="<<name<<" an empty vector exists there:" << DbgVector(vect));
	return false; // there was a vector for this options but it's empty now (maybe deleted?)
}

void cCmdData::AddOpt(const string &name, const string &value) throw(cErrArgIllegal) { // append an option with value (value can be empty
	_dbg3("adding option ["<<name<<"] with value="<<value);
	auto find = mOption.find( name );
	if (find == mOption.end()) {
		mOption.insert( std::make_pair( name , vector<string>{ value } ) );	
	} else {
		find->second.push_back( value );
	}	
}

// ========================================================================================================================


void cmd_test( shared_ptr<cUseOT> use ) {
	_mark("TEST TREE");

	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();

	auto alltest = vector<string>{ 
//		 "ot msg ls"
//		,"ot msg ls"
//		,"ot msg ls alice"
//		,"ot msg ls alice"
	"ot msg sendfrom alice bob --prio 1"
	, "ot msg sendfrom alice bob hello --cc eve --cc mark --bcc john --prio 4"
//	,"ot msg sendto bob hello --cc eve --cc mark --bcc john --prio 4"
//	,"ot msg rm alice 0"
//	,"ot msg-out rm alice 0"
	};
	for (auto cmd : alltest) {
		_mark("====== Testing command: " << cmd );
		auto processing = parser->StartProcessing(cmd, use);
		processing.Parse();
		processing.UseExecute();
	}

}

}; // namespace nNewcli
}; // namespace OT


