/* See other files here for the LICENCE that applies here. */

#include "lib_common2.hpp"

#include "otcli.hpp"
#include "othint.hpp"
#include "useot.hpp"

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
get_mynym is a Cmdfunc, and this one for example links to cUse::nymsGetMy

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

/*

[Description_of_auto_completion]

=== Introduction ===

First an advance example of magic of OT hints:
	ot msg send bob al<TAB>
will auto-complete options:
	alice alex alcapone
In this situation, bash autocomplete with othint program
will query the OT server to list your contacts named al...

With respecting your privacy, trying to use remote server only if
data was not cached, and only asking servers that you trust (the --HT option)
that is the default. Start with "ot --H0" to make sure there will be 0 network
activity, no servers will be asked:
	ot --H0 msg send bob a<TAB>
will auto-complete with data that can be given without causing network traffic.

ot_secure:
If it's more convinient, we might provide separate command: "ot_net", "ot_secure"
with other level of discretion in the hinting process as well with say more
confirmations when also EXECUTING a command that will connect to OT server.
	ot_quiet mint spawn bob silve<TAB>
	1. will NOT ask any server about the list of currenies silve... will use cache
		silvergrams silvertest silverbob {showing only cached data, last update 3h ago}
	2. when the command is finished and executed as
		ot_quiet mint spawn bob silvergrams 1000<ENTER>
	it will ask: "Please confirm do you want to connect NOW to OT server aliased BigTest1,
	with ID=855gusd2dffj2d9uisdfokj233 (unknown/new server), and execute
	mint-spawn command? Type 2 words: 'unknown spawn' to confirm, or 'no'"

Also name alice is in front (instead alphabet sorting) since it was recently/frequently used.

All OT commands will be neatly supported in this way.


=== Commands are in form ===

ot [front1,front2...] topic action   	var1,...  [varM,...] [--optNameN[=[optArgN]]...]

ot <-front options--> <- cCmdname --> <---- arguments ------------------------------->
ot                    <- cmd name --> <---- vars -----> <-- options ----------------->
ot <--- optional ---> <- mandatory--> <--- optional ------------------------>
ot <-- cmdFrontOpt -> <- cmdPart ---> <-------------- cmdArgs ----------------------->

Examples:
ot  --H0              msg   send              bob a
ot                    msg   send              bob alice            --attach scan.jpeg
ot                    msg   send              bob alice carol      --attach scan.jpeg
ot                    msg   send              bob alice carol      -v -v -v
ot  --hint-private    msg   send              bob alice carol      -v -v -v
ot  --hint-cached     msg   send              bob alice carol      -v -v -v
ot  --hint-cached     msg   help

Examples of SYNTAX ERRORS:
ot                    msg                     bob  alice # missing subcommand
ot                    msg   send              bob  # requires at least 2 arguments(*)
ot                    msg   send              bob  alice           --date=now --date=0 # date is unique
ot                    msg   send              bob  --hint-cachead=0 # forward command must be in font

(*) possibly such errors will instead allow execution and have the program interactivly ask for missing mandatory vars.

Therefore the syntax is:
[front-options] 2 words of command name, 0..N mandatory variables, 0..M extra variables, any options

	- subaction will be probably not used but leaving such possibility, then it would be 2..3 words

ARGUMENTS:
	- front-options are special options that must appear in front because they change meaning/parsing
		of everything next - particullary, auto-completion options. Read section [front-options] for details

	- Arguments available depend on the command name.

	- Options for command depend on the command name, as well are imported from global.

	- Options can't be placed before variables!

	- Options can be unique or can repeat. Options can have no value/data,
		or can have one.This gives 2*2 = 4 kinds of options: uniq, uniqData,
		repeat, repeatData.

	- Options can be both global and comming from selected action/subaction.

SEE testcases below in functions

#----------------Errors------------------#
msg     # error: incomplete action
msg send     # error: missing required options
msg send <mynym>     # error: missing required options
#----------------Errors------------------#

[*] - works
[/] - wip

#------List of all included commands-----#
account			# can display active (default) account
*account ls			# list all accounts
*account new			# make new account with UI
*account new <assetName>			# make new account by giving only <assetID>...
*account new <assetName> <accountName>			#... and <accountName>
account refresh			#	refresh database of private accounts' list
account set-default <accountID> # set default account
*account rm <accountName>			# delete account
*account mv <oldAccountName>	<newAccountName>		# rename account
--- Account inbox actions ---
account-in ls			# for active account
account-in ls <accountID>			# for specific <accountID>
account-in accept <paymentID>				#	accept this particular payment
account-in accept --all		# accept all incoming transfers, as well as receipts
--- Account outbox actions ---
account-out ls
------------------------------
asset				# can display active (default) asset - TODO check differences beetween asset and purse
*asset ls		# display all assets
*asset issue # Issue asset for default nym and server
*asset new # add new asset to the wallet
------------------------------
basket new
basket ls
basket exchange
cash send <mynym> <hisnym>
cheque new
contract? new  # Managed by asset new / server new
contract? get <contractID>
contract? sign
------------------------------
market # can't test markets for now
market ls
------------------------------
mint new
------------------------------
*msg			# should show what options do you have with this topic
/msg sendto		# should ask you about nyms ?
msg sendto <hisnym> 		# Send message from default nym to <hisnym>
*msg sendfrom <mynym> <hisnym> message 		# Send message from <mynym> to <hisnym>
msg sendfrom <mynym> <hisnym> --push     	# TODO option "--push" has no sense because msg will be already on the server. Implement msg draft?
msg sendfrom <mynym> <hisnym> --no-footer   # action option
msg sendfrom <mynym> <hisnym> --cc <ccnym>  # action option with value
msg sendfrom <mynym> <hisnym> --cc <ccnym> --cc <ccnym2>
msg sendfrom <mynym> <hisnym> --cc <ccnym> --cc <ccnym2> --push  	 # example of force send (?) - not sure if it will appear
*msg ls			# list all messages for all nyms
/msg ls <mynym> # list all messages for nym
msg mv			# move message to different directory in your mail box
msg rm <index>		# remove message with <index> number for current nym
msg rm --all		# remove all messages from mail box for current nym
*msg rm <nymName> <index> # remove <index> message from mail inbox for <nymName>
msg rm-out <nymName> <index> # proposition for command removing msg from mail outbox
------------------------------
*nym 			# can display active (default) nym
*nym ls			# list of all nyms
nym new			# make new nym with UI (it should ask potential user to give the name
*nym new <nymName>			# make new nym by giving name without UI
nym rm <name>			# remove nym with such <name>
nym rm <nymID>		# remove nym with such <nymID>
*nym info <nymName>		# show information about such <nymName>
*nym info <nymID>		# show information about such <nymID>
nym edit <nymID>		# allows to edit information about such <nymID>
*nym register <nymName>	# register nym defined by nymName on default server
nym register <nymID>	# register nym defined by nymID on default server
/nym register <nymID> <serverID>	# register this specific <nymID> to specific <serverID> server
nym set-default <nymID> # set default nym
nym import		# import saved (somewhere?) nyms
nym export		# export nyms to (outerspace) :) ?
*nym check <nymID>			# returns Public Key of this <nymID> nym
*nym refresh			# refresh nym's list and its included informations
nym-cred new 			# change credential to trust?
nym-cred revoke
nym-cred show			# show all credential to trust?
------------------------------
receipt?
------------------------------
server			# can display active (default) server
/server ls			# as above but all servers are listed TODO: Display more information about servers
server add		# add new server contract
server new 	# like newserver
server set-default # set default server
------------------------------
*text encode	# interactively get multiline text
*text encode <text>
text encode <textfile>
*text decode # interactively get multiline text
text decode <textfile>
*text encrypt <recipientNymName> <text>
text encrypt <textfile>
*text decrypt # interactively get multiline text
text decrypt <textfile>
------------------------------
voucher new
wallet? status
#------List of all included commands-----#

That's all commands included to OTHint for now.
(?) means that we're not sure if it will appear in main program, those keywords are implemente but commented for now

------------------------------------------------------------------------
older opentxs commands:

acceptall	acceptinbox	acceptinvoices	acceptmoney
acceptpayments	acceptreceipts	accepttransfers	addasset
addserver	addsignature	balance		buyvoucher
cancel		changepw	nymCheck	clearexpired
clearrecords	confirm		credentials	decode
decrypt		delmail		deloutmail	deposit
discard		editacct	editasset	editnym
editserver	encode		encrypt		exchange
expired		exportcash	exportnym	getboxreceipt
getcontract	getmarkets	getmyoffers	getoffers
importcash	importnym	inbox		issueasset
killoffer	killplan	mail		newacct
newasset	newbasket	newcred		newkey
newnym		newoffer	newserver	outbox
outmail		outpayment	pass_decrypt	pass_encrypt
paydividend	payinvoice	payments	propose
records		refresh		refreshacct	refreshnym
register	revokecred	sendcash	sendcheque
sendinvoice	msgSend		sendvoucher	showaccounts
showacct	showassets	showbasket	showcred
showincoming	showmarkets	showmint	showmyoffers
shownym		shownyms	showoffers	showoutgoing
showpayment	showpurse	showservers	sign
stat		transfer	trigger		verifyreceipt
verifysig	withdraw	writecheque	writeinvoice
------------------------------------------------------------------------

[front-options] usage is for example:
If you type -HN in front of options, then Hints will use Network (-HN) to autocomplete, typing
ot -HN msg send myuser1 bo<TAB> will e.g. ask OT server(s) (e.g. over internet) about you address book
ot -HT msg send myuser1 bo<TAB> the same but for Trusted servers only (will not accidentially talk to other servers)
ot -H0 .... will make sure you will not ask OT servers, just use data cached
ot -HR .... will force othint to refresh all information from servers. Option useful if you want fresh information from servers in real time. Slow and dagerous for privacy.

The exact planned options are 2 settings: accessing remote and accessing cache.
	--hint-remote=V set's the privacy to level 0..9. 0=never ask remote severs for data needed for
	this autocompletion, 9 freely ask (less secure, because remote server see that we compose a command).
	1=local server (e.g. localhost that was marked as trusted)
	3=trusted servers (e.g. several servers you configure as trusted)
	5=uses network (all servers) but might avoid some really sensive data
	9=you fully ask the server owners and consent to possiblity of them (or their ISP, hosting) learning
	what you are planning to do

	--hint-cached=V set's the usage of cached data. 0=revalidate now all data from server.
	5=normal reasonable use of cache
	8=only cached data, unless we have no choice, 9=only cached data even if it causes an error

--hint-remote=0 implies --hint-cached=9 as we are not allowed to touch remote server at all
--hint-cached=0 implied --hints-remote=0 as we are ordering to touch remote server so we are not working in private mode

--hint-remote=0 --hint-cached=0 is disallowed syntax,
even though some commands that indeed do not need neither cached nor remote data could work,
like "ot --hint-remote=0 --hint-cached=0 msg help"
or "ot --hint-remote=0 --hint-cached=0 msg draft"
but for clarity it will be an error because likelly someone confused the options.
But then, --hint-allow-strange option will allow such syntax, if it appears in front of option causing this
contradiction.
E.g. this is allowed syntax:
ot --hint-allow-strange --hint-remote=0 --hint-cached=0 msg draft

Shortcuts:
--H0 gives hints offline        equals --hint-offline     equals --hint-remote=0 --hint-cached=9
--HL gives hints local          equals --hint-local       equals --hint-remote=1 --hint-cached=5
--HT gives hints trusted        equals --hint-trusted     equals --hint-remote=3 --hint-cached=5
--HN gives hints from network   equals --hint-net         equals --hint-remote=5 --hint-cached=5
--HR is forced net reload       equals --hint-reload      equals --hint-remote=9 --hint-cached=0
--HV gives VPNed network        equals --hint-vpn         equals --vpn-all-net --hint-remote=9 --hint-cached=5

VPN:
option --vpn-all-net will force hint autocompletion (and also the actuall commands, unless later
canceled with other options) to use only VPN or similar secure networking. Details will be
configured, it could be e.g. a VPN network, or possibly other secured (private) network facility.
It is guarnateed that if such secure network fails to work, then no network will be touched
and program will not leak any activity to open network (LAN, Internet, etc)
--HV might be most comfortable and yet quite secure option, usable e.g. from hotels.

VPN forced:
Global configuration option could force to always use VPN (append --vpn-all-net)
then use "ot --HN" will not auto-complete on <TAB> but show:
	{can not use --HN because your configuration disabled it, please try --HV}
and if executed as full command, will also refuse to work, with explanation.

Please remember that VPNs or even more advanced solutions are not that secure, and that
ot hint sends anyway lots of data of intended action, with timing correlation, to the OT server
in --HV case.
VPN only hides your IP a bit.
Full caching with --H0 or --HL is most secure, there is no home like localhost :)

*/


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
			string defAcct = nOT::nUse::useOT.accountGetDefault();
			cout << "Default account: " << nOT::nUse::useOT.accountGetName(defAcct) + " - " + defAcct << endl;// <====== Execute - show active account
			return WordsThatMatch(  current_word  ,  vector<string>{"new", "ls", "refresh", "rm", "set-default", "mv"} ) ;
		}
		if (full_words<3) { // word3 (cmdArgs.at(0))
			if (action=="new") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.assetsGetNames() ) ; // <====== Execute
			}
			if (action=="ls") {
				nOT::nUtils::DisplayVectorEndl( cout, nOT::nUse::useOT.accountsGet() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="refresh") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy() ) ;
			}
			if (action=="rm") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.accountsGet() ) ;
			}
			if (action=="set-default") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.accountsGet() ) ;
			}
			if (action=="mv") {
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.accountsGet() ) ;
			}
		}

		if (full_words<4) { // word4 (cmdArgs.at(1))
			if (action=="new") {
				if (nOT::nUse::useOT.assetCheckIfExists(cmdArgs.at(0))) {
					return vector<string>{};
				}
				else {
					std::cout << "asset " << SpaceFromEscape(cmdArgs.at(0)) << " don't exists" << endl;
					return vector<string>{};
				}
			}
			if (action=="rm") {
				if (nOT::nUse::useOT.accountCheckIfExists(cmdArgs.at(0))) {
					return vector<string>{nOT::nUse::useOT.accountDelete(cmdArgs.at(0))};
				}
				else {
					std::cout << "No account with this name: "  << (cmdArgs.at(0)) << endl;
					return vector<string>{};
				}
			}
			if (action=="set-default") {
				nOT::nUse::useOT.accountSetDefault(cmdArgs.at(0));
			}
			if (action=="mv") {
				std::cout <<"Pass new account name";
				return vector<string>{};
			}
		}

		if (full_words<5) { // word5 (cmdArgs.at(2))
			if (action=="new") {
				if (!nOT::nUse::useOT.accountCheckIfExists(cmdArgs.at(1))) { // make sure that the name is unique
 					nOT::nUse::useOT.accountCreate(cmdArgs.at(0), cmdArgs.at(1)); // <====== Execute
					return vector<string>{};
				}
				else {
					std::cout << "Name " << cmdArgs.at(1) << " exists, choose another name ";
					return vector<string>{};
				}

			}
			if (action=="mv") {
				if (!nOT::nUse::useOT.accountCheckIfExists(cmdArgs.at(1))) { // make sure that the name is unique
					nOT::nUse::useOT.accountRename(cmdArgs.at(0), cmdArgs.at(1)); // <====== Execute
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
				nOT::nUtils::DisplayVectorEndl( cout,  nOT::nUse::useOT.assetsGetNames() ) ;
				return vector<string>{};
			}
			else if (action=="issue") {
				nOT::nUtils::DisplayStringEndl( cout, "Please paste a currency contract, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, "SERVER RESPONSE:\n" + nOT::nUse::useOT.assetIssue( nOT::nUse::useOT.serverGetDefault(),nOT::nUse::useOT.nymGetDefault(), nOT::nUtils::GetMultiline() ) );
				return vector<string>{};
			}
			else if (action=="new") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter the XML contents for the contract, followed by an EOF or a ~ by itself on a blank line:");
				const string assetID = nOT::nUse::useOT.assetNew( nOT::nUse::useOT.nymGetDefault(), nOT::nUtils::GetMultiline() );
				nOT::nUtils::DisplayStringEndl( cout, "ASSET ID:\n" + assetID );
				nOT::nUtils::DisplayStringEndl( cout, "SIGNED CONTRACT:\n" + nOT::nUse::useOT.assetGetContract(assetID) );
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
				//return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy() ) ; // TODO nyms autocomplete
				nOT::nUse::useOT.msgGetAll(); // <====== Execute
				return vector<string>{};
			}
			if (action=="mv") {
				return WordsThatMatch(  current_word  ,  vector<string>{"Where-to?"} ); // in mail box... will there be other directories?
			}
			if (action=="rm") { // nym
				return WordsThatMatch(  current_word  , nOT::nUse::useOT.nymsGetMy() );
			}
			if (action=="sendfrom") { // sender name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy() );
			}
			if (action=="sendto") { // recipient name TODO finish sendto functionality
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy() ); //TODO propose nyms from adressbook
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="ls") {
				if (nOT::nUse::useOT.nymCheckByName( cmdArgs.at(0) )) {
					nOT::nUse::useOT.msgGetForNym( cmdArgs.at(0) ); // <====== Execute
					return vector<string>{};
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
			if (action=="rm") { // nym
				if (nOT::nUse::useOT.nymCheckByName(cmdArgs.at(0))) {
					cout << "Index?" << endl;
					return vector<string>{};
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
			if (action=="sendfrom") { // recipient name
				if (nOT::nUse::useOT.nymCheckByName(cmdArgs.at(0))) {
					return WordsThatMatch(  current_word  , nOT::nUse::useOT.nymsGetMy() );
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
		}

		if (full_words<5) { // we work on word5 - var3
			if (action=="sendfrom") { // message text
				if (nOT::nUse::useOT.nymCheckByName(cmdArgs.at(1))) {
					nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of message, followed by an EOF or a ~ by itself on a blank line:" );
					nOT::nUse::useOT.msgSend(cmdArgs.at(0), cmdArgs.at(1), nOT::nUtils::GetMultiline()); // <====== Execute
					return vector<string>{}; // ready for message
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(1);
					return vector<string>{};
				}
			}
			if (action=="rm") { // nym
				if (nOT::nUse::useOT.nymCheckByName(cmdArgs.at(0))) {
					nOT::nUse::useOT.msgInRemoveByIndex( cmdArgs.at(0), std::stoi(cmdArgs.at(1)) );
				}
				else {
					std::cerr << "Can't find that nym: " << cmdArgs.at(0);
					return vector<string>{};
				}
			}
		}

		if (full_words<6) { // we work on word6
			if (action=="sendfrom") { // message text
				nOT::nUse::useOT.msgSend(cmdArgs.at(0), cmdArgs.at(1), cmdArgs.at(2)); // <====== Execute
				return vector<string>{};
			}
		}


	} // msg

	if (topic=="nym") {
		if (full_words<2) { // we work on word2 - the action:
			string defNym = nOT::nUse::useOT.nymGetDefault();
			cout << "Default Nym: " << nOT::nUse::useOT.nymGetName(defNym) + " - " + defNym << endl;// <====== Execute - show active nym
			return WordsThatMatch(  current_word  ,  vector<string>{"check", "edit", "export", "import", "info", "ls", "new", "refresh", "register", "rm", "set-default"} ) ;
		}
		if (full_words<3) { // we work on word3 - cmdArgs.at(0)

			if (action=="check") { // TODO interactive input
				nOT::nUtils::DisplayStringEndl(cout, "Type NymID to check"); // <====== Execute
				return vector<string>{};
			}
			if (action=="edit") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.nymsGetMy() );//TODO Suitable changes to this part - propably after merging with otlib
			}
			if (action=="info") {
				//nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.nymGetInfo(cmdArgs.at(0)) ); // <====== Execute TODO For default nym
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.nymsGetMy() );
			}
			if (action=="ls") {
				nOT::nUtils::DisplayVectorEndl(cout, nOT::nUse::useOT.nymsGetMy(), "\n"); // <====== Execute
				return vector<string>{};
			}
			if (action=="new") {
				nOT::nUtils::DisplayStringEndl(cout, "Type new Nym name"); // <====== Execute
				return vector<string>{};
			}
			if (action=="refresh") {
				nOT::nUse::useOT.nymRefresh(); // <====== Execute
				return vector<string>{};
			}
			if (action=="register") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.nymsGetMy() );//TODO server name
			}
			if (action=="rm") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.nymsGetMy() );//TODO Suitable changes to this part - propably after merging with otlib
			}
			if (action=="set-default") {
				return WordsThatMatch( current_word  ,  nOT::nUse::useOT.nymsGetMy() );
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="check") {
				nOT::nUse::useOT.nymCheck( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="info") {
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.nymGetInfo(cmdArgs.at(0)) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="new") {
				nOT::nUse::useOT.nymCreate( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="register") {
				nOT::nUse::useOT.nymRegister( cmdArgs.at(0) ); // <====== Execute
				return vector<string>{};
			}
			if (action=="set-default") {
				nOT::nUse::useOT.nymSetDefault( cmdArgs.at(0) ); // <====== Execute
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
			string defServer = nOT::nUse::useOT.serverGetDefault();
			cout << "Default server: " << nOT::nUse::useOT.serverGetName(defServer) + " - " + defServer << endl;// <====== Execute - show active server
			return WordsThatMatch(  current_word  ,  vector<string>{"add", "check", "ls", "new", "set-default"} ) ;
		}

		if (full_words<3) { // we work on word3 - var1
			if (action=="add") {
				nOT::nUtils::DisplayStringEndl( cout, "Please paste a server contract, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUse::useOT.serverAdd( nOT::nUtils::GetMultiline() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="check") {
				nOT::nUse::useOT.serverCheck();// <====== Execute
				return vector<string>{};
			}
			if (action=="ls") {

				nOT::nUtils::DisplayVectorEndl(cout, nOT::nUse::useOT.serversGet() ); // <====== Execute
				return vector<string>{};
			}
			if (action=="set-default") {
				return WordsThatMatch(  current_word  , nOT::nUse::useOT.serversGet() );
				return vector<string>{};
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="set-default") {
				nOT::nUse::useOT.serverSetDefault(cmdArgs.at(0)); // <====== Execute
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
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textEncode(nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}

			if (action=="decode") { // text to decode
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be decoded, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textDecode(nOT::nUtils::GetMultiline()) ); // <====== Execute
				return vector<string>{};
			}

			if (action=="encrypt") { // recipient Nym Name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy());
			}

			if (action=="decrypt") { // recipient Nym Name
				return WordsThatMatch(  current_word  ,  nOT::nUse::useOT.nymsGetMy());
			}
		}

		if (full_words<4) { // we work on word4 - var2
			if (action=="encode") {
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textEncode(cmdArgs.at(0))); // <====== Execute
				return vector<string>{};
			}

			if (action=="decode") {
				return WordsThatMatch(  current_word  ,  vector<string>{} ) ; //encoded text will be always multiline
			}

			if (action=="encrypt") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be encrypted, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textEncrypt(cmdArgs.at(0), nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}

			if (action=="decrypt") {
				nOT::nUtils::DisplayStringEndl( cout, "Please enter multiple lines of text to be decrypted, followed by an EOF or a ~ by itself on a blank line:" );
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textDecrypt(cmdArgs.at(0), nOT::nUtils::GetMultiline())); // <====== Execute
				return vector<string>{};
			}
		}

		if (full_words<5) { // we work on word5 - var3
			if (action=="encrypt") { // if plain text is passed as argument (don't implemented for decrypt action because of multiline encrytped block)
				nOT::nUtils::DisplayStringEndl( cout, nOT::nUse::useOT.textEncrypt(cmdArgs.at(0), cmdArgs.at(1))); // <====== Execute
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
	return vector<string>{};
	//throw std::runtime_error("Unable to handle following completion: sofar_str='" + ToStr(sofar_str) + "' in " + OT_CODE_STAMP);
}


// ======================================================================================

cInteractiveShell::cInteractiveShell()
:dbg(false)
{ }

void cInteractiveShell::runOnce(const string line) { // used with bash autocompletion
	gCurrentLogger.setDebugLevel(100);
	nOT::nOTHint::cHintManager hint;
	vector<string> out = hint.AutoCompleteEntire(line);
	nOT::nUtils::DisplayVectorEndl(std::cout, out);
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
// then it should cache possibilities of endings "msg" "mint" "msguard", and return 0th (first) one.
// Next it will be called with other number (probably 1,2,3..) and return N-th possibility.
// Function is non-reentrant also in the meaing that it can not be called in interlace, e.g.
// ("ot m",0) then ("ot m",1) then ("ot x",0) and suddenly back to ("ot x",2) without reinitialization
// (done with number=0) is an error (at least currently, in future we might cache various completion
// arrays, or recalculate on change)


static char* completionReadlineWrapper(const char *sofar , int number) {
	bool dbg = my_rl_wrapper_debug;
	if (dbg) _dbg3("sofar="<<sofar<<" number="<<number<<" rl_line_buffer="<<rl_line_buffer<<endl);
	string line;
	if (rl_line_buffer) line = rl_line_buffer;
	line = line.substr(0, rl_point); // Complete from cursor position
	nOT::nOTHint::cHintManager hint;
	static vector <string> completions;
	if (number == 0) {
		if (dbg) _dbg3("Start autocomplete (during first callback, number="<<number<<")");
		completions = hint.AutoCompleteEntire(line); // <--
		if (dbg)nOT::nUtils::DBGDisplayVectorEndl(completions); //TODO: display in debug
		if (dbg) _dbg3("Done autocomplete (during first callback, number="<<number<<")");
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
	char **matches;
	matches = (char **)NULL;
	matches = rl_completion_matches (text, completionReadlineWrapper);
	return (matches);
}

void cInteractiveShell::runEditline() {
	// nOT::nUse::useOT.Init(); // Init OT on the beginning // disabled to avoid some problems and delay (and valgrid complain)

	char *buf = NULL;
	my_rl_wrapper_debug = dbg;
	rl_attempted_completion_function = completion;
	rl_bind_key('\t',rl_complete);
	while((buf = readline("commandline-part> "))!=NULL) { // <--- readline()
		std::string word;
		if (buf) word=buf; // if not-null buf, then assign
		if (buf) { free(buf); buf=NULL; }
		// do NOT use buf variable below.

		if (dbg) cout << "Word was: " << word << endl;
		std::string cmd;
		if (rl_line_buffer) cmd = rl_line_buffer; // save the full command into string
		cmd = cmd.substr(0, cmd.length()-1); // remove \n

		if (dbg) cout << "Command was: " << cmd << endl;
		auto cmd_trim = nOT::nUtils::trim(cmd);
		if (cmd_trim=="exit") break;
		if (cmd_trim=="quit") break;
		if (cmd_trim=="q") break;

		if (cmd.length()) {
		add_history(cmd.c_str()); // TODO (leaks memory...) but why

		//Execute in BuildTreeOfCommandlines:
		nOT::nOTHint::cHintManager hint;
		hint.AutoCompleteEntire(cmd);
		}
	}
	if (buf) { free(buf); buf=NULL; }
	clear_history(); // http://cnswww.cns.cwru.edu/php/chet/readline/history.html#IDX11
}

}; // namespace nOTHint
}; // namespace nOT
// ########################################################################
// ########################################################################
// ########################################################################

std::string gVar1; // to keep program input argument for testcase_complete_1
// ====================================================================

// #####################################################################

