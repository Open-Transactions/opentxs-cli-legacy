/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_cmd
#define INCLUDE_OT_NEWCLI_cmd

#include "lib_common1.hpp"

#include "useot.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

class cCmdParser;
class cCmdProcessing;
class cCmdFormat;
class cCmdData;

class cCmdName;
class cParamInfo;
class cParamString;

class cCmdParser_pimpl;


struct cErrCommandNotFound : public std::runtime_error { cErrCommandNotFound(const string &s) : runtime_error(s) { } };

struct cErrArgNotFound : public std::runtime_error { cErrArgNotFound(const string &s) : runtime_error(s) { } }; // generally this arg was not found
struct cErrArgMissing : public cErrArgNotFound { 
	cErrArgMissing(const string &s) : cErrArgNotFound("Just missing : " + s) { } }; // more specificaly, the arg was not given, e.g. 3 out of 2
struct cErrArgIllegal : public cErrArgNotFound { 
	cErrArgIllegal(const string &s) : cErrArgNotFound("Illegal! : " + s) { } }; // more specificaly, such arg is illegal, e.g. number -1 or option name ""

/**
The parser (can be used many times), that should contain some tree of possible commands and format/validation/hint of each.
*/
class cCmdParser : public enable_shared_from_this<cCmdParser> {
	protected:
		unique_ptr< cCmdParser_pimpl > mI;
		void AddFormat( const cCmdName &name, shared_ptr<cCmdFormat> format);

	public:
		cCmdParser();

		cCmdProcessing StartProcessing(const vector<string> &words, shared_ptr<nUse::cUseOT> use );
		cCmdProcessing StartProcessing(const string &words, shared_ptr<nUse::cUseOT> use );

		shared_ptr<cCmdFormat> FindFormat( const cCmdName &name ) throw(cErrCommandNotFound);

		void Init();
		void Test();
};

/**
Particular instance of process of parsing one input. 
E.g. parsing the input "msg sendfrom rafal dorota 5000" and pointing to standard parser
*/
class cCmdProcessing {
	protected:
		shared_ptr<cCmdParser> mParser; // our "parent" parser to use here

		vector<string> mCommandLine; // the words of command to be parsed

		shared_ptr<cCmdData> mData; // our parsed command as data; NULL if error/invalid
		shared_ptr<cCmdFormat> mFormat; // the selected CmdFormat template; NULL if error/invalid

		shared_ptr<nUse::cUseOT> &mUse; // this will be used e.g. in Parse() - passed to called validations, in UseExecute and UseComplete etc

	public:
		cCmdProcessing(shared_ptr<cCmdParser> parser, vector<string> commandLine, shared_ptr<nUse::cUseOT> &use );

		void Parse(); // parse into mData, mFormat

		vector<string> UseComplete(); 
		void UseExecute();
};

/** 
A function to be executed that will do some actuall OT call <- e.g. execute this on "ot msg ls"
*/
class cCmdExecutable {  
	public:
		typedef int tExitCode;
		typedef std::function< tExitCode ( shared_ptr<cCmdData> , nUse::cUseOT & ) > tFunc;
	public:
		const static cCmdExecutable::tExitCode sSuccess;
	private:
		tFunc mFunc;
	public:
		cCmdExecutable( tFunc func );

		tExitCode operator()( shared_ptr<cCmdData> , nUse::cUseOT & use );
};

/**
Describes template how given command arguments should look like (validation, hint)
*/
class cCmdFormat {
	public:
		typedef vector<cParamInfo> tVar;
		typedef map<string, cParamInfo> tOption;
		
	protected:
		tVar mVar, mVarExt;
		tOption mOption;

		cCmdExecutable mExec;

	public:
		cCmdFormat(cCmdExecutable exec, tVar var, tVar varExt, tOption opt);

		cCmdExecutable getExec() const;
};

/**
The parsed and interpreted data of command, arguments and options are ready in containers etc.
*/
class cCmdData {
	public:
		typedef vector<string> tVar;
		typedef map<string, vector<string> > tOption; // even single (not-multi) options will be placed in vector (1-element)
		
	protected:
		tVar mVar, mVarExt;
		tOption mOption;

		friend class cCmdProcessing; // it will fill-in this class fields directly

		// [nr] REMARK: the argument "nr" is indexed like 1,2,3,4 (not from 0) and is including both arg and argExt.

		string VarAccess(int nr, const string &def, bool doThrow) const throw(cErrArgNotFound); // see [nr] ; if doThrow then will throw on missing var, else returns def

	public:
		cCmdData()=default;

		/** USE CASES:
		for options: --cc eve --cc mark --bcc zoidberg
			Opt("--cc") returns {"eve","mark"}
			Opt("--subject") throws exception
			OptIf(--subject") returns {}
		for variables: alice bob
		  Var(1) returns "alice"
			Var(3) throws exception
			VarDef(3) returns "" and VarDef(3,"unknown") returns "unknown"


		Exceptions: please note, that the Var, Opt are throwing when the argument is not found normally, e.g. var nr=3 was requested but just 2 are present
		the VarDef and OptIf avoid throwing usually - but they might throw cErrArgIllegal if the requested argument not just is not present but is totally illegal and can not be
		ever present, e.g. if requestion var number -1 or option named "" or other illegal operation (so in programming error usually)
		*/

		string VarDef(int nr, const string &def="",  bool doThrow=0) const throw(cErrArgIllegal); // see [nr] ; return def if this var was missing
		vector<string> OptIf(const string& name) const throw(cErrArgIllegal); // returns option values, or empty vector if missing (if none)

		string Var(int nr) const throw(cErrArgNotFound); // see [nr] ; throws if this var was missing
		vector<string> Opt(const string& name) const throw(cErrArgNotFound); // returns option values, throws if missing (if none)
}; 

// ============================================================================

/**
Name of command like "msg sendfrom"
*/
class cCmdName {
	protected:
		string mName;
	public:
		cCmdName(const string &name);
		
		bool operator<(const cCmdName &other) const;
		operator std::string() const;	
};


/**
Info about Parameter: How to validate and how to complete this argument
*/
class cParamInfo {
	public:
		typedef function< bool ( nUse::cUseOT &, cCmdData &, int, const string &  ) > tFuncValid;
		typedef function< vector<string> ( nUse::cUseOT &, cCmdData &, int, const string &  ) > tFuncHint;

	protected:
		tFuncValid funcValid;
		tFuncHint funcHint;

	public:
		cParamInfo()=default;
		cParamInfo(tFuncValid valid, tFuncHint hint);
};


void cmd_test( shared_ptr<nUse::cUseOT> useOT );




}; // namespace nNewcli
}; // namespace nOT


#endif

