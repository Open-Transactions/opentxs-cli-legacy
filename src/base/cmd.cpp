/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "cmd.hpp"
#include "cmd_detail.hpp"

#include "lib_common2.hpp"
#include "ccolor.hpp"
#include <iomanip>

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

using namespace nUse;

// ========================================================================================================================

ostream& operator<<(ostream &stream , const cParseEntity & obj) {
	stream << "{" << obj.KindIcon() << "," << obj.mCharPos << "," << obj.mSub << "}" ;
	return stream;
}

// ========================================================================================================================


void cCmdParser_pimpl::BuildCache_CmdNames() {
	_dbg1("Caching CmdNames");
	mCache_CmdNames.clear();

	for(const auto &elem : mTree) {
		const string & cmdName = elem.first;
		auto space_pos = cmdName.find(' ');

		_dbg2("Caching cmdName="<<cmdName<<" space at: " << (long long int)space_pos);

		const string word1 = cmdName.substr(0, space_pos);
		_dbg3("word1="<<word1);
		ASRT(word1.length());


		if (space_pos == string::npos) { // 1word command
			mCache_CmdNames[ word1 ].insert(""); // add
		} else { // 2word command
			const string word2 = cmdName.substr(space_pos+1);
			_dbg3("word2="<<word2);
			mCache_CmdNames[ word1 ].insert( word2 );
		}
	}

	for (const auto & elem : mCache_CmdNames) {
		const string & word1 = elem.first; // "msg"
		mCache_CmdNamesVect1.push_back(word1); // write down possible word1
		_dbg2("word1: " << word1);
		for (const auto & elem2 : elem.second) {
			const string & word2 = elem2; // "msg send"
			_dbg3("word2 in " <<word1<< " is: " << word2);
			mCache_CmdNamesVect2[word1].push_back(word2);
		}
	}

}

// ------------------------------------------------------------------------------------------------------------------------

// *** cCmdParser ***

const vector<string> cCmdParser::mNoWords;

cCmdParser::cCmdParser()
: mI( new cCmdParser_pimpl )
{ }

cCmdParser::~cCmdParser() { } // let's instantize default destr in all TUs so compilation will fail (without this line) on unique_ptr on not-complete types trololo - B. Stroustrup

cCmdExecutable::tExitCode Execute1( shared_ptr<cCmdData> , nUse::cUseOT ) {
	_mark("***Execute1***");
	return cCmdExecutable::sSuccess;
}

void cCmdParser::PrintUsage() const {
	auto & out = cerr;
	out << endl;
	using namespace zkr;
	for(auto element : mI->mTree) {
		string name = element.first;
		shared_ptr<cCmdFormat> format = element.second;
		out << cc::fore::console << "  ot " << cc::fore::green << name << cc::fore::console	<< " " ;
		format->PrintUsageShort(out);
		out << endl;
	}
	out << cc::console;
	out << endl;
}

void cCmdParser::PrintUsageCommand(const string &cmdname) const {
/*
 //	mI->mTree.at(cmdname); // find
auto & out = cerr;
 try {
 auto format  = ....
		format->PrintUsageShort(out);
		format->PrintUsageLong(out); // ***
 } ...
TODO
*/
	using namespace zkr;
	auto const & name = cmdname;
	auto & out = cerr;
	out << endl;
	try {
		const cCmdFormat & format = * FindFormat(cmdname);
		out << cc::fore::console << "  ot " << cc::fore::green << name << cc::fore::console	<< " " ;
		format.PrintUsageShort(out);
		out << endl << endl;
		out << cc::fore::lightgreen << cmdname << cc::fore::console << " - is name of the command" << endl;
		format.PrintUsageLong(out);

	} catch (const cErrParseName &e) {
		out << "There is no command name: " << cmdname << endl;
	}

	out << endl;
}

cCmdProcessing cCmdParser::StartProcessing(const string &words, shared_ptr<nUse::cUseOT> use ) {
	return cCmdProcessing( shared_from_this() , words , use );
}

shared_ptr<cCmdFormat> cCmdParser::FindFormat(const cCmdName &name) const
{
	auto it = mI->mTree.find( name );
	if (it == mI->mTree.end()) {
		throw cErrParseName("No such ot command="+(string)name);
	}

	return it->second;
}

bool cCmdParser::FindFormatExists( const cCmdName &name ) const
{
	try {
		FindFormat(name);
		return true;
	} catch(cErrParseName &e) { }
	return false;
}

const vector<string> & cCmdParser::GetCmdNamesWord1() const { // possible word1 in loaded command names
	return mI->mCache_CmdNamesVect1;
}

const vector<string> & cCmdParser::GetCmdNamesWord2(const string &word1) const { // possible word2 for given word1 in loaded command names
	auto found = mI->mCache_CmdNamesVect2.find(word1);
	if (found != mI->mCache_CmdNamesVect2.end()) {
		return found->second;
	} else return mNoWords;
}

// ========================================================================================================================

cCmdName::cCmdName(const string &name) : mName(name) { }

bool cCmdName::operator<(const cCmdName &other) const { return mName < other.mName; }

cCmdName::operator std::string() const { return mName; }

// ========================================================================================================================

cCmdProcessing::cCmdProcessing(shared_ptr<cCmdParser> parser, const string &commandLineString, shared_ptr<nUse::cUseOT> use )
:
mStateParse(tState::never), mStateValidate(tState::never), mStateExecute(tState::never),
mFailedAfterBadCmdname(false),
mParser(parser), mCommandLineString(commandLineString), mUse(use)
{
	mCommandLine = vector<string>{}; // will be set in Parse()
	_dbg2("Creating processing from (" << mCommandLineString <<") " << " with use=" << use->DbgName() );
}

