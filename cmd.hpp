/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_cmd
#define INCLUDE_OT_NEWCLI_cmd

#include "lib_common1.hpp"

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
	public:
		cCmdParser()=default;

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
};

/**
Describes template how given command arguments should look like (validation, hint)
*/
class cCmdFormat {
	protected:
		vector<cParamInfo> var;
		vector<cParamInfo> varExt;
		vector<cParamInfo> option;
		vector<cParamInfo> optionExt;

	public:
		
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
	public:
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

