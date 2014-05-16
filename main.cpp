/* See other files here for the LICENCE that applies here. */

#include "lib_common1.hpp"
#include "runoptions.hpp"

#include "otcli.hpp"

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1

int main(int argc, const char **argv) {
	int ret=0;

	try {
		ret=1; // if aborted then this indicated error
		nOT::nNewcli::cOTCli application;


		vector<string> args_full;	// eg: argv_full: ot +debug +debugfile msg sendfrom rafal piotr +thisisplussign
		for (int i=0; i<argc; ++i) {
			args_full.push_back( argv[i] );
		}

		vector<string> args_clear = nOT::gRunOptions.ExecuteRunoptionsAndRemoveThem(args_full); 
		// eg: argv_minus: ot msg sendfrom rafal piotr +thisisplussign 
		// +debug +debugfile <-- this will be Executed by gRunOptions

		ret = application.Run(args_clear);
	}
	catch (const std::exception &e) {
  	_erro("\n*** Captured exception:" << e.what());
	}
	catch (...) {
  	_erro("\n*** Captured UNKNOWN exception:");
	}

	// nOT::nTests::exampleOfOT(); // TODO from script
	// nOT::nTests::testcase_run_all_tests(); // TODO from script

	return ret;
}