cCmdProcessing::~cCmdProcessing()
{ }

void cCmdProcessing::Validate() {
	if (mStateValidate != tState::never) { _dbg1("Validation was done already"); return; }
	mStateValidate = tState::failed; // assumed untill succeed below
	try {
		_Validate();
		mStateValidate = tState::succeeded;
		_dbg1("Validation succeeded");
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cCmdProcessing::_Validate() {
	if (mStateParse != tState::succeeded) Parse();
	if (mStateParse != tState::succeeded) { _dbg1("Failed to parse."); }

	if (!mData) { _warn("Can not validate - there is no mData"); return; }
	if (!mFormat) { _warn("Can not validate - there is no mData"); return; }

	const auto sizeAll = mData->SizeAllVar();
	_dbg2("Will validate all variables, size=" << sizeAll );

	for (size_t nr=1; nr<=sizeAll; ++nr) { // TODO:nrix
		auto var = mData->Var(nr); // get the var
		const cParamInfo & info = mFormat->GetParamInfo(nr);
		auto func = info.GetFuncValid();
		bool ok = func( *mUse, *mData, nr-1 ); // ***
		if (!ok) {
			const string err = ToStr("Validation failed at nr=") + ToStr(nr) + " for var=" + ToStr(var);
			_warn(err);
			throw myexception(err);
		}
	}
}

void cCmdProcessing::Parse(bool allowBadCmdname) {
	_dbg1("Entering Parse()");
	if (mStateParse != tState::never) { _dbg1("Already parsed"); return; }
	mStateParse = tState::failed; // assumed untill succeed below
	try {
		_Parse(allowBadCmdname);
		if (mFailedAfterBadCmdname) { _info("Parsed partially"); mStateParse = tState::succeeded_partial; }
		else { _info("Parsed ok (fully ok)"); mStateParse = tState::succeeded; }
	}
	catch (const myexception &e) { e.Report(); throw ; }
	catch (const std::exception &e) { _info("Exception " << e.what()); throw ; }
}

void cCmdProcessing::_Parse(bool allowBadCmdname) {
	// int _dbg_ignore=50;
	bool dbg=1;
	bool test_char2word = false; // run a detailed test on char to word conversion

	mData = std::make_shared<cCmdDataParse>();
	mData->mOrginalCommand = mCommandLineString;

	// will be used to calculate offsets (e.g. words) between orginal string suppied (eg from user) and the upgraded string we store later (after addig pre "ot" etc)
	int namepart_words = 0; // how many words are in name SINCE BEGINING (including ot), "ot msg send"=3, "ot help"=2", "ot"=1
	int prepart_words = 0; // simillary, how many words are the begining, usually there is 1 (for "ot")

	if (mCommandLineString.empty()) { const string s="Command for processing was empty (string)"; _warn(s);  throw cErrParseSyntax(s); } // <--- THROW

	{ // initial parsing

		mCommandLine.clear();

		// [doc] praser documentation

		// char processing (remove double-space, parse quotations etc)
		// TODO: quotation "..."
		//            |  help
		//            |ot   help
		//     string |ot  msg  ls  bob  --all  --color red  --reload  --color blue
		// char_pos   |012345678901234567890123456789012345678901234567890123456789
		// word_ix    |0   1    2   3    4      5       6     7        8       9
		// any_arg    |             1    2      3            4         5        6
		// char2ent   |p,0 n,0 n,1  v,1  on,w=4 on,w=5 ov,w=6 on,w=7    on,w=8  ov,w=9  see details in cParseEntity;
		// cmd name        "msg ls"
		// Opt("--color")                              red,                    blue
		// Opt("--reload")                                    ""
		// Arg(1)                   bob
		// vector     =ot,msg,ls
		// mWordIx2CharIx [0]=2, [1]=6, [2]=11 (or so)
		string curr_word="";
		size_t curr_word_pos=0; // at which pos(at which char) that current word had started
		for (size_t pos=0; pos<mCommandLineString.size(); ++pos) { // each character
			char c = mCommandLineString.at(pos);
			if (c==' ') { // white char
				if (curr_word.size()) {  // if there was a previous word
					mCommandLine.push_back(curr_word);
					mData->mWordIx2Entity.push_back( cParseEntity( cParseEntity::tKind::unknown, curr_word_pos) );
					curr_word="";
				}
			}
			else { // normal char
				if (curr_word.size()==0) curr_word_pos = pos; // this is the first character of current (new) word
				curr_word += c;
			}
		} // for each char

		if (curr_word.size()) {
			mCommandLine.push_back(curr_word);
			mData->mWordIx2Entity.push_back( cParseEntity( cParseEntity::tKind::unknown, curr_word_pos)  );
		}
		_dbg1("Vector of words: " << DbgVector(mCommandLine));
		if(dbg) _dbg2("Words position mWordIx2Entity=" << DbgVector(mData->mWordIx2Entity));
	}

	if (test_char2word) { for (int i=0; i<mCommandLineString.size(); ++i) {
		const char c = mCommandLineString.at(i);
		_dbg3("char '" << c << "' on position " << i << " is inside word: " << mData->CharIx2WordIx(i) );
	} }

	if (mCommandLine.empty()) { const string s="Command for processing was empty (had no words)"; _info(s);  throw cErrParseSyntax(s); } // <--- THROW

	// -----------------------------------
	if (mCommandLine.at(0) == "help") { mData->mCharShift=-3; namepart_words--; prepart_words--;  mCommandLine.insert( mCommandLine.begin() , "ot"); } // change "help" to "ot help"
	// ^--- namepart_words-- because we here inject the word "ot" and it will make word-position calculation off by one

	if (mCommandLine.at(0) != "ot") {
		_info("Command for processing is mallformed");
		const string s="Missing pre word: ot";  _info(s);  throw cErrParseSyntax(s);
	}
	mCommandLine.erase( mCommandLine.begin() ); // delete the first "ot" ***
	mData->mIsPreErased=true;
	// mCommandLine = msg, send-from, alice, bob, hello
	_dbg1("Parsing (after erasing ot) : " << DbgVector(mCommandLine) );

	if (mCommandLineString.empty()) { const string s="Command for processing was empty (besides prefix)"; _info(s);  throw cErrParseSyntax(s); } // <--- THROW

	prepart_words++;
	mData->mFirstWord = prepart_words; // usually 1, meaning that there is 1 word between actuall entities, e.g. when we remove the "ot" pre

	if(dbg) _dbg3("Shift: mCharShift=" << mData->mCharShift << " mFirstWord="<<mData->mFirstWord );

	int phase=0; // 0: cmd name  1:var, 2:varExt  3:opt   9:end
	try {
		if (mCommandLine.size()==0) { const string s="No words (besides pre ot)";  _info(s);  throw cErrParseSyntax(s); }

		// phase0 - the command name
		string name_tmp = mCommandLine.at(0); // build the name of command, start with 1st word like "msg" or "help"
		if(mCommandLine.size()>1) {	// if NOT one-word command like "help", then:
			string name_tmp2 = name_tmp+" " + mCommandLine.at(1);
			if (	mParser->FindFormatExists(name_tmp) && mParser->FindFormatExists(name_tmp2)) { // if the second word is CmdName
				if (mCommandLine.size()>1) {  // take 2nd word as part of the name
					namepart_words++;
					name_tmp += " " + mCommandLine.at(1);
				}
			}
			else if ( mParser->FindFormatExists(name_tmp2)) { // if the second word is CmdName
				if (mCommandLine.size()>1) {  // take 2nd word as part of the name
					namepart_words++;
					name_tmp += " " + mCommandLine.at(1);
				}
			}
			else if (! mParser->FindFormatExists(name_tmp) ) { // like "msg" - this can not be a 1word command
				if (mCommandLine.size()>1) {  // take 2nd word as part of the name
					namepart_words++;
					name_tmp += " " + mCommandLine.at(1);
				}
			}
		} // more then 1 word

		const string name = name_tmp;
		namepart_words++;

		// "msg send" or "help"
		_dbg1("Name of command is: " << name << " namepart_words="<<namepart_words);
		mData->mFirstArgAfterWord = namepart_words;

		mData->mWordIx2Entity.at(0).SetKind( cParseEntity::tKind::pre ); // "ot" token

		for (int i=1; i<=namepart_words; ++i) mData->mWordIx2Entity.at(i).SetKind( cParseEntity::tKind::cmdname , i ); // mark this words as part of cmdname
		if(dbg) _dbg2("Words position mWordIx2Entity=" << DbgVector(mData->mWordIx2Entity));

		try {
			mFormat = mParser->FindFormat( name ); // <---
		}
		catch(cErrParseName &e) {
			if (allowBadCmdname) {
				mFailedAfterBadCmdname=true;  return;  // <=== RETURN.  exit, but report that we given up early  <===========================
			}
			else { throw ; } // else just panic - throw // <======
		}
		_info("Got format for name="<<name);

		// msg send
		// msg ls
		// always 2 words are the command (we assume there are no sub-command)
		const size_t words_count = mCommandLine.size();
		const cCmdFormat & format = * mFormat; // const to be sure to just read from it (we are friends of this class)
		const size_t var_size_normal = format.mVar.size(); // number of the normal (mandatory) part of variables
		const size_t var_size_all = format.mVar.size() + format.mVarExt.size(); // number of the size of all variables (normal + extra)
		_dbg2("Format: size of vars: " << var_size_normal << " normal, and all is: " << var_size_all);

		int pos = 2; // number of currently parsed word "msg send"
		if (name.find(" ") == string::npos) pos=1; // in case of 1-word name

		phase=1;
		const size_t offset_to_var = pos; // skip this many words before we have first var, to conver pos(word number) to var number
		size_t quotes_offset_to_var = 0;
		if (phase==1) { // phase: parse variable
			while (true) { // parse var normal
				const int var_nr = pos - offset_to_var - quotes_offset_to_var;
				_dbg2("phase="<<phase<<" pos="<<pos<<" var_nr="<<var_nr);
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}
				if (var_nr >= var_size_normal) { _dbg1("reached end of var normal, var_nr="<<var_nr); phase=2;	break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
				++pos;
				mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::variable , var_nr );

				if ( nUtils::CheckIfBegins("\"", word) ) { // TODO review memory access
					_dbg1("Quotes detected in: " + word);
					word.erase(0,1);
					while ( !nUtils::CheckIfEnds("\"", word) ) {
						word += " " + mCommandLine.at(pos);
						++pos;
						++quotes_offset_to_var;
					}
					word.erase(word.end()-1, word.end()); // ease the closing " of last mCommandLine[..] that is not at end of word
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

		_mark("Words position mWordIx2Entity=" << DbgVector(mData->mWordIx2Entity));

		if (phase==2) {
			while (true) { // parse var extra
				const int var_nr = pos - offset_to_var - quotes_offset_to_var;
				_dbg2("phase="<<phase<<" pos="<<pos<<" var_nr="<<var_nr);
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}
				if (var_nr >= var_size_all) { _dbg1("reached end of var ALL, var_nr="<<var_nr); phase=3;	break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
				++pos;
				mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::variable_ext , var_nr );

				if ( nUtils::CheckIfBegins("\"", word) ) { // TODO review memory access
					_dbg1("Quotes detected in: " + word);
					word.erase(0,1);
					while ( !nUtils::CheckIfEnds("\"", word) ) {
						word += " " + mCommandLine.at(pos);
						++pos;
						++quotes_offset_to_var;
					}
					word.erase(word.end()-1, word.end()); // ease the closing " of last mCommandLine[..] that is not at end of word
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
			int token_nr=0;
			while (true) { // parse options
				++token_nr;
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word<<" (token_nr="<<token_nr<<")");
				++pos;

				bool is_newopt =  nUtils::CheckIfBegins("--", word); // word is opt name like "--cc"

				if (is_newopt) { // some new option like --private or --cc
					if (inside_opt) { // finish the previos option, that didn't got a value then.  --fast [--private]
						inside_opt=false;
						mData->AddOpt(prev_name , ""); // ***
						mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::option_name ); // TODO sub number!
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
						mData->mWordIx2Entity.at( pos-1 ).SetKind( cParseEntity::tKind::option_name ); // e.g. "--cc" // TODO sub number!
						mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::option_value ); // e.g. "alice" // TODO sub number!
						_dbg1("got option "<<prev_name<<" with value="<<value);
					}
					else { // we have a word like "bob", but we are not in middle of an option - syntax error
						// this is the special case of "ot nym inf" the word "inf" might be not a wrong use of --option, but 2nd word of command name being entered
						_dbg3("TOLERATE? token_nr"<<token_nr<<" namepart_words="<<namepart_words<<" allowBadCmdname="<<allowBadCmdname);
						if ((token_nr==1) && (namepart_words==1) && (allowBadCmdname)) {
							_dbg2("Tollerating this option-or-nameword2 here at pos="<<pos);
							mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::cmdname , 2 ); // assume this is 2nd word of command
						} else {
							throw cErrParseSyntax("Expected an --option here, but got a word=" + ToStr(word) + " at pos=" + ToStr(pos));
						}
					}
				}
			} // all words
			if (inside_opt) { // finish the previos LAST option, that didn't got a value then.  --fast [--private] (END)
				inside_opt=false;
				mData->AddOpt(prev_name , "");
				mData->mWordIx2Entity.at( pos ).SetKind( cParseEntity::tKind::option_name ); // TODO sub number!
				_dbg1("got option "<<prev_name<<" (empty) - on end");
			}
		} // phase 3

		_note("Entities:" << DbgVector(mData->mWordIx2Entity));
		_note("mVar parsed:    " + DbgVector(mData->mVar));
		_note("mVarExt parsed: " + DbgVector(mData->mVarExt));
		_note("mOption parsed  " + DbgMap(mData->mOption));
	}
	catch (cErrParse &e) {
		_info("Command can not be parsed " << e.what());
		throw ;
	}
	catch (std::exception &e) {
		_erro("Internal error in parser code " << e.what() << " while parsing:" << DbgVector( mCommandLine ) );
		throw ;
	}
	catch (myexception &e) {
		_erro("Internal error in parser code " << e.what() << " while parsing:" << DbgVector( mCommandLine ) );
		throw ;
	}
}


vector<string> cCmdProcessing::UseComplete(int char_pos) {
	_mark("Will complete command line: ["<<mCommandLineString<<"] at char_pos="<<char_pos);  // mCommandLine is not parsed yet
	const vector<string> allowed_pre_words = {"ot","help"};

	mCommandLineString = nUtils::SpecialFromEscape(mCommandLineString, char_pos); // change all escaped space characters to special character for easier parsing. Need to pass char_pos, because of change in string length

	if (mStateParse == tState::never) {
		bool ok=0;
		try {
			Parse( true );
			ok=1;
		} catch (cErrParseSyntax &e) { ok=0; }

		_mark("Will complete command line: ["<<mCommandLineString<<"] " << " words " << DbgVector(mCommandLine) << " at char_pos="<<char_pos);
		if ( (mCommandLine.size()==0) && (mCommandLineString.size()>0) ) {
			_note("Correcting the no-words case");
			mCommandLine.push_back( mCommandLineString ); // cut by space?
		}

		if (!ok) { // first parse failed maybe because we wanted to complete something here like ot nym sh~ alice and parsed assumed it's "nym" + arg "sh",
			mCommandLineString = mCommandLineString.substr(0, char_pos );
			_note("Parsing failed, will parse again as ["<<mCommandLineString<<"]");
			// instead assuming it is half-written command name "nym sh~".
			// so we will re-parse just the part before char position
			mStateParse = tState::never; // to force re-parsnig again
			try {
				Parse( true );
				ok=1;
			} catch (cErrParseSyntax &e) { ok=0; }
			_mark("After PARSING AGAIN: Will complete command line: ["<<mCommandLineString<<"] " << " words " << DbgVector(mCommandLine) << " at char_pos="<<char_pos);
		}
		if (!ok) mFormat=nullptr; // we do not have any realiable format/cmdname if even very lax parsing failed
	} // parse
	if (mStateParse != tState::succeeded) {
		if (mStateParse == tState::succeeded_partial) _dbg3("Failed to fully parse.");  // can be ok - maybe we want to comlete cmd name like "ot msg sendfr~"
		else _dbg1("Failed to parse (even partially)");
	}
	ASRT(nullptr != mData);

	vector<string> allwords; // vector of all words including pre "ot" (before removing that pre, so we can complete first word)
	if (mData->mIsPreErased) {
		_mark("Restoring pre, for the reason to complete the pre-word");
		size_t pos = mCommandLineString.find(' ');
		if (pos == string::npos) pos = mCommandLineString.size(); // there are no words with space e.g. "ot" or "o" or "hel"
		if (pos != string::npos) allwords.push_back( mCommandLineString.substr(0, pos ) );
	}
	_mark("Allwords1=" << DbgVector(allwords));
	for (const auto & elem : mCommandLine) allwords.push_back( elem );
	_mark("Allwords2=" << DbgVector(allwords));

	if (allwords.size()==0) return allowed_pre_words;

	vector<string> mCommandLine = this->mCommandLine ; // XXX we are on purpose shadowing this->mCommandLine.  TODO not needed we aren't overwritting it ater all here?

	using namespace nOper;

/*	_info("check0.  size=" << mCommandLine.size());
	if (mCommandLine.size()>0) {
		if (mCommandLine.at(0) != "ot") {
			_dbg1("not ot!");
			if (mCommandLine.at(0)=="help") return vector<string>{""};
			return WordsThatMatch(mCommandLine.at(0), vector<string>{"ot", "help"});
		}
	} else {
		_dbg1("empty cmd line");
		return vector<string>{ "ot" } ;
	}

	_info("check1.  size=" << mCommandLine.size());
*/

	try {
		int word_ix = mData->CharIx2WordIx( char_pos  );
		bool fake_empty=false; // are we adding fake word "" at end for purpose of completion at end of string?
		_mark("mCommandLineString: " << mCommandLineString << "char_pos: " << char_pos);
		if (mCommandLineString.at(char_pos-1) == ' ') {
			_mark("tryblock2");
			word_ix++; // jump to next word XXX
			if (word_ix >= mCommandLine.size()) { fake_empty=true;
				int add_empty_at_word = mData->CharIx2WordIx(char_pos-1); // choose the word-index after which we should insert the "" new word (if we're completing in middle of string)
				_mark("will add in add_empty_at_word=" << add_empty_at_word);
				mCommandLine.insert( mCommandLine.begin() + add_empty_at_word , "");
				_dbg2("mCommandLine="<<DbgVector(mCommandLine));
			} // fake word
		}

		if (allwords.size()<1) {
			_mark("size<1");
			if (!fake_empty) return WordsThatMatch("", allowed_pre_words);
		}
		if (allwords.size()==1) {
			_mark("size==1");
			_mark("tryblock3");
			if (!fake_empty) return WordsThatMatch(allwords.at(0), allowed_pre_words);
			_mark("tryblock4");
		}

		if (allwords.at(0)=="help") return {}; // there is nothing to complete after "help" alias

		//_dbg1("word_ix=" << word_ix);
		int arg_nr = mData->WordIx2ArgNr( word_ix );

		_dbg1("mCommandLine=" << DbgVector(mCommandLine));
		string word_sofar = mCommandLine.at(word_ix - mData->mFirstWord);  // the current word that we need to complete. e.g. "--dryr" (and we will complete "--dryrun")
		_dbg3(word_sofar);
		long int word_previous_ixtab = word_ix - mData->mFirstWord - 1;
		const string word_previous = (word_previous_ixtab>=0) ? mCommandLine.at(word_previous_ixtab) : "";
		//_dbg1("word_sofar="<<word_sofar<<", and previous word="<<word_previous<<" char_pos="<<char_pos);

		cParseEntity entity = (!fake_empty) ? mData->mWordIx2Entity.at(word_ix) : cParseEntity( cParseEntity::tKind::fake_empty , char_pos );
		_dbg3("entity="<<entity);
		char sofar_last_char = mCommandLineString.at(char_pos-1); // the character after which we are now completing e.g. "g" for "msg~" or " " for "msg ~"
		const bool after_word = sofar_last_char==' ' ; // are we now after (e.g. 1st) word, e.g. because we stand on space like in  "ot msg ~"  (instead "ot msg~")
		_note("Completion at pos="<<char_pos<<" word_ix="<<word_ix<<" arg_nr="<<arg_nr<<" entity="<<entity
			<<" word_sofar=["<<word_sofar<<"] sofar_last_char=["<<sofar_last_char<<"] word_previous="<<word_previous<<" after_word="<<after_word);

		vector<string> matching; // <--- the completions what seem to fit

		// handle empty word (the Parser did not told us what is/would be the type of token that is now beginning:
		if (entity.mKind == cParseEntity::tKind::fake_empty) { // we are finishg an empty word - we are now creating a new word
			// *** DUAL: handling DUAL choice, where user might be starting now an option or something else ***
			// try to make an --option out of the DUAL

			cParseEntity entity_previous = mData->mWordIx2Entity.at(word_ix-1);
			_fact("checking DUAL vs entity_previous="<<entity_previous<<" at mFormat:" << ( (mFormat!=nullptr) ? "yes":"null") );
			if ((entity_previous.mKind == cParseEntity::tKind::cmdname) && (entity_previous.mSub==1) && (mFormat!=nullptr)) {
				// "msg ~" (or maybe... "msg -~" or "msg ad~") and "msg" is a valid command, so we add all options here, like "msg --dryrun"
				// yes obviously "msg a~" will not be an option but this is ok e.g. code will add 0 options here and continue
				_mark(" OPTIONS NAMES: " << DbgVector(mFormat->GetPossibleOptionNames()));
				matching += WordsThatMatch( word_sofar ,  mFormat->GetPossibleOptionNames() );
			}

			// now finish the normal (not-options) part of DUAL:

			if (entity_previous.mKind == cParseEntity::tKind::pre) { // we are adding new word after pre: so it will be command name
				entity = cParseEntity(cParseEntity::tKind::cmdname, char_pos, 0);
			} else
			if (entity_previous.mKind == cParseEntity::tKind::cmdname) { // after command name: 2nd word of command name or jump further to e.g. var/varExt/opt
				// nym --> cmd (has 1-word variant)   [we assumed there will never be "nym arg1 ..." no args after 1-word cmd]
				// msg     (has no 1-word variant)  --> cmd
				// nym ls --> arg
				// msg ls --> arg

				if (entity_previous.mSub==1) { // "nym" "msg"
					if (mFormat!=nullptr) { // "nym"
						entity = cParseEntity(cParseEntity::tKind::cmdname, char_pos, 2); // now continue to parse this empty as command, word 2
					} else { // "msg"
						entity = cParseEntity(cParseEntity::tKind::cmdname, char_pos, 2); // now continue to parse this empty as command, word 2
					}
				}
				else if (entity_previous.mSub==2) { // "nym ls" "msg ls"
					entity = cParseEntity(cParseEntity::tKind::argument_somekind, char_pos, 1);
				}
			} // after a command
			else if (entity_previous.mKind == cParseEntity::tKind::option_name) {
				entity = cParseEntity(cParseEntity::tKind::option_value, char_pos,  0); // after option_name goes option_value
			}
			else {
				entity = cParseEntity(cParseEntity::tKind::argument_somekind, char_pos,  0); // else var/varExt/opt
			}
		} // fake empty

		if (entity.mKind == cParseEntity::tKind::argument_somekind) {  _dbg1("Finding out which kind of argument is here, from entity="<<entity);
			if (!mFormat) { // we do not yet have a format even
				if (allwords.size()<=1) entity.SetKind(cParseEntity::tKind::cmdname); // in commad name... is 0 correct?
				if (allwords.size()<=2) entity.SetKind(cParseEntity::tKind::cmdname); // in commad name... is 1 correct?
			}
			else {
				ASRT(mFormat);
				if (arg_nr <= mFormat->mVar.size()) { entity.SetKind(cParseEntity::tKind::variable); }
				else if (arg_nr <= mFormat->SizeAllVar()) { entity.SetKind(cParseEntity::tKind::variable_ext); }
				else { entity.SetKind(cParseEntity::tKind::option_name); }
			}
		}
		_fact("matching after DUAL: " << DbgVector(matching) << " and now entity="<<entity );

		if (entity.mKind == cParseEntity::tKind::option_name) {
			shared_ptr<cCmdFormat> format = mFormat;  // info about command "msg sendfrom"
			if (!format) return vector<string>{}; // if we did not understood command name, then return empty vector
			matching += WordsThatMatch( word_sofar ,  format->GetPossibleOptionNames() );
			return matching;
		}
		else if (entity.mKind == cParseEntity::tKind::option_value) {
			string option_name = mCommandLine.at(word_ix - mData->mFirstWord  -1); // the corresponding name of option like "--cc"
			shared_ptr<cCmdFormat> format = mFormat;  // info about command "msg sendfrom"
			if (!format) return vector<string>{}; // if we did not understood command name, then return empty vector
			try {
				const cParamInfo &info = format->mOption.at(option_name);
				auto funcHint = info.GetFuncHint();   // typedef function< bool ( nUse::cUseOT &, cCmdData &, size_t ) > tFuncValid;
				auto hint = (funcHint)( *mUse , *mData  , word_ix );
				matching += WordsThatMatch( word_sofar , hint);
				// TODO check if the word_ix here is correct
				return matching;
			} catch(std::exception &e) { } // something failed probaly the option name was not known
			return vector<string>{}; // the name of option seems unknown, so we can not offer any completion for the value for that option
		}
		else if (entity.mKind == cParseEntity::tKind::variable) { _info("Completing variable as arg_nr="<<arg_nr);
			ASRT( mFormat );
			//if (!fake_empty) ASRT( mData->V(arg_nr) == word_sofar ); // the current work == current arg. (unless this is new word) VYRLY - dont wan't it because there can be more words than args
			cParamInfo param_info = mFormat->GetParamInfo( arg_nr );  // eg. pNymFrom  <--- info about kind (completion function etc) of argument that we now are tab-completing
			auto completions = param_info.GetFuncHint()  ( *mUse , *mData , arg_nr );
			_info("Var completions: " << DbgVector(completions));
			return matching + WordsThatMatch( word_sofar ,  completions );
		}
		else if (entity.mKind == cParseEntity::tKind::variable_ext) { _info("Completing variable_ext as arg_nr="<<arg_nr);
			ASRT( mFormat );
			if (!fake_empty) ASRT( mData->v(arg_nr) == word_sofar ); // the current work == current arg. (unless this is new word)
			cParamInfo param_info = mFormat->GetParamInfo( arg_nr );  // eg. pNymFrom  <--- info about kind (completion function etc) of argument that we now are tab-completing
			auto completions = param_info.GetFuncHint()  ( *mUse , *mData , arg_nr );
			return matching + WordsThatMatch( word_sofar ,  completions );
		}
		else if (entity.mKind == cParseEntity::tKind::cmdname) {
			const int cmd_word_nr = entity.mSub;
			_info("Completing command name cmd_word_nr="<<cmd_word_nr<<" after_word="<<after_word<<" word_sofar="<<word_sofar);
			if ( (cmd_word_nr==0) ) { // "~" like from "ot ~"
				matching += WordsThatMatch( "" , mParser->GetCmdNamesWord1() );
				return matching; // <---
			}	else if ( (cmd_word_nr==1) && (!after_word) ) { // "ms~" or "msg~"
				matching += WordsThatMatch( word_sofar , mParser->GetCmdNamesWord1() );
				return matching; // <---
			} else if ( (cmd_word_nr==1) && (after_word) )  { // "msg ~"
				matching += WordsThatMatch( "" , mParser->GetCmdNamesWord2( word_sofar ) );
				return matching; // <---
			} else if ( (cmd_word_nr==2))  { // "msg se~"
				auto match2 = mParser->GetCmdNamesWord2( word_previous ); // skip ignore for the "" word2 of command (so to not add it e.g. to options --dryrun from DUAL condition)
				match2.erase( std::remove_if( match2.begin() , match2.end(),   [](const string &v){ return (v=="") ; }    )  ,  match2.end() );
				_dbg2("match2="<<DbgVector(match2));
				matching += WordsThatMatch( word_sofar , match2 );
				return matching; // <---
			} else throw cErrInternalParse("Bad cmd_word_nr="+ToStr(cmd_word_nr)+", after_word="+ToStr(after_word)+" in completion");
			return vector<string>{};
		}
		else if (entity.mKind == cParseEntity::tKind::pre) {
			return matching + vector<string>{"ot"};
		}
		else if (entity.mKind == cParseEntity::tKind::fake_empty) {
			_warn("Didnt knew how to complete (further) this fake_empty entity="<<entity<<" (set it to proper type after the DUAL code)");
			return matching; // TODO
		}
		else {
			_erro("Unimplemented entity type in completion");
			return vector<string>{};
		}

		_erro("DEAD CODE");
		vector<string> ret;
		return ret;
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }

	_erro("Dead code hit?");	return vector<string>(); // should not happen
}

void cCmdProcessing::UseExecute() { // TODO write as a template for all the 3 wrappres that set state ??
	if (mStateParse != tState::succeeded) Parse();
	if (mStateValidate != tState::succeeded) Validate();
	if (mStateParse != tState::succeeded) { _dbg1("Failed to parse."); }
	if (mStateValidate != tState::succeeded) { _dbg1("Failed to validate."); }

	if (mStateExecute != tState::never) { _dbg1("Exec was done already"); return; }
	mStateExecute = tState::failed; // assumed untill succeed below
	try {
		_UseExecute();
		mStateExecute = tState::succeeded;
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cCmdProcessing::_UseExecute() {
	if (!mFormat) { _warn("Can not execute this command - mFormat is empty"); return; }
	cCmdExecutable exec = mFormat->getExec();
	exec( mData , *mUse );
}

// ========================================================================================================================

void cValidateError::Print() const {
	//cout <<

}

// ========================================================================================================================

cParamInfo::cParamInfo(const string &name, tFuncDescr descr, tFuncValid valid, tFuncHint hint, tFlags mFlags)
	: mName(name), funcDescr(descr), funcValid(valid), funcHint(hint), mFlags(mFlags)
{ }

cParamInfo::cParamInfo(const string &name, tFuncDescr descr)
	: mName(name), funcDescr(descr)
{ }

cParamInfo cParamInfo::operator<<(const cParamInfo &B) const {
	cParamInfo A = *this;
	A.mName = B.mName;
	A.funcDescr = B.funcDescr;
	if (B.funcValid) A.funcValid = B.funcValid;
	if (B.funcHint) A.funcHint = B.funcHint;
	return A;
}

bool cParamInfo::IsValid() const {
	if (mName.length()<1) { _warn("Invalid cParamInfo with empty name!"); return false; }
	return true;
}

// ========================================================================================================================

// cCmdFormat::cCmdFormat(cCmdExecutable exec, tVar var, tVar varExt, tOption opt)
cCmdFormat::cCmdFormat(const cCmdExecutable &exec, const tVar &var, const tVar &varExt, const tOption &opt)
	:	mExec(exec), mVar(var), mVarExt(varExt), mOption(opt)
{
	_dbg1("Created new format");
}


bool cCmdFormat::IsValid() const {
	bool allok=true;
	for(const auto & elem : mVar) if (!elem.IsValid()) allok=false;
	for(const auto & elem : mVarExt) if (!elem.IsValid()) allok=false;
	for(const auto & elem : mOption) if (!elem.second.IsValid()) allok=false;
	return allok;
}

vector<string> cCmdFormat::GetPossibleOptionNames() const {
	vector<string> ret;
	for(auto elem : mOption) {
		ret.push_back( elem.first ); // add eg "--cc"
	}
	return ret;
}

cParamInfo cCmdFormat::GetParamInfo(int nr) const {
	// similar to cCmdData::VarAccess()
	if (nr <= 0) throw myexception("Illegal number for var, nr="+ToStr(nr));
	const int ix = nr - 1;
	if (ix >= mVar.size()) { // then this is an extra argument
		const int ix_ext = ix - mVar.size();
		if (ix_ext >= mVarExt.size()) { // then this var number does not exist - out of range
			throw myexception("Missing argument: out of range number for var, nr="+ToStr(nr)+" ix="+ToStr(ix)+" ix_ext="+ToStr(ix_ext)+" vs size="+ToStr(mVarExt.size()));
		}
		return mVarExt.at(ix_ext);
	}
	return mVar.at(ix);
}

size_t cCmdFormat::SizeAllVar() const { // return size of required mVar + optional mVarExt
	return mVar.size() + mVarExt.size();
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

void cCmdFormat::PrintUsageLong(ostream &out) const {
	using namespace zkr;
	bool written=false;

	const int width1 = 20;

	int varnr=0;
	for (int tabnr=0; tabnr<=1; ++tabnr)
	{
		const auto & tab = ( (tabnr==0) ? mVar : mVarExt );
		for(auto var : tab) {
			++varnr;
			out << ( (tabnr==0) ? cc::fore::lightyellow : cc::fore::lightcyan )  <<  std::setw(width1) << (string)var ;
			out << cc::fore::console << " - " ;
			out << var.getDescr() << " - ";
			out << ( (tabnr==0) ? "required variable" : "extra variable" ) << " number #" << varnr;
			out << endl;
		}
	}

	for (int sort=0; sort<=1; ++sort) {
		size_t nr=0;
		for(auto opt : mOption) {
			const string &name = opt.first;
			const cParamInfo &info = opt.second;
			bool boring = (info.getFlags().n.isBoring);
			auto color1 = boring  ?  cc::fore::lightblack  :  cc::fore::lightblue;
			if ((int)sort != boring) continue; // first 0 then 1

			out << color1 << std::setw(width1) << name ; // --cc
			if (info.getTakesValue()) out << " " << cc::fore::lightcyan << info.getName() << color1 ; // username
			out << cc::fore::console << " - an option " ;
			out << endl;
		}
	}
	out << cc::fore::console;
}

void cCmdFormat::PrintUsageShort(ostream &out) const {
	using namespace zkr;
	bool written=false;
	{
		out << cc::fore::lightyellow;
		size_t nr=0;  for(auto var : mVar) { if (nr) out<<" ";  out << (string)var;  ++nr; written=true; }
	}
	if (written) out<<" ";

	written=false;
	{
		auto color1 = cc::fore::lightcyan;
		out << color1;
		size_t nr=0;  for(auto var : mVarExt) { if (nr) out<<" ";  out << '[' << (string)var <<']';  ++nr; written=true; }
	}
	if (written) out<<" ";

	for (int sort=0; sort<=1; ++sort) {
		size_t nr=0;
		for(auto opt : mOption) {
			const string &name = opt.first;
			const cParamInfo &info = opt.second;
			bool boring = (info.getFlags().n.isBoring);
			auto color1 = boring  ?  cc::fore::lightblack  :  cc::fore::lightblue;
			if ((int)sort != boring) continue; // first 0 then 1

			out << color1 << "[" << name ; // --cc
			if (info.getTakesValue()) out << " " << cc::fore::lightcyan << info.getName() << color1 ; // username
			out << "] ";
		}
	}
	out << cc::fore::console;
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

size_t cCmdData::SizeAllVar() const { // return size of required mVar + optional mVarExt
	return mVar.size() + mVarExt.size();
}

string cCmdData::VarAccess(int nr, const string &def, bool doThrow) const { // see [nr] ; if doThrow then will throw on missing var, else returns def
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

void cCmdData::AssertLegalOptName(const string &name) const {
	if (name.size()<1) throw cErrArgIllegal("option name can not be empty");
	const size_t maxlen=100;
	if (name.size()>maxlen) throw cErrArgIllegal("option name too long, over" + ToStr(maxlen));
	// TODO test [a-zA-Z0-9_.-]*
}

vector<string> cCmdData::OptIf(const string& name) const {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) {
		return vector<string>{};
	}
	return find->second;
}

string cCmdData::Opt1If(const string& name, const string &def) const { // same but requires the 1st element; therefore we need def argument again
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) {
		return def;
	}
	const auto &vec = find->second;
	if (vec.size()<1) { _warn("Not normalized opt for name="<<name); return def; }
	return vec.at(0);
}


string cCmdData::VarDef(int nr, const string &def, bool doThrow) const {
	return VarAccess(nr, def, false);
}

string cCmdData::Var(int nr) const { // nr: 1,2,3,4 including both arg and argExt
	static string nothing;
	return nUtils::SpaceFromSpecial( VarAccess(nr, nothing, true) );
}

vector<string> cCmdData::Opt(const string& name) const {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) { _warn("Map was: [TODO]"); throw cErrArgMissing("Option " + name + " was missing"); }
	return find->second;
}

string cCmdData::Opt1(const string& name) const {
	AssertLegalOptName(name);
	auto find = mOption.find( name );
	if (find == mOption.end()) {  throw cErrArgMissing("Option " + name + " was missing"); }
	const auto &vec = find->second;
	if (vec.size()<1) { _warn("Not normalized opt for name="<<name); throw cErrArgMissing("Option " + name + " was missing (not-normalized empty vector)"); }
	return vec.at(0);
}

bool cCmdData::IsOpt(const string &name) const {
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

void cCmdData::AddOpt(const string &name, const string &value) { // append an option with value (value can be empty
	_dbg3("adding option ["<<name<<"] with value="<<value);
	auto find = mOption.find( name );
	if (find == mOption.end()) {
		mOption.insert( std::make_pair( name , vector<string>{ value } ) );
	} else {
		find->second.push_back( value );
	}
}

// ========================================================================================================================

cCmdDataParse::cCmdDataParse()
	: mFirstArgAfterWord(0), mFirstWord(0), mCharShift(0), mIsPreErased(false)
{
}


int cCmdDataParse::CharIx2WordIx(int char_ix) const {
	cParseEntity goal( cParseEntity::tKind::unknown , char_ix );
	int word_ix = RangesFindPosition( mWordIx2Entity , goal );
	ASRT(  (word_ix >= 0) );
	// TODO assert versus number of known words?
	return word_ix;
}

int cCmdDataParse::WordIx2ArgNr(int word_ix) const {
	if (word_ix<0) throw cErrAfterparse("Trying to use word_ix="+ToStr(word_ix));
	int arg_nr = word_ix - mFirstArgAfterWord;
	_dbg3("mFirstArgAfterWord="<<mFirstArgAfterWord);
	if (arg_nr < 0) arg_nr=0; // arg number 0 will mean "part of the name"
	// ASRT(arg_nr >=0 );
	return arg_nr;
}

// ========================================================================================================================


}; // namespace nNewcli
}; // namespace OT


