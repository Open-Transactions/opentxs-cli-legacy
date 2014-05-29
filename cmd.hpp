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

/**
The parser (can be used many times), that should contain some tree of possible commands and format/validation/hint of each.
*/
class cCmdParser : public enable_shared_from_this<cCmdParser> {
	protected:
		unique_ptr< cCmdParser_pimpl > mI;
		void AddFormat( const cCmdName &name, const cCmdFormat &format );

	public:
		cCmdParser();

		cCmdProcessing StartProcessing(const vector<string> &words);
		cCmdProcessing StartProcessing(const string &words);

		void Init();
		void Test();
};

/**
Particular instance of process of parsing one input. 
E.g. parsing the input "msg sendfrom rafal dorota 5000" and pointing to standard parser
*/
class cCmdProcessing {
	protected:
		shared_ptr<cCmdParser> parser;
		vector<string> commandLine;
	public:
		cCmdProcessing(shared_ptr<cCmdParser> _parser, vector<string> _commandLine);

		void Parse();

		vector<string> UseComplete( nUse::cUseOT &use );
		void UseExecute( nUse::cUseOT &use );
};

/** 
A function to be executed that will do some actuall OT call
*/
class cCmdExecutable {  
	public:
		typedef std::function< void (void) > tFunc;
	private:
		tFunc mFunc;
	public:
		cCmdExecutable( tFunc func );
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
		tOption mOption, mOptionExt;

		cCmdExecutable mExec;

	public:
		cCmdFormat(cCmdExecutable exec, tVar var);
};

/**
The parsed and interpreted data of command, arguments and options are ready in containers etc.
*/
class cCmdData {
	public:
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
};


/**
Info about Parameter: How to validate and how to complete this argument
*/
class cParamInfo {
	public:
		typedef function< bool ( cCmdData , int  ) > tFuncValid;
		typedef function< vector<string> ( cCmdData , int  ) > tFuncHint;

	protected:
		tFuncValid funcValid;
		tFuncHint funcHint;

	public:
		cParamInfo()=default;
		cParamInfo(tFuncValid valid, tFuncHint hint=nullptr);
};


void cmd_test();




}; // namespace nNewcli
}; // namespace nOT


#endif

