/* See other files here for the LICENCE that applies here. */

#include "lib_common2.hpp"

#include "othint.hpp"
#include "otcli.hpp"

#include "tests.hpp" // TODO Not needed

/**
OT Hints (new CLI - new commandline : auto complete commands, verify, check, etc)

Goal: this project aims to provide auto completion of newCLI OT commands, see [Description_of_auto_completion]
Description: See [Description_of_auto_completion] below
Example: "ot msg send bob a<TAB>" will ask remote OT and auto-complete alice.

Usage:
	./othint --complete-one "ot msg send ali"
	./othint --one "ot msg send ali"
	./othint --complete-shell
	./othint --shell
	./othint --devel # test various things etc

This subproject is separated out of OT, and uses C++11 and few other modern coding style changes.

Rules of language: use C++11, do not use boost (thoug we could copy small part of boost source code if needed,
or headers-only library)

OTNewCli composed of this parts:
- nExamplesOfConvention - documenation of coding style convention
- nOT::nUtils - various utils for modern (C++11) OT ; ToStr(), OT_CODE_STAMP
- nOT::nNewcli - new command line classes, to parse the command line options, and to execute commands
- nOT::nOTHint - new command line auto-completion. Move here things usable only for auto-completion and nothing else
- nOT::nTests - the testcases for our code
- main() is here

Coding rules:
- testcase driven development
- testcase on almost each compilation, commit often
- secure - use asserts, secure methods, etc. Use static analysis
- fast - optimize for speed overall, and in picked bottlenecks
- scalable - assume usage in massive scripts one day, e.g. 1 million users email server :)
- document all conventions, all code; Entire WG (working group) must know all of them
- flexible - easy to tune, to extend, in object-oriented way

File format of sources: identation with \t char, which we assume is 2 spaces wide. Unicode, UTF-8.

*/

// Editline. Check 'git checkout linenoise' to see linenoise version.
#ifndef CFG_USE_EDITLINE // should we use editline?
	#define CFG_USE_EDITLINE 1 // default
#endif

#if CFG_USE_EDITLINE
	#ifdef __unix__
		#include <editline/readline.h>
	#else // not unix
		// TODO: do support MinGWEditline for windows)
	#endif // not unix
#endif // not use editline

// OTNewcliCmdline

// Please read and follow this syntax examples from example_coding.cpp


namespace nOT {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

/*

Commandline-Functor - Cmdfunc
Commandline-Functor(s) are expressions that are expected to have certain value, e.g. one of your nyms (mynym).

Consider following command line:
	ot msg send mynym hisnym [ccnym]
	ot msg send $get_mynym $get_somenym [$get_somenym_o]
get_mynym is a Cmdfunc, and this one for example links to cUse::NymGetAllNames

They are defined in code in order to have following functions:
- othint - completion, e.g. mynym provides list of possible your nyms
- othint - parsing/verification, checking if the so-far entered command line is vali
- otcli  - parsing/verification, verifying the value from cmd line and checking if it is valid
- help - in either program it could be good to show name/description of given argument and generate doc
- testcase - they could produce various example command lines with valid data filled in
- code - heaving in-memory representation with tree of possible commands, with functors (named functions) could
	have other uses in future C++ code.

Cmdfunc in this C++ code are functions (methods), that take some extra data e.g. state of entire command line (usable in some cases),
and return possible data (mainly some vector of strings).

Syntax:
cCmdfuncReturn is the returnded data, mainly some vector of strings
cCmdfuncInput is the additional input data, that some functors might use
Functors will exist as member functions in various objects (mainly in cUse) called cCmdfuncProvider
- tCommandlineFunction is the functor type
- cCmdfuncProvider is the class that contains said members being pointed to by tCommandlineFunction

*/

// this class provides methods that can be plugged into command line parsing,
// e.g. that take state of command line,
// and return possible words that could be used in command line

class cCmdfuncProvider;

struct cCmdfuncReturn {
	vector<string> mWord;
};

struct cCmdfuncInput {
	public:
		const string& mSoFar; // the string with so-far provided command line
		cCmdfuncInput(const string &soFar);
};
cCmdfuncInput::cCmdfuncInput(const string &soFar) : mSoFar(soFar) { }

// tCmdfuncPtr - pointer to the Cmdfunc:
typedef cCmdfuncReturn (cCmdfuncProvider::*tCmdfuncPtr)(cCmdfuncInput);

class cCmdfunc { // the main Cmdfunc object - contains the pointer to member function doing the work, name, etc
	public:
		cCmdfunc( tCmdfuncPtr fun , const string & name);
	protected:
		tCmdfuncPtr mFun; // <---- PTR ! function
		const string mName;
};
cCmdfunc::cCmdfunc( tCmdfuncPtr fun , const string & name)
: mFun(fun), mName(name) { }

// ====================================================================

class cCmdfuncProvider {
	public:
};

}; // nOT

