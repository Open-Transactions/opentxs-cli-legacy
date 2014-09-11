/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "tests.hpp"

#include <base/lib_common2.hpp>

#include <base/othint.hpp>
#include <base/otcli.hpp>

namespace nOT {
namespace nTests {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

using namespace nOT::nUtils;

std::string StreamName(std::ostream &str) {
	//if (str == std::cout) return "cout";
	//if (str == std::cout) return "cin";
	return "other-stream";
}

struct cTestCaseCfg {
	std::ostream &ossErr;
	bool debug;

	cTestCaseCfg(std::ostream &ossErr, bool debug)
	: ossErr(ossErr) , debug(debug)
	{ }

	std::ostream & print(std::ostream &ostr) const { ostr << "[" << (debug ? "debug":"quiet") << " " << StreamName(ossErr) << "]";  return ostr; }

};

std::ostream & operator<<(std::ostream &ostr, const cTestCaseCfg &cfg) { return cfg.print(ostr); }

bool testcase_complete_1(const std::string &sofar); // TODO ... testcase or really used???
bool testcase_complete_1_wrapper(); // TODO ... testcase or really used???

typedef bool ( * tTestCaseFunction )(const cTestCaseCfg &) ;
// ^- tTestCaseFunction is a function:  bool ....(const cTestCaseCfg &)

void exampleOfOT(); // TODO implementation ehre

// ==================================================================
// ==================================================================
bool testcase_namespace_pollution(const cTestCaseCfg &testCfg) {
	class a {
	};

	using namespace nOT::nOTHint;

	#ifdef __unittest_mustfail_compile1
	{
		// using std::string; // without this
		string s; // <-- must be a compile error
	}
	#endif
	{
		using std::string;
		string s; // <-- must work
	}

	return true;
}

bool testcase_cxx11_memory(const cTestCaseCfg &testCfg) {
	using namespace nOT::nNewcli;
	using namespace nOT::nOTHint;

	// TODO capture output and verify expected output

	struct cObj {
			cObj() { /*cout<<"new"<<endl;*/ }
			~cObj() { /*cout<<"delete"<<endl;*/ }
	};

	unique_ptr<cObj> A( new cObj );

	return true;
}

bool testcase_fail1(const cTestCaseCfg &testCfg) {
	if (testCfg.debug) testCfg.ossErr<<"This special testcase will always FAIL, on purpose of testing the testcases framework."<<endl;
	return false;
}

bool testcase_fail2(const cTestCaseCfg &testCfg) {
	if (testCfg.debug) testCfg.ossErr<<"This special testcase will always FAIL, on purpose of testing the testcases framework."<<endl;
	throw std::runtime_error("This test always fails.");
}

bool helper_testcase_run_main_with_arguments(const cTestCaseCfg &testCfg , vector<string> tab ) {
/*
	int argc = tab.size(); // <--
	typedef char * char_p;
	char_p * argv  = new char_p[argc]; // C++ style new[]

	bool dbg = testCfg.debug;   auto &err = testCfg.ossErr;
	if (dbg) err << "Testing " << __FUNCTION__ << " with " << argc << " argument(s): "  << endl;

	size_t nr=0;
	for(auto rec:tab) {
		argv[nr] = strdup(rec.c_str()); // C style strdup/free
		//if( dbg) {err << "argv " << argv[nr];}

		++nr;
		if( dbg) {err << rec; };
	}
	if (dbg) err << endl;

	bool ok=true;
	try {
		ok = main_main(argc, argv) == 0 ; // ... ok? TODO

		if (!ok) err << "BAD TEST " << __FUNCTION__ << " with " << argc << " argument(s): ";
	}
	catch(const std::exception &e) {
		ok=false;
		testCfg.ossErr << "\n *** in " << __FUNCTION__ << " catched exception: " << e.what() << endl;
	}
	for (int i=0; i<argc; ++i) { free( argv[i] ); argv[i]=NULL; } // free!
	delete []argv; argv=nullptr;
	return ok;*/
	_erro("Test case not refactored yet!");
	return false;
}

// Separate functions for failing tests:
bool testcase_run_main_args_fail1(const cTestCaseCfg &testCfg) {
	bool ok=true;
	const string programName="othint";

	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot "} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName} ) ) ok=false;

	return ok;
}

//All this tests should succeed:
bool testcase_run_main_args(const cTestCaseCfg &testCfg) {
	bool ok=true;
	const string programName="othint";

	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account new game\\ toke_ns TEST_CASE"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account rm TEST_CA_SE"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot msg sen"} ) ) ok=false;

	return ok;
}

bool testcase_account(const cTestCaseCfg &testCfg) {
	bool ok=true;
/* TODO: check output
	map<string , vector<string> > const cases {
		 { "ot account ls", { ,  } }
		,{ "ot account new game\\ toke_ns TEST_CASE", { ,  } }
		,{ "ot account mv TEST_CASE TEST_CASE_MOVED", { ,  } }
		,{ "ot account refresh", { ,  } }
		,{ "ot account rm TEST_CASE_MOVED", { ,  } }
	}
	for(auto rec:out)

	nOT::nOTHint::cHintManager hint;
	vector<string> out = hint.AutoCompleteEntire(line);
	int i = 0;
	for(auto rec:out)	{
		out[i] = SpaceFromEscape(rec);
		i++;
	}
	nOT::nUtils::DisplayVectorEndl(std::cout, out);

	*/

	// TODO: This code breaks autocompletion (autocompletion don't see current and previous word and complete with previous word):
	const string programName="othint";
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account new game\\ toke_ns TEST_CASE"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account ls"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account mv"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account refresh"} ) ) ok=false;
	if (!	helper_testcase_run_main_with_arguments(testCfg, vector<string>{programName,"--complete-one", "ot account rm TEST_CA_SE"} ) ) ok=false;
	return ok;
}

