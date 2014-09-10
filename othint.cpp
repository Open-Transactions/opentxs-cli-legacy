/* See other files here for the LICENCE that applies here. */

#include "lib_common2.hpp"

#include "othint.hpp"
#include "otcli.hpp"

#include "tests.hpp" // TODO Not needed
#include "daemon_tools.hpp"

#ifndef _WIN32
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <thread>

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

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

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

} // nOT

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



// ======================================================================================

shared_ptr<nNewcli::cCmdParser> gReadlineHandleParser;
shared_ptr<nUse::cUseOT> gReadlineHandlerUseOT;

cInteractiveShell::cInteractiveShell()
:dbg(false)
{ }


bool cInteractiveShell::Execute(const string cmd) {
	bool all_ok=false;
	if (cmd.length()) {
		add_history(cmd.c_str()); // TODO (leaks memory...) but why
		write_history("otcli-history.txt"); // Save new history line to file
		try {
			_dbg1("Processing command");
			int offset = 0;
			string cmd_ = nUtils::SpecialFromEscape(cmd,offset);
			auto processing = gReadlineHandleParser->StartProcessing(cmd_, gReadlineHandlerUseOT); // <---
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
	} // length
	return all_ok;
}

void cInteractiveShell::_CompleteOnce(const string line, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	gCurrentLogger.setDebugLevel(100);

	auto parser = make_shared<nNewcli::cCmdParser>();
	gReadlineHandleParser = parser;
	gReadlineHandlerUseOT = use;
	parser->Init();
	vector <string> completions;
	auto processing = gReadlineHandleParser->StartProcessing(line, gReadlineHandlerUseOT);
	completions = processing.UseComplete( line.size() ); // Function gets line before cursor, so we need to complete from the end

	nOT::nUtils::DisplayVectorEndl(std::cout, completions);

	gReadlineHandlerUseOT->CloseApi(); // Close OT_API at the end of shell runtime
}


void cInteractiveShell::CompleteOnceWithDaemon(const string & line) {
	_info("Entering CompleteOnceWithDaemon");

	cDaemoninfoComplete dinfo;

	if (dinfo.IsRunning()) {
		_mark("DAEMON available, will use it");
		string pipe_name = dinfo.GetPathIn();
		dinfo.CreateOut(); // *** prepare OUT file
		string reply_file_choice = dinfo.GetPathOut();
		std::ostringstream oss;  oss << "complete " << reply_file_choice << " " << line << std::endl << std::ends;
		const string request_string = oss.str();
		_mark("STARTER: sending request [" << request_string << "] to pipe " << pipe_name );

		ofstream request_file( pipe_name.c_str()  );
		request_file << request_string << endl;

		_note("Waiting for daemon reply");

		int cycle=0;
		double time_waited=0; const double time_max=5000; // in milliseconds
		while (!dinfo.IsReadyPatchOut()) { ++cycle;
			const double wait = 2;
			time_waited += wait;
			std::this_thread::sleep_for(std::chrono::milliseconds( (int)wait )); // ***
			if (time_waited > time_max) { // timeout
				ostringstream oss;
				oss << "Timeout in starter while waiting for daemon reply"
					<< " after " << time_waited <<  " ms " << " in " << cycle << " cycles" << ends;
				const string ERR=oss.str();
				_erro(ERR); throw std::runtime_error(ERR);
			}
		} // wait for daemon
		_note("Done waiting in starter while waiting for daemon reply"
				<< " after " << time_waited <<  " ms " << " in " << cycle << " cycles" );

		ifstream response_file( dinfo.GetPathOut() );
		vector<string> response;
		while (response_file.good()) {
			if (response_file.eof()) break;
			string word;
			response_file >> word;
			response.push_back(word);
		}
		_note("Ready reply from daemon: " << DbgVector(response));

		for (auto word : response) {
			cout << word << " ";
		}
		cout << endl;

		_mark("STARTER: I'm done");
	}
	else {
		// we will become the daemon then... (well, we will fork here)
		_mark("DAEMON IS NOT YET RUNNIG - WILL START IT");


		_fact("I will fork here");
		pid_t pid = fork(); // <--- *** *** FORK *** ***
		if (!pid) {
			gCurrentLogger.setOutStreamFile("daemon.log");
			_fact("=== Daemon log started ===");
		}
		_fact("After fork, fork-pid = " << pid);

		if (pid) { // fork: I am the parent - I will execute first call of completion
			long int cycle=0;
			double time_waited=0; const double time_max=1000; // in milliseconds
			while (!dinfo.IsRunning()) { ++cycle;
				const double wait = 2;
				time_waited += wait;
				std::this_thread::sleep_for(std::chrono::milliseconds( (int)wait ));
				if (time_waited > time_max) { // timeout
					ostringstream oss;
					oss << "Timeout in starter (first starter) while waiting for the child daemon that we just started to become ready"
						<< " after " << time_waited <<  " ms " << " in " << cycle << " cycles" << ends;
					const string ERR=oss.str();
					_erro(ERR); throw std::runtime_error(ERR);
				}
			} // wait for daemon
			_note("Done waiting in starter (first starter) while waiting for the child daemon that we just started to become ready"
					<< " after " << time_waited <<  " ms " << " in " << cycle << " cycles" );

			CompleteOnceWithDaemon(line); // *** again use self (recurency) <--- RECURSION ***
		} // the parent
		else
		{ // fork: I am the child - I will become daemon
			_fact("daemon()");
			int daemon_err = daemon(1,1); // ***
			if (daemon_err)  { const string ERR="Daemon failed"; _erro(ERR); throw std::runtime_error(ERR); }
			_fact("daemon() done");

			// preparing OT variables etc:
			auto useOT = std::make_shared<nUse::cUseOT>("Daemon-Completion");
			auto parser = make_shared<nNewcli::cCmdParser>();
			gReadlineHandleParser = parser;
			gReadlineHandlerUseOT = useOT;
			parser->Init();

			// prepare named pipe file
			string pipe_name = dinfo.GetPathIn();

			_note("Will create the pipe (fifo) to listen on, as " << pipe_name);
			mkfifo(pipe_name.c_str(), 0600);
			_dbg1("Will open the pipe to listen on, as " << pipe_name);
			FILE * pipe_file = fopen( pipe_name.c_str() , "r");
			if (pipe_file == NULL) { const string ERR="Pipe failed"; _erro(ERR); throw std::runtime_error(ERR); }
			_dbg1("Pipe opened, on pipe_file="<<(void*)pipe_file);

			bool finished=false;
			while (!finished) { // read all requests in loop
				_dbg1("Reading from pipe");
				const size_t buff_size = 8192;
				char buff[ buff_size ];

				// wait for reading something
				float sleep_size=25;
				float const sleep_inc=2, sleep_limit1=60, sleep_limit2=200, sleep_limit3=300, sleep_eff1=0.2, sleep_eff2=0.01;
				bool read_something=false;
				long int cycle=0;
				do {
					++cycle;
					char * read_status = fgets( buff , buff_size , pipe_file );
					if (read_status == NULL) {
						auto sleep_add = sleep_inc;
						if (sleep_size>sleep_limit1) sleep_add = sleep_inc * sleep_eff1;
						if (sleep_size>sleep_limit2) sleep_add = sleep_inc * sleep_eff2;
						sleep_size += sleep_add;
						sleep_size = std::min(sleep_size, sleep_limit3);
						if ((cycle % 100)==0) _dbg1("Daemon waiting for input, cycle="<<cycle<<", now sleeping for " << (int)sleep_size << " ms" );
						std::this_thread::sleep_for(std::chrono::milliseconds( (int)sleep_size ));
					} else read_something=true;
				} while (!read_something);
				string pipe_command( buff );

				bool found_nl=false;
				if (pipe_command.size()) { // >0 size
					if (*(pipe_command.end()-1) == '\n') { // ends with \n
						found_nl=true;
						pipe_command = pipe_command.substr(0, pipe_command.size()-1);
					}
					if (!found_nl) _warn("No new line");
				}

				_info("Read from pipe: [" << ToStr(pipe_command) << "] " << (found_nl ? "found-NL" : "no-NL?" ) );
				if (pipe_command=="QUIT") { _fact("Read QUIT (1)"); finished=true;  continue; }

				// TODO SECURITY verify that this parses correctly (no UB / mem errors in string) on mallformed strings! TODO XXX
				size_t sep1 = pipe_command.find(' ',0);
				size_t sep2 = pipe_command.find(' ',sep1+1);
				const string request_command = pipe_command.substr(0, sep1);
				const string request_output_name = pipe_command.substr(sep1+1, sep2-sep1-1);
				const string request_data = pipe_command.substr(sep2+1);

				const string request_output_name_with_flag = request_output_name + ".ready";

				_note("Daemon: got request: " << request_command<<";"<<request_output_name<<";"<<request_data<<";");

				// *** work on the REQUEST here:

				if (request_command == "complete") {
					// CompleteOnce(request_data, useOT);
					const string & line = request_data;

					vector <string> completions;
					auto processing = gReadlineHandleParser->StartProcessing(line, gReadlineHandlerUseOT);
					completions = processing.UseComplete( line.size() ); // Function gets line before cursor, so we need to complete from the end
					_info("Daemon: I generated completions: " << DbgVector(completions));

					// TODO XXX verify if file name begins with safe path intended for OT daemon
					{
						ofstream reply_file( request_output_name.c_str() );
						nOT::nUtils::DisplayVectorEndl( reply_file , completions); // write to file
						reply_file.close();

						ofstream reply_flag_file( request_output_name_with_flag.c_str());
						reply_flag_file.close();
					}
					_info("Written the reply, with completions count " << completions.size() << " to file " << request_output_name );
				} // request
				else if (request_command == "execute") {
					_warn("NOT IMPLEMENTED YET ("<<request_command<<")");
				}
				else {
					_warn("Invalid request command for daemon ("<<request_command<<")");
				}

			} // untill finish
			_mark("DONE reading commands as daemon.");
		}
	}
}

void cInteractiveShell::CompleteOnce(const string line, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	try {
		_CompleteOnce(line, use);
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cInteractiveShell::_RunOnce(const string cmd, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	gCurrentLogger.setDebugLevel(100);

	auto parser = make_shared<nNewcli::cCmdParser>();
	gReadlineHandleParser = parser;
	gReadlineHandlerUseOT = use;
	parser->Init();

	bool all_ok = Execute(cmd); // <---

	if (!all_ok) { // if there was a problem
//			if ((!said_help) || (!(help_needed % opt_repeat_help_each_nth_time))) { cerr<<"If lost, type command 'ot help'."<<endl; ++said_help; }
//			++help_needed;
	}
	gReadlineHandlerUseOT->CloseApi(); // Close OT_API at the end of shell runtime
}

void cInteractiveShell::RunOnce(const string line, shared_ptr<nUse::cUseOT> use) { // used with bash autocompletion
	try {
		_RunOnce(line, use);
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
	dbg=false || 1; // XXX
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
	char **matches;
	matches = (char **)NULL;
	matches = rl_completion_matches (text, CompletionReadlineWrapper);
	rl_attempted_completion_function = completion;
	rl_completer_quote_characters = "\"";
	if (gReadlineHandleParser->mEnableFilenameCompletion) {
		rl_attempted_completion_over = 0;
		gReadlineHandleParser->mEnableFilenameCompletion = false;
	}
	else {
		rl_attempted_completion_over = 1;
	}
	return (matches);
}

void cInteractiveShell::RunEditline(shared_ptr<nUse::cUseOT> use) {
	try {
		_RunEditline(use);

	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}

void cInteractiveShell::_RunEditline(shared_ptr<nUse::cUseOT> use) {
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

			bool all_ok = Execute(cmd); // <---

		} // try an editline turn
		catch (const std::exception &e) {
			cerr << "Problem while reading your command: " << e.what() << endl;
			_erro("Error while reading command: " << e.what() );
		}
	} // while
	// history_truncate_file not available in 2.11-20080614-5 and in wineditline?
	// int maxHistory = 100; //TODO move this to settings
	// history_truncate_file("otcli-history.txt", maxHistory);
	if (buf) { free(buf); buf=NULL; }
	clear_history(); // http://cnswww.cns.cwru.edu/php/chet/readline/history.html#IDX11

	gReadlineHandlerUseOT->CloseApi(); // Close OT_API at the end of shell runtime
}

} // namespace nOTHint
} // namespace nOT
// ########################################################################
// ########################################################################
// ########################################################################

std::string gVar1; // to keep program input argument for testcase_complete_1
// ====================================================================

// #####################################################################