// ====================================================================

// ====================================================================

// TODO: move to own file
namespace nOT {
namespace nNewcli {

// list of things from libraries that we pull into namespace nOT::nNewcli
using std::string;
using std::vector;
using std::list;
using std::set;
using std::map;
using std::unique_ptr;
using std::cin;
using std::cerr;
using std::cout;
using std::endl;
using nOT::nUtils::ToStr;

// more rich information about a nym
class cNyminfo {
	public:
		cNyminfo(std::string name);

		operator const std::string&() const;

	public:
		string mName;
		int mScore;
};

cNyminfo::cNyminfo(std::string name)
: mName(name), mScore(0)
{ }

cNyminfo::operator const string&() const {
	return mName;
}


class cCmdname { // holds (2)-part name of command like "msg","send"
	// represents name of one command, including the (1)-2-3 components e.g. msg,send or msg,export,msg
	// "msg send"
	// "msg list"
	// "msg export msg"
	// "msg export all"
	// "msg" (not a valid command, will be used with null_function, is just to provide global options to inherit)

	protected:
		const string mTopic;
		const string mAction;
		const string mSubaction;

	public:
		cCmdname(const string &topic, const string &action, const string &subaction);
};

cCmdname::cCmdname(const string &topic, const string &action, const string &subaction)
:mTopic(topic), mAction(action), mSubaction(subaction)
{
}


/*
Build:
Tree of max options:
[msg] ---------> { --unicode }
[msg send] ----> (2) + (3) + {--now,--later,--sign,--red} , msg_send_complete()

msg_send_complete() {
	if (stage==arg) { if (arg_pos==1) get_my_nyms();    if (arg_pos==2) get_your_nyms(); }
	else if (stage==extra) { ... }
	else if (stage==option) {
		foreach (opt) ..
			if (--now) delete --later;
			if (uniq) delete this_option;
		} // existing options eliminate possibilities
		if (half_world) finish_option( remaining_options );
		else show_other_options();
	}
}
*/


/*
class argument_info {
	// class says what kind of argument is this i.e. that argument is "mynym" and it have to be string or integer or only boolean
	// and provide class!!! for auto complete

	string mName; // "mynym" "hisnym" (even those arguments which have specific order, so they don't have "name", will have, for example for generating help text

	// other mName will be for eg. "cc"  for option --cc which remain name

};
*/

/*
typedef string argument_data ; // for now...

class cCmdoptions {

		// required arguments
		vector< argument_info > // here should be { ("msg","send")  ,  ("msg","send","body") ect. same as in cCmdname::returnStandard

		map< argument_info , vector< argument_data > > // here will be map
}

class cCmdlineInfo {
	public:
	protected:
		vector<cCmdname> mPossible; // store here possible command names
		map<cCmdname , details_of_command> mPossibleDetails;
};
*/



} // namespace nNewcli
} // namespace nOT
// ########################################################################
// ########################################################################
// ########################################################################