bool testcase_run_EscapeString(const cTestCaseCfg &testCfg) {
	bool ok=true;
	std::string test = "TestTest";
	std::string shouldBe = "T\\3stTe\\3t";
	test[1] = 3;
	test[6] = 3;
	std::string out = EscapeString(test);
	if(out!=shouldBe) {
		ok = false;
		if (testCfg.debug)
		testCfg.ossErr<<"Bad Test EscapeString: test string "<<test << " out " << out << " should be " <<shouldBe <<endl;
	}
	return ok;
}

bool testcase_run_all_tests() { // Can only run bool(*)(void) functions (to run more types casting is needed)
	_dbg3("=== test cases, unit tests ============================================");

	long int number_errors = 0; // long :o

	std::ostringstream quiet_oss;

	cTestCaseCfg testCfg(cerr, true);
	cTestCaseCfg testCfgQuiet(quiet_oss, false); // to quiet down the tests

	struct cTestCaseNamed {
		cTestCaseNamed( tTestCaseFunction func  , const string &name, bool expectedOutcome)
		:mFunc(func), mName(name) , mExpectedOutcome(expectedOutcome) // XXX
		{
		}

		tTestCaseFunction mFunc;
		string mName;
		bool mExpectedOutcome; // must succeed or fail?
	};
	vector<cTestCaseNamed> vectorOfFunctions;

	// [stringification], [macro-semicolon-trick]
	#define xstr(s) str(s)
	#define str(s) #s
	#define AddFunction(XXX) do {   vectorOfFunctions.push_back( cTestCaseNamed( & XXX , str(XXX) , true ));   } while(0)
	#define AddFunctionMustFail(XXX) do {   vectorOfFunctions.push_back( cTestCaseNamed( & XXX , str(XXX) , false ));   } while(0)
	AddFunction(testcase_namespace_pollution);
	AddFunction(testcase_cxx11_memory);
	AddFunction(testcase_run_main_args);
	AddFunction(testcase_run_EscapeString);
	//AddFunction(testcase_account);

	AddFunctionMustFail(testcase_fail1); // only for testing of this test code
	AddFunctionMustFail(testcase_fail2); // only for testing of this test code
	AddFunctionMustFail(testcase_run_main_args_fail1);

	#undef AddFunction
	#undef xstr
	#undef str

	std::ostringstream failure_details;

	int nr=0;
	for(auto it = vectorOfFunctions.begin(); it != vectorOfFunctions.end(); ++it) { // Calling all test functions
		const cTestCaseNamed &test = *it;
		bool result = 0;
		string exception_msg;
		try {
			cTestCaseCfg & config = ( (test.mExpectedOutcome == 0) ? testCfgQuiet : testCfg ); // if test should fail - make it quiet
			cerr << "--- start test --- " << test.mName << " (config=" << config << ")" << " \n";
			result = (   test.mFunc   )( config ); // <--- run the test with config
			cerr << "--- done test --- " << test.mName << " \n\n";
		} catch(const std::exception &e) { exception_msg = e.what(); }

		bool as_expected = (result == test.mExpectedOutcome);

		if (!as_expected) {
			number_errors++;
			std::ostringstream msgOss; msgOss << "test #" << nr << " " << test.mName  <<  " failed"
				<< "(result was "<<result<<" instead expected "<<test.mExpectedOutcome<<") ";
			if (exception_msg.size()) msgOss << " [what: " << exception_msg << "]";
			msgOss<<"!";
			string msg = msgOss.str();
			//_dbg3(" *** " << msg);
			failure_details << " " << msg << " ";
		}
		++nr;
	}

	if (number_errors == 0) {
		//cout << "All tests completed successfully." << endl;
	}
	else {
		cerr << "*** Some tests were not completed! (" << failure_details.str() << ")" << endl;
	}
		cerr << "=== test cases, unit tests - done =====================================" << endl;

	return number_errors==0;
}

