/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "cmd.hpp"
#include "cmd_detail.hpp"

#include "lib_common2.hpp"
#include "ccolor.hpp"

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

void cCmdParser::PrintUsage() {
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


cCmdProcessing cCmdParser::StartProcessing(const string &words, shared_ptr<nUse::cUseOT> use ) {
	return cCmdProcessing( shared_from_this() , words , use );
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

bool cCmdParser::FindFormatExists( const cCmdName &name ) throw()
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
	catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cCmdProcessing::_Parse(bool allowBadCmdname) {
	// int _dbg_ignore=50;
	bool test_char2word = false; // run a detailed test on char to word conversion

	mData = std::make_shared<cCmdDataParse>();
	mData->mOrginalCommand = mCommandLineString;

	// will be used to calculate offsets (e.g. words) between orginal string suppied (eg from user) and the upgraded string we store later (after addig pre "ot" etc)
	int namepart_words = 0; // how many words are in name SINCE BEGINING (including ot), "ot msg send"=3, "ot help"=2", "ot"=1
	int prepart_words = 0; // simillary, how many words are the begining, usually there is 1 (for "ot")

	if (mCommandLineString.empty()) { const string s="Command for processing was empty (string)"; _warn(s);  throw cErrParseSyntax(s); } // <--- THROW

	{
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
		_mark("Vector of words: " << DbgVector(mCommandLine));
		_mark("Words position mWordIx2Entity=" << DbgVector(mData->mWordIx2Entity));
	}

	if (test_char2word) { for (int i=0; i<mCommandLineString.size(); ++i) {
		const char c = mCommandLineString.at(i);
		_dbg3("char '" << c << "' on position " << i << " is inside word: " << mData->CharIx2WordIx(i) 	);
	} }

	if (mCommandLine.empty()) { const string s="Command for processing was empty (had no words)"; _warn(s);  throw cErrParseSyntax(s); } // <--- THROW

	// -----------------------------------
	if (mCommandLine.at(0) == "help") { mData->mCharShift=-3; namepart_words--; prepart_words--;  mCommandLine.insert( mCommandLine.begin() , "ot"); } // change "help" to "ot help"
	// ^--- namepart_words-- because we here inject the word "ot" and it will make word-position calculation off by one

	if (mCommandLine.at(0) != "ot") _warn("Command for processing is mallformed");
	mCommandLine.erase( mCommandLine.begin() ); // delete the first "ot" ***
	// mCommandLine = msg, send-from, alice, bob, hello
	_dbg1("Parsing (after erasing ot) : " << DbgVector(mCommandLine) );

	if (mCommandLineString.empty()) { const string s="Command for processing was empty (besides prefix)"; _warn(s);  throw cErrParseSyntax(s); } // <--- THROW

	prepart_words++;
	mData->mFirstWord = prepart_words; // usually 1, meaning that there is 1 word between actuall entities, e.g. when we remove the "ot" pre

	_mark("Shift: mCharShift=" << mData->mCharShift << " mFirstWord="<<mData->mFirstWord );

	int phase=0; // 0: cmd name  1:var, 2:varExt  3:opt   9:end
	try {
		string name_tmp = mCommandLine.at(0); // buld the name of command, start with 1st word like "msg" or "help"
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
		}

		const string name = name_tmp;
		_mark("command name = " << name);

		// "msg send" or "help"
		namepart_words++;
		_dbg3("Name of command is: " << name);
		_dbg3("namepart_words="<<namepart_words);
		mData->mFirstArgAfterWord = namepart_words;

		mData->mWordIx2Entity.at(0).SetKind( cParseEntity::tKind::pre );

		for (int i=1; i<=namepart_words; ++i) mData->mWordIx2Entity.at(i).SetKind( cParseEntity::tKind::cmdname , i ); // mark this words as part of cmdname
		_mark("Words position mWordIx2Entity=" << DbgVector(mData->mWordIx2Entity));

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
			while (true) { // parse options
				if (pos >= words_count) { _dbg1("reached END, pos="<<pos);	phase=9; break;	}

				string word = mCommandLine.at(pos);
				_dbg1("phase="<<phase<<" pos="<<pos<<" word="<<word);
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
						throw cErrParseSyntax("Expected an --option here, but got a word=" + ToStr(word) + " at pos=" + ToStr(pos));
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
		_warn("Command can not be parsed " << e.what());
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
	if (mStateParse == tState::never) Parse( true );
	if (mStateParse != tState::succeeded) {
		if (mStateParse == tState::succeeded_partial) _dbg3("Failed to fully parse.");  // can be ok - maybe we want to comlete cmd name like "ot msg sendfr~"
		else _dbg1("Failed to parse (even partially)");
	}
	ASRT(nullptr != mData);


	try {
		int word_ix = mData->CharIx2WordIx( char_pos  );
		_dbg1("word_ix=" << word_ix);
		int arg_nr = mData->WordIx2ArgNr( word_ix );

		_dbg1("mCommandLine=" << DbgVector(mCommandLine));
		string word_sofar = mCommandLine.at(word_ix - mData->mFirstWord);  // the current word that we need to complete. e.g. "--dryr" (and we will complete "--dryrun")
		long int word_previous_ixtab = word_ix - mData->mFirstWord - 1;
		const string word_previous = (word_previous_ixtab>=0) ? mCommandLine.at(word_previous_ixtab) : "";
		_dbg1("word_sofar="<<word_sofar<<", and previous word="<<word_previous<<" char_pos="<<char_pos);

		cParseEntity entity =  mData->mWordIx2Entity.at(word_ix);
		char sofar_last_char = mCommandLineString.at(char_pos-1); // the character after which we are now completing e.g. "g" for "msg~" or " " for "msg ~"
		const bool after_word = sofar_last_char==' ' ; // are we now after (e.g. 1st) word, e.g. because we stand on space like in  "ot msg ~"  (instead "ot msg~")
		_mark("Completion at pos="<<char_pos<<" word_ix="<<word_ix<<" arg_nr="<<arg_nr<<" entity="<<entity
			<<" word_sofar=["<<word_sofar<<"] sofar_last_char=["<<sofar_last_char<<"] after_word="<<after_word);

		if (entity.mKind == cParseEntity::tKind::option_name) {
			shared_ptr<cCmdFormat> format = mFormat;  // info about command "msg sendfrom"
			if (!format) return vector<string>{}; // if we did not understood command name, then return empty vector
			vector<string> matching = WordsThatMatch( word_sofar ,  format->GetPossibleOptionNames() );
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
				vector<string> matching = WordsThatMatch( word_sofar , hint);
				// TODO check if the word_ix here is correct
				return matching;
			} catch(std::exception &e) { } // something failed probaly the option name was not known
			return vector<string>{}; // the name of option seems unknown, so we can not offer any completion for the value for that option
		}
		else if (entity.mKind == cParseEntity::tKind::variable) {
			shared_ptr<cCmdFormat> format = mFormat;  // info about command "msg sendfrom"
			ASRT( format );
			cParamInfo param_info = format->GetParamInfo( arg_nr ); // eg. pNymFrom  <--- info about kind (completion function etc) of argument that we now are tab-completing
			vector<string> completions = param_info.GetFuncHint()  ( *mUse , *mData , arg_nr );
			_fact("Completions: " << DbgVector(completions));
			vector<string> matching = WordsThatMatch( mData->V( arg_nr ) ,  completions );
			return matching;
		}
		else if (entity.mKind == cParseEntity::tKind::cmdname) {
			const int cmd_word_nr = entity.mSub;
			_fact("Completing command name cmd_word_nr="<<cmd_word_nr<<" after_word="<<after_word);
			if ( (cmd_word_nr==1) && (!after_word) ) { // "ms~" or "msg~"
				vector<string> matching = WordsThatMatch( word_sofar , mParser->GetCmdNamesWord1() );
				return matching; // <---
			} else if ( (cmd_word_nr==1) && (after_word) )  { // "msg ~"
				vector<string> matching = WordsThatMatch( "" , mParser->GetCmdNamesWord2( word_sofar ) );
				return matching; // <---
			} else if ( (cmd_word_nr==2))  { // "msg se~"
				vector<string> matching = WordsThatMatch( word_sofar , mParser->GetCmdNamesWord2( word_previous ) );
				return matching; // <---
			} else throw cErrInternalParse("Bad cmd_word_nr="+ToStr(cmd_word_nr)+", after_word="+ToStr(after_word)+" in completion");
			return vector<string>{};
		}
		else if (entity.mKind == cParseEntity::tKind::pre) {
			return vector<string>{"ot"}; // TODO
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

cParamInfo::cParamInfo(const string &name, const string &descr, tFuncValid valid, tFuncHint hint, tFlags mFlags)
	: mName(name), mDescr(descr), funcValid(valid), funcHint(hint), mFlags(mFlags)
{ }

cParamInfo::cParamInfo(const string &name, const string &descr)
	: mName(name), mDescr(descr)
{ }

cParamInfo cParamInfo::operator<<(const cParamInfo &B) const {
	cParamInfo A = *this;
	A.mName = B.mName;
	A.mDescr = B.mDescr;
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

cCmdDataParse::cCmdDataParse()
	: mFirstArgAfterWord(0), mFirstWord(0), mCharShift(0)
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