// TODO: move to own file
namespace nOT {
namespace nOTHint {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2

using namespace nOT::nUtils::nOper; // vector + vector and other shortcut operators. It's appen, as in strings! :)



// ====================================================================

cHintData::cHintData()
{
}

// ====================================================================

cHintManager::cHintManager()
: mHintData(new cHintData)
{ }

void cHintManager::TestNewFunction_Tree() { // testing new code [wip]
	_info("Working on this="<<(void*)this);
	// XXX deprecated - moving to cmd.*
}

vector<string> cHintManager::AutoCompleteEntire(const string &sofar_str) const {
	bool dbg = false;
	const std::string cut_begining="ot"; // minimal beginning
	const int cut_begining_size = cut_begining.length();
	if (dbg) _info("cut_begining=[" << cut_begining << "]");
	// <= add space if we are at ot<tab>
	if (sofar_str.length() <= cut_begining_size) return WordsThatMatch(sofar_str, vector<string>{ cut_begining }); // too short, force completio to "ot"

	std::string ot = sofar_str.substr(0,cut_begining_size); // separate out the part that is know to has correct size and should be "ot"
	if (ot!=cut_begining) return vector<string>{}; // nothing matches, not command ot...

	// TODO optimize, avoid copy?
	std::string line = sofar_str;
	line.erase(0, cut_begining_size);

	return AutoComplete(line);
}

vector<string> cHintManager::AutoComplete(const string &sofar_str) const { // the main function to auto-complete
	// cerr << "COMPLETE for sofar=[" << sofar_str << "]." << endl;
	auto possible = BuildTreeOfCommandlines(sofar_str,false);
	return possible;
}


vector<string> cHintManager::BuildTreeOfCommandlines(const string &sofar_str, bool show_all) const {

	_erro("Not implemented currently (while refactoring)");

#if 0

/*
	nOT::nNewcli::cNew newcli;
	newcli.assign(sofar_str);
	newcli.parse();

}
*/
	bool dbg = false;
	string sofar_str_tmp = sofar_str;
	if (dbg) { _dbg3("sofar_str "<<sofar_str);};
	string esc ("\\ ");

	string newEsc = "#x#";
	// change Escape on new unique substring
	while(sofar_str_tmp.find(esc)!=std::string::npos) {
		sofar_str_tmp.replace(sofar_str_tmp.find(esc),esc.length(),newEsc);
		if (dbg) { _dbg3("sofar_str_tmp "<< sofar_str_tmp);}
	}

/*
TODO - planned new tree of commands using lambda

	v<s> suggest_nym_my() { return mynyms(); }
	NYM_FROM = argument_type { validate_nym_my() , suggest_nym_my() };
	NYM_TO   = argument_type { validate_nym_any() , suggest_nym_recipient_any() };
	ASSET_ANY  = argument_type { validate_asset_any() ,... };

  add("msg", "sendfrom", vars{ NYM_FROM, NYM_TO }, vars{ TEXT_SHORT }, opts{opt_priority}, opts{opt_cc, opt_bcc},
  		[](){ useOT.msgSendAdvanced(arg(1), arg(2), get_multiline(), arg(3), map<string>{{"cc",opt("cc")} ); } );
  add("msg", "sendto", vars{ NYM_TO }, vars{ TEXT_SHORT }, opts{opt_priority}, opts{opt_cc, opt_bcc},
  		[](){ useOT.msgSendAdvanced(nym_me(), arg(2), get_multiline(), arg(3), map<string>{{"cc",opt("cc")} ); } );

*/
	vector<string> sofar = nUtils::SplitString(sofar_str_tmp);

	if (dbg) { _dbg3("sofar.size()  "<<sofar.size());};

	for (auto& rec : sofar) {
		while(rec.find(newEsc)!=std::string::npos) {
			//first back to Escape
			rec = rec.replace(rec.find(newEsc),newEsc.length(),esc);
			//second remove Escape
			rec = SpaceFromEscape(rec);
		}
	}

	if (dbg) for (auto& rec : sofar) _dbg3("rec "<< rec);

	// exactly 2 elements, with "" for missing elements
	decltype(sofar) cmdPart;
	decltype(sofar) cmdArgs;
	if (sofar.size()<2) {
		cmdPart = sofar;
	}
	else {
		cmdPart.insert( cmdPart.begin(), sofar.begin(), sofar.begin()+2 );
		cmdArgs.insert( cmdArgs.begin(), sofar.begin()+2, sofar.end() );

	}
	while (cmdPart.size()<2) cmdPart.push_back("");
	if (dbg) { _dbg3("parts "<< vectorToStr(cmdPart));};
	if (dbg) { _dbg3("args "<< vectorToStr(cmdArgs));}

	if (GetLastCharIf(sofar_str)==" ") {
		if( sofar.size()>=1 ) { // if there is any last-word element:
			if (dbg) _dbg3("Adding space after last word to mark that it was ended");
			sofar.at( sofar.size()-1 )+=" "; // append the last space - to the last word so that we know it was ended.
		}
	}

	const vector<string> cmdFrontOpt = {"--H0","--HL","--HT","--HV","--hint-remote","--hint-cached","--vpn-all-net"};

	const string topic  =  cmdPart.at(0) ;
	const string action =  cmdPart.at(1) ;

	int full_words=0;
	int started_words=0;

	bool last_word_pending=false;
	size_t nr=0;
	string prev_char;
	for (auto rec : sofar) {
		if (rec!="") started_words++;
		if (last_word_pending) { full_words++; last_word_pending=0; }
		if (GetLastCharIf(rec)==" ") full_words++; else last_word_pending=1; // we ended this part, without a space, so we have a chance to count it
		// still as finished word if there is a word after this one
		++nr;
		prev_char = rec;
	}
	string current_word="";
	if (full_words < started_words) current_word = sofar.at(full_words);
	if (dbg) { _dbg3("full_words=" << full_words << " started_words="<<started_words
		<< " topic="<<topic << " action="<<action
		<< " current_word="<<current_word <<endl);
	}

	// TODO produce the object of parsed commandline by the way of parsing current sofar string
	// (and return - via referenced argument)

	// * possib variable - short for "possibilities"

	// TODO support discarding forward-opion flags
	// ...

	// === at 1st (non-front-option) word (topic) ===

	if (full_words<1) { // at 1st word (topic) -> show all level 1 cmdnames
		//TODO new option that will show actual settings (nym, account, server, purse) -> implement profiles
		return WordsThatMatch(  current_word  ,  vector<string>{"account", "account-in", "account-out", "asset", "basket", "cash", "cheque", "contract", "market", "mint", "msg", "nym", "nym-cred", /*"receipt"??,*/ "server", "text", "voucher"/*, "wallet"??*/} + cmdFrontOpt  ) ;
		//commented procedures are those which we ain't sure if they will appear - definitions below
	}

	// === at 2nd (non-forward-option) word (action) ===


	if (topic=="account") {

		if (full_words<2) { // word2 - the action:
			string defAcct = nOT::nUse::useOT.AccountGetDefault();
			cout << "Default account: " << nOT::nUse::useOT.AccountGetName(defAcct) + " - " + defAcct << endl;// <====== Execute - show active account
			return WordsThatMatch(  current_word  ,  vector<string>{"new", "ls", "refresh", "rm", "set-default", "mv"} ) ;
		}
		if (full_words<3) { // word3 (cmdArgs.at(0))
			if (action=="new") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.AssetGetAllNames() ) ; // <====== Execute
			}
			if (action=="ls") {
				nOT::nUtils::DisplayVectorEndl( cout, nOT::nUse::useOT.AccountGetAllNames() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="refresh") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames() ) ;
			}
			if (action=="rm") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.AccountGetAllNames() ) ;
			}
			if (action=="set-default") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.AccountGetAllNames() ) ;
			}
			if (action=="mv") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.AccountGetAllNames() ) ;
			}
		}

		if (full_words<4) { // word4 (cmdArgs.at(1))
			if (action=="new") {
				if (nOT::nUse::useOT.AssetCheckIfExists(cmdArgs.at(0))) {
					return vector<string>{};
				}
				else {
					std::cout << "asset " << SpaceFromEscape(cmdArgs.at(0)) << " don't exists" << endl;
					return vector<string>{};
				}
			}
			if (action=="rm") {
				if (nOT::nUse::useOT.AccountCheckIfExists(cmdArgs.at(0))) {
					return vector<string>{nOT::nUse::useOT.AccountDelete(cmdArgs.at(0))};
				}
				else {
					std::cout << "No account with this name: "  << (cmdArgs.at(0)) << endl;
					return vector<string>{};
				}
			}
			if (action=="set-default") {
				nOT::nUse::useOT.AccountSetDefault(cmdArgs.at(0));
			}
			if (action=="mv") {
				std::cout <<"Pass new account name";
				return vector<string>{};
			}
		}

		if (full_words<5) { // word5 (cmdArgs.at(2))
			if (action=="new") {
				if (!nOT::nUse::useOT.AccountCheckIfExists(cmdArgs.at(1))) { // make sure that the name is unique
 					nOT::nUse::useOT.AccountCreate(cmdArgs.at(0), cmdArgs.at(1)); // <====== Execute
					return vector<string>{};
				}
				else {
					std::cout << "Name " << cmdArgs.at(1) << " exists, choose another name ";
					return vector<string>{};
				}

			}
			if (action=="mv") {
				if (!nOT::nUse::useOT.AccountCheckIfExists(cmdArgs.at(1))) { // make sure that the name is unique
					nOT::nUse::useOT.AccountRename(cmdArgs.at(0), cmdArgs.at(1)); // <====== Execute
					return vector<string>{};
				}
				else {
					std::cout << "Name " << cmdArgs.at(1) << " exists, choose another name ";
					return vector<string>{};
				}
			}
		}

	} // account

	if (topic=="account-in") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  ,  vector<string>{"accept", "ls"} ) ;
		}
		if (full_words<3) { // we work on word3 - var1
			if (action=="accept") {
				//TODO
				return WordsThatMatch(  current_word  ,  vector<string>{"--all", "<paymentID>"} ) ;
			}
			if (action=="ls") {
				//TODO
				return WordsThatMatch(  current_word  ,  vector<string>{"<accountID>"} ) ;
			}
		}
	}

	if (topic=="account-out") {
		return WordsThatMatch(  current_word  ,  vector<string>{"ls"} ) ;
	}

	if (topic=="asset") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  ,  vector<string>{"issue", "ls", "new"} ) ;
		}
		if (full_words<3) { // we work on word3 - cmdArgs.at(0) - asset name
			if (action=="ls") {
				nOT::nUtils::DisplayVectorEndl( cout,  nOT::nUse::useOT.AssetGetAllNames() ) ;
				return vector<string>{};
			}
			else if (action=="issue") {
				nOT::nUtils::DisplayStringEndl( cout, "Please paste a currency contract, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, "SERVER RESPONSE:\n" + nOT::nUse::useOT.AssetIssue( nOT::nUse::useOT.ServerGetDefault(),nOT::nUse::useOT.NymGetDefault(), nOT::nUtils::GetMultiline() ) );
				return vector<string>{};
			}
			else if (action=="new") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter the XML contents for the contract, followed by an EOF or a ~ by itself on a blank line:");
				const string assetID = nOT::nUse::useOT.AssetNew( nOT::nUse::useOT.NymGetDefault(), nOT::nUtils::GetMultiline() );
				nOT::nUtils::DisplayStringEndl( cout, "ASSET ID:\n" + assetID );
				nOT::nUtils::DisplayStringEndl( cout, "SIGNED CONTRACT:\n" + nOT::nUse::useOT.AssetGetContract(assetID) );
				return vector<string>{};
			}
		}
	}

	if (topic=="basket") {
		return WordsThatMatch(  current_word  ,  vector<string>{"exchange", "ls","new" } ) ;
	}

	if (topic=="cash") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  ,  vector<string>{"send"} ) ;
		}
		if (full_words<3) {
			if (action=="send") {
				return WordsThatMatch(  current_word  ,  vector<string>{"<mynym>"} ); //TODO Suitable changes to this part - propably after merging with otlib
			}
		}
		if (full_words<4) { // we work on word3 - var1
			if (cmdArgs.at(0)=="send") {
				return WordsThatMatch(  current_word  ,  vector<string>{"<hisnym>"} ); //TODO Suitable changes to this part - propably after merging with otlib
			}
		}
	}

	if (topic=="cheque") {
		return WordsThatMatch(  current_word  ,  vector<string>{"new"} ) ;
	}

	if (topic=="contract") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  ,  vector<string>{"get", "new", "sign"} ) ;
		}
		if (full_words<3) {
			if (action=="get") {
				return WordsThatMatch(  current_word  ,  vector<string>{"<contractID>"} ); //TODO Suitable changes to this part - propably after merging with otlib
			}
		}
	}

	if (topic=="market") {
		return WordsThatMatch(  current_word  ,  vector<string>{"ls"} ) ;
	}

	if (topic=="mint") {
		return WordsThatMatch(  current_word  ,  vector<string>{"new"} ) ;
	}

	if (topic=="msg") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  , vector<string>{"ls", "mv", "rm", "sendfrom", "sendto" } );
		}

		if (full_words<3) { // we work on word3 - var1
			if (action=="ls") {
				//return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames() ) ; // TODO nyms autocomplete
				nOT::nUse::useOT.MsgGetAll(); // <====== Execute
				return vector<string>{};
			}
			if (action=="mv") {
				return WordsThatMatch(  current_word  ,  vector<string>{"Where-to?"} ); // in mail box... will there be other directories?
			}
			if (action=="rm") { // nym
				return WordsThatMatch(  current_word  , nOT::nUse::useOT.NymGetAllNames() );
			}
			if (action=="sendfrom") { // sender name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames() );
			}
			if (action=="sendto") { // recipient name TODO finish sendto functionality
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames() ); //TODO propose nyms from adressbook
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="ls") {
				if (nOT::nUse::useOT.NymCheckIfExists( cmdArgs.at(0) )) {
					nOT::nUse::useOT.MsgGetForNym( cmdArgs.at(0) ); // <====== Execute
					return vector<string>{};
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
			if (action=="rm") { // nym
				if (nOT::nUse::useOT.NymCheckIfExists(cmdArgs.at(0))) {
					cout << "Index?" << endl;
					return vector<string>{};
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
			if (action=="sendfrom") { // recipient name
				if (nOT::nUse::useOT.NymCheckIfExists(cmdArgs.at(0))) {
					return WordsThatMatch(  current_word  , nOT::nUse::useOT.NymGetAllNames() );
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
		}

		if (full_words<5) { // we work on word5 - var3
			if (action=="sendfrom") { // message text
				if (nOT::nUse::useOT.NymCheckIfExists(cmdArgs.at(1))) {
					nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of message, followed by an EOF or a ~ by itself on a blank line:" );
					nOT::nUse::useOT.MsgSend(cmdArgs.at(0), cmdArgs.at(1), nOT::nUtils::GetMultiline()); // <====== Execute
					return vector<string>{}; // ready for message
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(1);
					return vector<string>{};
				}
			}
			if (action=="rm") { // nym
				if (nOT::nUse::useOT.NymCheckIfExists(cmdArgs.at(0))) {
					nOT::nUse::useOT.MsgInRemoveByIndex( cmdArgs.at(0), std::stoi(cmdArgs.at(1)) );
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
		}

		if (full_words<6) { // we work on word6
			if (action=="sendfrom") { // message text
				nOT::nUse::useOT.MsgSend(cmdArgs.at(0), cmdArgs.at(1), cmdArgs.at(2)); // <====== Execute
				return vector<string>{};
			}
		}


	} // msg

	if (topic=="nym") {
		if (full_words<2) { // we work on word2 - the action:
			string defNym = nOT::nUse::useOT.NymGetDefault();
			cout << "Default Nym: " << nOT::nUse::useOT.NymGetName(defNym) + " - " + defNym << endl;// <====== Execute - show active nym
			return WordsThatMatch(  current_word  ,  vector<string>{"check", "edit", "export", "import", "info", "ls", "new", "refresh", "register", "rm", "set-default"} ) ;
		}
		if (full_words<3) { // we work on word3 - cmdArgs.at(0)

			if (action=="check") { // TODO interactive input
				nOT::nUtils::DisplayStringEndl(cout, "Type NymID to check"); // <====== Execute
				return vector<string>{};
			}
			if (action=="edit") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.NymGetAllNames() );//TODO Suitable changes to this part - propably after merging with otlib
			}
			if (action=="info") {
				//nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.nymGetInfo(cmdArgs.at(0)) ); // <====== Execute TODO For default nym
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.NymGetAllNames() );
			}
			if (action=="ls") {
				nOT::nUtils::DisplayVectorEndl(cout, nOT::nUse::useOT.NymGetAllNames(), "\n"); // <====== Execute
				return vector<string>{};
			}
			if (action=="new") {
				nOT::nUtils::DisplayStringEndl(cout, "Type new Nym name"); // <====== Execute
				return vector<string>{};
			}
			if (action=="refresh") {
				nOT::nUse::useOT.NymRefresh(); // <====== Execute
				return vector<string>{};
			}
			if (action=="register") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.NymGetAllNames() );//TODO server name
			}
			if (action=="rm") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.NymGetAllNames() );//TODO Suitable changes to this part - propably after merging with otlib
			}
			if (action=="set-default") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.NymGetAllNames() );
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="check") {
				nOT::nUse::useOT.NymCheck( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="info") {
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.NymGetInfo(cmdArgs.at(0)) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="new") {
				nOT::nUse::useOT.NymCreate( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="register") {
				nOT::nUse::useOT.NymRegister( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="set-default") {
				nOT::nUse::useOT.NymSetDefault( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
		}
	} // nym

	if (topic=="nym-cred") {
		return WordsThatMatch(  current_word  ,  vector<string>{"new", "revoke" , "show"} ) ;
	}

	/*if (topic=="receipt") {
		return WordsThatMatch(  current_word  ,  vector<string>{"[BLANK]"} ) ;
	}*/

	if (topic=="server") {
		if (full_words<2) { // we work on word2 - the action:
			string defServer = nOT::nUse::useOT.ServerGetDefault();
			cout << "Default server: " << nOT::nUse::useOT.ServerGetName(defServer) + " - " + defServer << endl;// <====== Execute - show active server
			return WordsThatMatch(  current_word  ,  vector<string>{"add", "check", "ls", "new", "set-default"} ) ;
		}

		if (full_words<3) { // we work on word3 - var1
			if (action=="add") {
				nOT::nUtils::DisplayStringEndl( cout, "Please paste a server contract, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUse::useOT.ServerAdd( nOT::nUtils::GetMultiline() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="check") {
				nOT::nUse::useOT.ServerCheck();// <====== Execute
				return vector<string>{};
			}
			if (action=="ls") {

				nOT::nUtils::DisplayVectorEndl(cout, nOT::nUse::useOT.ServerGetAllNames() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="set-default") {
				return WordsThatMatch(  current_word  , nOT::nUse::useOT.ServerGetAllNames() );
				return vector<string>{};
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="set-default") {
				nOT::nUse::useOT.ServerSetDefault(cmdArgs.at(0)); // <====== Execute
				return vector<string>{};
			}
		}

	} // server

	if (topic=="text") {
		if (full_words<2) { // we work on word2 - the action:
			return WordsThatMatch(  current_word  ,  vector<string>{"encode", "decode", "encrypt", "decrypt" } ) ; //coding
		}

		if (full_words<3) { // we work on word3 - var1
			if (action=="encode") { // text to encode
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be encoded, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextEncode(nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}

			if (action=="decode") { // text to decode
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be decoded, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextDecode(nOT::nUtils::GetMultiline()) ); // <====== Execute
				return vector<string>{};
			}

			if (action=="encrypt") { // recipient Nym Name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames());
			}

			if (action=="decrypt") { // recipient Nym Name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.NymGetAllNames());
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="encode") {
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextEncode(cmdArgs.at(0))); // <====== Execute
				return vector<string>{};
			}

			if (action=="decode") {
				return WordsThatMatch(  current_word  ,  vector<string>{} ) ; //encoded text will be always multiline
			}

			if (action=="encrypt") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be encrypted, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextEncrypt(cmdArgs.at(0), nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}

			if (action=="decrypt") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be decrypted, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextDecrypt(cmdArgs.at(0), nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}
		}

		if (full_words<5) { // we work on word5 - var3
			if (action=="encrypt") { // if plain text is passed as argument (don't implemented for decrypt action because of multiline encrytped block)
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.TextEncrypt(cmdArgs.at(0), cmdArgs.at(1))); // <====== Execute
				return vector<string>{};
			}
		}
	} // topic=="text"

	if (topic=="voucher") {
		return WordsThatMatch(  current_word  ,  vector<string>{"new"} ) ;
	}

	/*if (topic=="wallet") {
		return WordsThatMatch(  current_word  ,  vector<string>{"status"} ) ;
	}*/

#endif

	return vector<string>{};
	//throw std::runtime_error("Unable to handle following completion: sofar_str='" + ToStr(sofar_str) + "' in " + OT_CODE_STAMP);
}


// ======================================================================================

cInteractiveShell::cInteractiveShell()
:dbg(false)
{ }

void cInteractiveShell::_runOnce(const string line, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	gCurrentLogger.setDebugLevel(100);

	auto parser = make_shared<nNewcli::cCmdParser>();
	gReadlineHandleParser = parser;
	//gReadlineHandlerUseOT = use;
	parser->Init();
	vector <string> completions;
	auto processing = gReadlineHandleParser->StartProcessing(line, use);
	completions = processing.UseComplete( line.size() ); // Function gets line before cursor, so we need to complete from the end

	nOT::nUtils::DisplayVectorEndl(std::cout, completions);

}

void cInteractiveShell::runOnce(const string line, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	try {
		_runOnce(line, use);
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

extern bool my_rl_wrapper_debug; // external

bool my_rl_wrapper_debug; // external

// [null-trem-cstr] definition: as defined by readline, the format of null-term-cstr is:
// dynamic array, null terminated, of dynamic c-strings.
// caller is responsible for deallocation of the pointed strings data (using free) as well as the array itself
// (using free).
// it can be NULL for 0 element array
// exmple:
// NULL - empty array
// { strdup("foo") , NULL } - 1 record array
// { strdup("foo") , strdup("bar", NULL } - 2 record array


// When readline will call us to complete "ot m" then our function will be called with number=0,
// then it should cache possibilities of endings "msg" "mint", and return 0th (first) one.
// Next it will be called with other number (probably 1,2,3..) and return N-th possibility.
// Function is non-reentrant also in the meaning that it can not be called in interlace, e.g.
// ("ot m",0) then ("ot m",1) then ("ot x",0) and suddenly back to ("ot x",2) without reinitialization
// (done with number=0) is an error (at least currently, in future we might cache various completion
// arrays, or recalculate on change)

shared_ptr<nNewcli::cCmdParser> gReadlineHandleParser;
shared_ptr<nUse::cUseOT> gReadlineHandlerUseOT;

/**
Caller: before calling this function gReadlineHandleParser and gReadlineHandlerUseOT must be set!
Caller: you must free the returned char* memory if not NULL! (this will be done by readline lib implementation that calls us)
*/
static char* CompletionReadlineWrapper(const char *sofar , int number) {
	// sofar - current word,  number - number of question / of word to be returned
	// rl_line_buffer - current ENTIER line (or more - with trailing trash after rl_end)
	// rl_end - position to which rl_line_buffer should be read
	// rl_point - current CURSOR position
	// http://www.delorie.com/gnu/docs/readline/rlman_28.html

	bool dbg = my_rl_wrapper_debug;
	dbg=false; // XXX
	ASRT( !(gReadlineHandleParser == nullptr) ); // must be set before calling this function
	ASRT( !(gReadlineHandlerUseOT == nullptr) ); // must be set before calling this function

	// rl_line_buffer, rl_point not in WinEditLine API TODO should be possible to get this

	string line_all;
	if (rl_line_buffer) line_all = string(rl_line_buffer).substr(0,rl_end); // <<<
	string line = line_all.substr(0, rl_point); // Complete from cursor position
	if (dbg) _dbg2("sofar="<<sofar<<" number="<<number<<" rl_line_buffer="<<rl_line_buffer<<" and line="<<line<<endl);


	static vector <string> completions;
	if (number == 0) {
		if (dbg) _dbg3("Start autocomplete (during first callback, number="<<number<<") of line="<<line);
		auto processing = gReadlineHandleParser->StartProcessing(line_all, gReadlineHandlerUseOT);
		completions = processing.UseComplete( rl_point );
		_note( "TAB-Completion: " << DbgVector(completions) );
		if (dbg) _dbg3("Done autocomplete (during first callback, number="<<number<<"); completions="<<DbgVector(completions));
	}

	auto completions_size = completions.size();
	if (!completions_size) {
		if (dbg) _dbg3("Stop autocomplete: no matching words found because completions_size="<<completions_size);
		return NULL; // <--- RET
	}
	if (dbg) _dbg3( "completions_size=" << completions_size << endl);
	if (number==completions_size) { // stop
		if (dbg) _dbg3("Stop autocomplete because we are at last callback number="<<number<<" completions_size="<<completions_size);
		return NULL;
	}
	if (dbg) _dbg3("Current completion number="<<number<<" is: [" + completions.at(number) + "]");
	return strdup( completions.at(number).c_str() ); // caller must free() this memory
}

char ** completion(const char* text, int start, int end __attribute__((__unused__))) {
	rl_attempted_completion_over = 0;
	char **matches;
	matches = (char **)NULL;
	matches = rl_completion_matches (text, CompletionReadlineWrapper);
	if ( matches == (char **)NULL ) // Disable filename autocompletion
		rl_attempted_completion_over = 1;
	return (matches);
}

void cInteractiveShell::runEditline(shared_ptr<nUse::cUseOT> use) {
	try {
		_runEditline(use);

	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cInteractiveShell::_runEditline(shared_ptr<nUse::cUseOT> use) {
	_mark("Running editline loop");
	// nOT::nUse::useOT.Init(); // Init OT on the beginning // disabled to avoid some problems and delay (and valgrid complain)

	char *buf = NULL;
	my_rl_wrapper_debug = dbg;
	rl_attempted_completion_function = completion;
	rl_bind_key('\t',rl_complete);

	auto parser = make_shared<nNewcli::cCmdParser>();
	gReadlineHandleParser = parser;
	gReadlineHandlerUseOT = use;
	parser->Init();

	int said_help=0, help_needed=0;
	const int opt_repeat_help_each_nth_time = 5; // how often to remind user to run ot help on error

	cout << endl << "For help type: ot help" << endl;

	read_history("otcli-history.txt");

	while (true) {
		try {
			_fact("Waiting for user input via readline (time "<<time(NULL)<<")");
			buf  = readline("ot command> "); // <=== READLINE
			_dbg3("Readline returned");
			if (buf==NULL) break;

			std::string word;
			if (buf) word=buf; // if not-null buf, then assign
			if (buf) { free(buf); buf=NULL; }
			// do NOT use buf variable below.

			if (dbg) cout << "Word was: " << word << endl;
			std::string cmd;
			if (rl_line_buffer) cmd = rl_line_buffer; // save the full command into string
			cmd = cmd.substr(0, cmd.length()-1); // remove \n

			_info("Command is: " << cmd );
			auto cmd_trim = nOT::nUtils::trim(cmd);
			if (cmd_trim=="exit") break;
			if (cmd_trim=="quit") break;
			if (cmd_trim=="q") break;

			if (cmd.length()) {
				add_history(cmd.c_str()); // TODO (leaks memory...) but why
				write_history("otcli-history.txt"); // Save new history line to file

				bool all_ok=false;
				try {
					_dbg1("Processing command");
					auto processing = parser->StartProcessing(cmd, use); // <---
					_info("Executing command");
					processing.UseExecute(); // <--- ***
					all_ok=true;
					_info("Executed command.");
				}
				catch (const myexception &e) {
					cerr<<"ERROR: Could not execute your command ("<<cmd<<")"<<endl;
					cerr << e.what() << endl;
					//e.Report();
					cerr<<endl;
				}
				catch (const std::exception &e) {
					cerr<<"ERROR: Could not execute your command ("<<cmd<<") - it triggered internal error: " << e.what() << endl;
				}

				if (!all_ok) { // if there was a problem
					if ((!said_help) || (!(help_needed % opt_repeat_help_each_nth_time))) { cerr<<"If lost, type command 'ot help'."<<endl; ++said_help; }
					++help_needed;
				}
			} // length

		} // try an editline turn
		catch (const std::exception &e) {
			cerr << "Problem while reading your command: " << e.what() << endl;
			_erro("Error while reading command: " << e.what() );
		}
	} // while
	int maxHistory = 100; //TODO move this to settings
	history_truncate_file("otcli-history.txt", maxHistory);
	if (buf) { free(buf); buf=NULL; }
	clear_history(); // http://cnswww.cns.cwru.edu/php/chet/readline/history.html#IDX11

	gReadlineHandlerUseOT->CloseApi(); // Close OT_API at the end of shell runtime
}

}; // namespace nOTHint
}; // namespace nOT
// ########################################################################
// ########################################################################
// ########################################################################

std::string gVar1; // to keep program input argument for testcase_complete_1
// ====================================================================

// #####################################################################