#if 0
void exampleOfOT() {
	OTAPI_Wrap::AppInit(); // Init OTAPI
	std::cout <<"loading wallet: ";
	if(OTAPI_Wrap::LoadWallet())
	std::cout <<"wallet was loaded "<<std::endl;
	else
	std::cout <<"error while loanding wallet "<<std::endl<<std::endl;

	std::cout <<std::endl<<"account count :"<< OTAPI_Wrap::GetAccountCount()<<std::endl;
	std::cout <<"list of account :"<< std::endl;

	std::string SERVER_ID;
	std::string  USER_ID = "DYEB6U7dcpbwdGrftPnslNKz76BDuBTFAjiAgKaiY2n";

		for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
			std::string ACCOUNT_ID = OTAPI_Wrap::GetAccountWallet_ID (i);
			std::cout <<OTAPI_Wrap::GetAccountWallet_Name ( OTAPI_Wrap::GetAccountWallet_ID (i)	) <<std::endl;

			std::cout << std::endl<<"server count: " << OTAPI_Wrap::GetServerCount () << std::endl;
			std::cout << "list of servers: " << std::endl;

			for(int i = 0 ; i < OTAPI_Wrap::GetServerCount ();i++) {
					SERVER_ID = OTAPI_Wrap::GetServer_ID (i);
					std::string Name = OTAPI_Wrap::GetServer_Name (SERVER_ID);
					std::cout << Name<< "\t\t\tid "<<  SERVER_ID  << std::endl;

					/*std::cout <<"connecting to server: ";
					std::cout <<OTAPI_Wrap::checkServerID(SERVER_ID,USER_ID)<< std::endl;

					std::cout << std::endl<< "asset from server "<<OTAPI_Wrap::getAccount(SERVER_ID, USER_ID,ACCOUNT_ID);
*/
			/*
			std::string  Pubkey_Encryption = OTAPI_Wrap::LoadUserPubkey_Encryption 	( 	USER_ID	);
	bool OTAPI_Wrap::ConnectServer 	( 	ID,USER_ID,
		const std::string &  	strCA_FILE,
		const std::string &  	strKEY_FILE,
		const std::string &  	strKEY_PASSWORD
	) */
			}
		}

	//bool connected  = OTAPI_Wrap::ConnectServer 	( SERVER_ID,USER_ID,strCA_FILE,strKEY_FILE,strKEY_PASSWORD);

	std::cout << std::endl<<"nym count: " << OTAPI_Wrap::GetNymCount () << std::endl;

	std::cout << "list of nyms: " << std::endl;
	for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
			std::string nym_ID = OTAPI_Wrap::GetNym_ID (i);
			std::string nym_Name = OTAPI_Wrap::GetNym_Name (nym_ID);
			std::cout << nym_Name<< "\t\t\tid "<<  nym_ID  << std::endl;

			std::cout <<" inbox mail count for nym:"<<OTAPI_Wrap::GetNym_MailCount(nym_ID) << std::endl;
			for(int i = 0 ; i < OTAPI_Wrap::GetNym_MailCount (nym_ID);i++) {
					std::cout << std::endl<< "inbox mail numer "<< i+1<<std::endl << OTAPI_Wrap::GetNym_MailContentsByIndex (nym_ID,i)<<std::endl;
					}

			std::cout <<" outbox mail count for nym:"<<OTAPI_Wrap::GetNym_OutmailCount(nym_ID) << std::endl;
			for(int i = 0 ; i < OTAPI_Wrap::GetNym_OutmailCount (nym_ID);i++) {
					std::cout << std::endl<< "outbox mail numer "<< i+1<<std::endl << OTAPI_Wrap::GetNym_OutmailContentsByIndex (nym_ID,i)<<std::endl;
					}
			std::cout <<"RevokedCred for nym:"<<OTAPI_Wrap::GetNym_RevokedCredCount(nym_ID) << std::endl;
			for(int i = 0 ; i < OTAPI_Wrap::GetNym_RevokedCredCount (nym_ID);i++) {
					std::cout << std::endl<< "RevokedCred numer "<< i+1<<std::endl << OTAPI_Wrap::GetNym_RevokedCredID (nym_ID,i)<<std::endl;
					}
			std::cout <<"Statistic for nym:" <<OTAPI_Wrap::GetNym_Stats(nym_ID);
			}



		/*std::string ASSET_ID = OTAPI_Wrap::GetAssetType_ID (0);
		//CREATE ACCOUNT
		OTAPI_Wrap::accountCreate(SERVER_ID,USER_ID,ASSET_ID);
	*/

	}

#endif



}; // namespace nTests
}; // namespace nOT


