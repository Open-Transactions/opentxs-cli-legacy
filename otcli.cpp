/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "otcli.hpp"
#include "othint.hpp"
#include "cmd.hpp"

#include "lib_common1.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

int cOTCli::Run(const vector<string> args_without_programname) { 
	LoadScript("autostart-dev.local", "autostart script"); // todo depending on execution mode? +devel ?

	if (nOT::gRunOptions.getDoRunDebugshow() || 0) {
		_note("Running test Debugshow:");
		string msg="Testing debug system.";
		_erro(msg);
		_warn(msg);
		_mark(msg);
		_note(msg);
		_info(msg);
		_dbg1(msg);
		_dbg2(msg);
		_dbg3(msg);
	}

	auto args = args_without_programname;

	int status = 0;
	if (args.size() == 0) {
		throw std::runtime_error("Main program called with 0 arguments (not even program name).");
	}

	size_t nr=0;
	for(auto arg: args) {
		if (arg=="--complete-shell") {
			nOT::nOTHint::cInteractiveShell shell;

			switch ( gRunOptions.getTRunMode() ){
				case gRunOptions.eRunModeNormal :
					shell.runEditline();
					break;
				case gRunOptions.eRunModeDemo :
					shell.runEditline();
					break;
				case gRunOptions.eRunModeCurrent :
					using namespace nOT::nNewcli;

					string input = "msg sendfrom rafal";

					shared_ptr<cCmdParser> newcli(new cCmdParser);
					newcli->Test();
					newcli->StartProcessing(input);
					//cCmdProcessing proc = newcli.StartProcessing(input);
					break;
			}
		}
		else if (arg=="--complete-one") { // otcli "--complete-one" "ot msg sendfr"
			string v;  bool ok=1;  try { v=args.at(nr+1); } catch(...) { ok=0; } //
			if (ok) {
				nOT::nOTHint::cInteractiveShell shell;
				shell.runOnce(v);
			}
			else {
				_erro("Missing variables for command line argument '"<<arg<<"'");
				status = 1;
			}
		}
		++nr;
	}

	_note("Exiting application with status="<<status);
	return status;
}

bool cOTCli::LoadScript_Main(const std::string &thefile_name) { 
	using std::string;
	std::string cmd="";
	std::ifstream thefile( thefile_name.c_str() );
	bool anything=false; // did we run any speciall test
	bool force_continue=false; // should we continue to main program forcefully
	bool read_anything=false;
	while (  (!thefile.eof()) && (thefile.good())  ) {
		getline(thefile, cmd);
		_dbg2("cmd="+cmd);
		if (!read_anything) { read_anything=true; _dbg1("Started reading data from "+thefile_name); }
		if (cmd=="quit") {
			_note("COMMAND: "<<cmd<<" - QUIT");
			return false;
		}
		else if (cmd=="tree") {
			_note("Will test new functions and exit");
			_note("TTTTTTTT");
			nOT::nOTHint::cHintManager hint;
			hint.TestNewFunction_Tree();
			_note("That is all, goodby");
		}
		else if ((cmd=="hello")) {
			_note("COMMAND: Hello world.");
		}
		else if ((cmd=="continue")||(cmd=="cont")) {
			_dbg1("Will continue");
			force_continue = true;
		}
	} // entire file
	bool will_continue = (!anything) || force_continue ;
	if (!will_continue) _note("Will exit then.");
	return will_continue;
} // LocalDeveloperCommands()

void cOTCli::LoadScript(const std::string &script_filename, const std::string &title) {
	_note("Script " + script_filename + " ("+title+")");
	try {
		LoadScript_Main(script_filename);
	}
	catch(const std::exception &e) {
		_erro("\n*** In SCRIPT "+script_filename+" got an exception: " << e.what());
	}
	catch(...) {
		_erro("\n*** In SCRIPT "+script_filename+" got an UNKNOWN exception!");
	}
}


}; // namespace nNewcli
}; // namespace OT


