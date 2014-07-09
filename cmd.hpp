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

class cParseEntity;

class cCmdParser;
class cCmdProcessing;
class cCmdFormat;
class cCmdData;
class cCmdDataParse;

class cCmdName;
class cParamInfo;
class cParamString;

class cCmdExecutable;

class cCmdParser_pimpl;

// ============================================================================

struct cErrParse : public std::runtime_error { cErrParse(const string &s) : runtime_error(s) { } };
struct cErrAfterparse : public std::runtime_error { cErrAfterparse(const string &s) : runtime_error("problem in later parsing/using:" + s) { } };
struct cErrParseName : public cErrParse { cErrParseName(const string &s) : cErrParse("name of command is unknown: "+s) { } };
struct cErrParseSyntax : public cErrParse { cErrParseSyntax(const string &s) : cErrParse("syntax error: "+s) { } };

struct cErrArgNotFound : public std::runtime_error { cErrArgNotFound(const string &s) : runtime_error(s) { } }; // generally this arg was not found
struct cErrArgMissing : public cErrArgNotFound {
	cErrArgMissing(const string &s) : cErrArgNotFound("Just missing : " + s) { } }; // more specificaly, the arg was not given, e.g. 3 out of 2
struct cErrArgIllegal : public cErrArgNotFound {
	cErrArgIllegal(const string &s) : cErrArgNotFound("Illegal! : " + s) { } }; // more specificaly, such arg is illegal, e.g. number -1 or option name ""

struct cErrInternalParse : public std::runtime_error { cErrInternalParse(const string &s) : runtime_error("Assert related to parsing: " + s) { } }; // some assert/internal problem related to parsing

// ============================================================================

class cParseEntity {
	public:
		// int mWordNr; // then number of word (in command line)  (this is part of the array index)

		int mCharPos; // at which character does this word start in command line (e.g. in orginal command)

		int mSub; // additional number - e.g. number of word in command or argument; see tKind

		enum class tKind {
			unknown, // yet-unknown. but mWordNr is valid here
			pre, // "ot", mWordNr is 0 usually
			cmdname, // "msg" or "sendfrom", mWordNr=1,2,3,... usually; mSub=1,2 number of part of command name: word1, word2
			variable, // positional argument; mSub == arg_nr (numbered from 1)
			variable_ext, // same, but the extra (optional, not required) variables
			argument_somekind, // some variable or option, not decided yet (used internally e.g. by completion code on new word)
			option_name, // an option, the name part; mSub is the occurance of the option, e.g. mSub==3 for 4th use of --x in "--x --x --x --x"
			option_value, // an option, the value part; mSub is same as for corresponding option_name  --color red  --color green
			fake_empty // empty word e.g. at end of string (when user is completing)
		};

		tKind mKind;

		cParseEntity(tKind mKind, int mCharPos, int mSub=0) : mKind(mKind), mCharPos(mCharPos), mSub(mSub) { }

		// to define the entities meaning later once we know them:
		void SetKind(tKind kind) { mKind=kind; }
		void SetKind(tKind kind, int sub) { mKind=kind; mSub=sub; }

		// usefull for RangesFindPosition:
		bool operator<(const cParseEntity & other) const { return mCharPos < other.mCharPos; }
		bool operator>(const cParseEntity & other) const { return mCharPos > other.mCharPos; }
		bool operator<=(const cParseEntity & other) const { return mCharPos <= other.mCharPos; }
		bool operator>=(const cParseEntity & other) const { return mCharPos >= other.mCharPos; }
		operator int() const { return mCharPos; }

		const char * KindIcon() const {
			switch (mKind) {
				case tKind::unknown: return "?";
				case tKind::pre: return "P";
				case tKind::cmdname: return "C";
				case tKind::argument_somekind: return "v*";
				case tKind::variable: return "V";
				case tKind::variable_ext: return "ve";
				case tKind::option_name: return "on";
				case tKind::option_value: return "ov";
				case tKind::fake_empty: return "+";
			}
			string msg="Unexpected type of Kind!";
			_erro(msg);
			throw std::runtime_error("msg");
			return "!";
		}
};

ostream& operator<<(ostream &stream , const cParseEntity & obj);

// ============================================================================

/**
A function to be executed that will do some actuall OT call <- e.g. execute this on "ot msg ls"
*/
class cCmdExecutable {  MAKE_CLASS_NAME("cCmdExecutable");
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
The parser (can be used many times), that should contain some tree of possible commands and format/validation/hint of each.
*/
class cCmdParser : public enable_shared_from_this<cCmdParser> { MAKE_CLASS_NAME("cCmdParser");
	protected:
		unique_ptr< cCmdParser_pimpl > mI;
		void _AddFormat( const cCmdName &name, shared_ptr<cCmdFormat> format); // warning, will not do certain things like adding common defaults

		void AddFormat(
			const string &name,
			const vector<cParamInfo> &var,
			const vector<cParamInfo> &varExt,
			const map<string, cParamInfo> &opt,
			const cCmdExecutable::tFunc &exec)
			;

		static const vector<string> mNoWords; // this vector represents lack of any words, e.g. for GetCmdNamesWord2

	public:
		cCmdParser();
		~cCmdParser(); // let's instantize default destr in all TUs so compilation will fail (without this line) on unique_ptr on not-complete types trololo - B. Stroustrup

	//	cCmdProcessing StartProcessing(const vector<string> &words, shared_ptr<nUse::cUseOT> use );
		cCmdProcessing StartProcessing(const string &words, shared_ptr<nUse::cUseOT> use );

		shared_ptr<cCmdFormat> FindFormat( const cCmdName &name ) const throw(cErrParseName);
		bool FindFormatExists( const cCmdName &name ) const throw();

		void Init();
		void Test();

		void PrintUsage() const;
		void PrintUsageCommand(const string &cmdname) const;

		vector<string> EndingCmdNames (const string sofar);
		static void _cmd_test_completion(  shared_ptr<nUse::cUseOT> use  );
		static void _cmd_test_tree(  shared_ptr<nUse::cUseOT> use  );
		static void _cmd_test(  shared_ptr<nUse::cUseOT> use  );
		static void cmd_test( shared_ptr<nUse::cUseOT> use );
		static void cmd_test_EndingCmdNames(  shared_ptr<nUse::cUseOT> use  );
		static void _cmd_test_safe_completion(  shared_ptr<nUse::cUseOT> use  );
		static void _cmd_test_completion_answers( shared_ptr<nUse::cUseOT> use  );
		static void _parse_test( shared_ptr<nUse::cUseOT> use);

		const vector<string> & GetCmdNamesWord1() const; // possible word1 in loaded command names
		const vector<string> & GetCmdNamesWord2(const string &word1) const; // possible word2 for given word1 in loaded command names
};

/**
Particular instance of process of parsing one input.
E.g. parsing the input "msg sendfrom rafal dorota 5000" and pointing to standard parser
*/
class cCmdProcessing : public enable_shared_from_this<cCmdProcessing> { MAKE_CLASS_NAME("cCmdProcessing");
	protected:
		enum class tState { never=0, failed, succeeded, succeeded_partial  } ;
		tState mStateParse, mStateValidate, mStateExecute;

		bool mFailedAfterBadCmdname; // did we given up in parsing after seeig bad cmd name (usefull for completion)

		shared_ptr<cCmdParser> mParser; // our "parent" parser to use here

		string mCommandLineString; // as the string from user e.g. "ot    msg sendfrom 'alice'"
		vector<string> mCommandLine; // the words of command to be parsed; Warning: set only from Parse() due to character-spaces etc processing

		shared_ptr<cCmdDataParse> mData; // our parsed command as data; NULL if error/invalid
		shared_ptr<cCmdFormat> mFormat; // the selected CmdFormat template; NULL if error/invalid

		shared_ptr<nUse::cUseOT> mUse; // this will be used e.g. in Parse() - passed to called validations, in UseExecute and UseComplete etc

		virtual void _Parse(bool allowBadCmdname ); // throw if failed;  allowBadCmdname - will exit early if cmdname is not complete (to use from completion of cmdname)
		virtual void _Validate(); // throw if failed
		virtual void _UseExecute(); // throw if failed

	public:
		cCmdProcessing(shared_ptr<cCmdParser> parser, const string &commandLineString, shared_ptr<nUse::cUseOT> use );
		virtual ~cCmdProcessing();

		virtual void Parse(bool allowBadCmdname=false); // parse into mData, mFormat
		virtual void Validate(); // detects validation errors; Might report the error (or maybe throw or save status in *this, depending on this->mUse settings)
		virtual void UseExecute(); // execute the command

		vector<string> UseComplete(int char_pos); // hint the possible completions (aka tab-completion)
		shared_ptr<cCmdDataParse> getmData(){
			return mData;
		}
};

/**
Describes template how given command arguments should look like (validation, hint)
*/
class cCmdFormat {  MAKE_CLASS_NAME("cCmdFormat");
	public:
		typedef vector<cParamInfo> tVar;
		typedef map<string, cParamInfo> tOption;

	protected:
		tVar mVar, mVarExt;
		tOption mOption;

		cCmdExecutable mExec;

		friend class cCmdProcessing; // allow direct access (should be read-only!)

	public:
		cCmdFormat(const cCmdExecutable &exec, const tVar &var, const tVar &varExt, const tOption &opt);
		bool IsValid() const;

		cCmdExecutable getExec() const;

		void Debug() const;
		void PrintUsageShort(ostream &out) const;
		void PrintUsageLong(ostream &out) const;

		vector<string> GetPossibleOptionNames() const;

		size_t SizeAllVar() const ; // return size of required mVar + optional mVarExt

		cParamInfo GetParamInfo(int nr) const;
};

/**
The parsed and interpreted data of command, arguments and options are ready in containers etc.
*/
class cCmdData {  MAKE_CLASS_NAME("cCmdData");
	public:
		typedef vector<string> tVar;
		typedef map<string, vector<string> > tOption; // even single (not-multi) options will be placed in vector (1-element)

	protected:

		friend class cCmdProcessing; // it will fill-in this class fields directly
		tVar mVar, mVarExt;
		tOption mOption;

		void AddOpt(const string &name, const string &value) throw(cErrArgIllegal); // append an option with value (value can be empty)

		// [nr] REMARK: the argument "nr" is indexed like 1,2,3,4 (not from 0) and is including both arg and argExt.

		string VarAccess(int nr, const string &def, bool doThrow) const throw(cErrArgNotFound); // see [nr] ; if doThrow then will throw on missing var, else returns def

	public:
		cCmdData()=default;
		virtual ~cCmdData()=default;

		/** USE CASES:
		for options: --cc eve --cc mark --bcc zoidberg --pivate
			Opt("--cc") returns {"eve","mark"}
			Opt("--subject") throws exception
			OptIf(--subject") returns {}
			IsOpt("--private") returns true, while IsOpt("--cc") is true too, and IsOpt("--foobar") is false
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
		string Opt1If(const string& name, const string &def="") const throw(cErrArgIllegal); // same but requires the 1st element; therefore we need def argument again

		string Var(int nr) const throw(cErrArgNotFound); // see [nr] ; throws if this var was missing
		vector<string> Opt(const string& name) const throw(cErrArgNotFound); // --cc bob --bob alice returns option values, throws if missing (if none)
		string Opt1(const string& name) const throw(cErrArgNotFound); // --prio 100 same but requires the 1st element

		bool IsOpt(const string &name) const throw(cErrArgIllegal); // --dryrun

		size_t SizeAllVar() const ; // return size of required mVar + optional mVarExt

	public: // aliases ; I hope it will be fully optimized out/elided (TODO if not then copy/paste code of above methods)
	// public? compiler bug? would prefer to have it as private, but lambdas made in cCmdProcessing should access this fields
		string v(int nr, const string &def="",  bool doThrow=0) const throw(cErrArgIllegal) { return VarDef(nr,def,doThrow); }
		vector<string> o(const string& name) const throw(cErrArgIllegal)  { return OptIf(name); }
		string o1(const string& name, const string &def="") const throw(cErrArgIllegal) { return Opt1If(name,def); }

		string V(int nr) const throw(cErrArgNotFound) { return Var(nr); }
		vector<string> O(const string& name) const throw(cErrArgNotFound) { return Opt(name); }
		string O1(const string& name) const throw(cErrArgNotFound) { return Opt1(name); }

		bool has(const string &name) const throw(cErrArgIllegal) { return IsOpt(name); }

//		virtual const cCmdProcessing* MetaGetProcessing() const; // return optional pointer to the processing information
//		virtual cCmdProcessing MetaGetProcessing() const; // return optional pointer to the processing information
	const tVar & getmVar() const{return this->mVar;}
	const tOption & getmOption() const{return this->mOption;}
	const tVar & getmVarExt() const{return this->mVarExt;}

	protected:
		void AssertLegalOptName(const string & name) const throw(cErrArgIllegal); // used internally to catch programming errors e.g. in binding lambdas
};

// ============================================================================

/**
Command data, but with the details of parsing - e.g. to explain/show user exact errors
or to run completions that work on characters instead of data/arg numbers
*/
class cCmdDataParse : public cCmdData { MAKE_CLASS_NAME("cCmdDataParse");
	protected:
		friend class cCmdProcessing;

		string mOrginalCommand; // full orginal command as given e.g. by the user, "ot msg     send   bob    'alice' title"
		vector< cParseEntity  > mWordIx2Entity; // mWordIxEntity[3].mCharPos==20, means that 4th word starts at character position 20 in the orginal command string

		int mFirstArgAfterWord; // at which word we have first param. Usually after 3 words (ot msg send ...) but could be e.g. 2 ("ot help" ...)
		int mFirstWord; // at which word is the first entity (because we could had removed "ot" for example). Usually 1 ("ot something ...")

		int mCharShift; // *deprecated?* how many characters should we shift to get back to orginal string becuse auto-prepending like "help"->"ot help" (-3), or 0 often
		bool mIsPreErased;
		

	public:
		cCmdDataParse();

		int CharIx2WordIx(int char_ix) const; // eg #15 char = word #4
		int WordIx2ArgNr(int word_ix) const; // eg word #4 = argument #1 like in "ot msg ls alice" alice is 4th word == 1 argument
};

// ============================================================================

/**
Name of command like "msg sendfrom"
*/
class cCmdName {  MAKE_CLASS_NAME("cCmdName");
	protected:
		string mName;
	public:
		cCmdName(const string &name);

		bool operator<(const cCmdName &other) const;
		operator std::string() const;
};


// ot msg sendfrom alice bob --cc mark --cc dave --cc mark
//                           ^^^^^^^^^           ^^^^^^^^^  warning duplicated recipient
//
// ot msg sendfrom alice bob --intaractive --prio 4 --verbose
//                           ^^^^^^^^^^^^^         ^^^^^^^^^^  warning excluding options
//
// ot msg sendfrom alice bob --prio 4 --cc dave --prio 8
//                           ^^^^^^^^           ^^^^^^^^ warning: there can be just one --prio

class cValidateError {
	public:
		enum class tKind {
			w_syntax, // problem detectable just by seeing the strings of arguments (warning)
			e_syntax, // same - but error
			w_data, // problem when working on the data, e.g. after checking the address-book we know this operation seems strange
			e_data, // after checking the data we are sure this operation is wrong
		};

		enum class tGuess {
			none, // this problem exists alwas, no guessing, e.g. "10AAA1" is never a valid amount of currency (float)
			cached, // this problem exists (with data) as we checked the cached data (cache, local address book etc) perhaps it would work
			now  // we checked with authoritative source now (e.g. the server) and still there is a problem (user dosn't exist on server)
		};

		tKind mKind;
		tGuess mGuess;

		string mMessage;

		vector<int> argpos; // TODO:nrix at which position of argument did the error occured

	public:
		cValidateError(const string &message, tKind exit, tGuess guess) : mMessage(message), mKind(exit), mGuess(guess) { }  // inline

		void Print() const;
};

//		vector< cValidateError > mError;

/**
Info about Parameter: How to validate and how to complete this argument
*/
class cParamInfo {  MAKE_CLASS_NAME("cParamInfo");
	public:
		friend class cCmdParser; // allow direct access - will need that when building pNym etc for shorter syntax

		// bool validation_function ( otuse, partial_data, curr_word_ix )
		// use: this function should validate the curr_word_ix out of data - data.ArgDef
		// warning: the curr_word_ix might NOT exist
		// ot msg sendfrom alice %%% hel<--validate --prio 4   ( use , data["alice", "%%%", "hel"  ], 2 )
		typedef function< bool ( nUse::cUseOT &, cCmdData &, size_t ) > tFuncValid;

		// vector<string>   hint_function ( otuse, partial_data, curr_word_ix )
		// warning: the curr_word_ix might NOT exist
		// ot msg sendfrom alice bo<TAB> hello --prio 4   ( use , data["alice", "bo", "hello"  ], 1 )
		// ot msg sendfrom alice bob hel<TAB> --prio 4   ( use , data["alice", "bob", "hel"  ], 2 )
		typedef function< vector<string> ( nUse::cUseOT &, cCmdData &, size_t ) > tFuncHint;

		enum eFlags {
				takesValue = 1 << 0,// if used as option, then: YES it take a value, or NO is it an boolean option like --dry-run
				isBoring = 2 << 0, // is the option boring option like --dry-run
		}; // edit-warning! always edit this and the following struct together so they match:
		struct sFlags { // as struct with named fields
			bool takesValue:1;
			bool isBoring:1;
		};

		typedef std::underlying_type<eFlags>::type tFlags_bits;
		union tFlags { tFlags_bits bits; sFlags n;
			tFlags() { bits = 0 | eFlags::takesValue ; } // defaults
			tFlags(tFlags_bits _bits) { bits=_bits; }
		}; // now you can access the data both ways

	protected:
		string mName; // short name
		string mDescr; // medium description

		tFuncValid funcValid;
		tFuncHint funcHint;

		tFlags mFlags;
	public:
		cParamInfo()=default;
		cParamInfo(const string &name, const string &descr, tFuncValid valid, tFuncHint hint, tFlags mFlags = tFlags());
		cParamInfo(const string &name, const string &descr); // to be used for renaming

		bool IsValid() const;

		operator string() const noexcept { return mName; }
		std::string getName() const noexcept { return mName; }
		std::string getName2() const noexcept { return mName+"("+mDescr+")"; }
		std::string getDescr() const noexcept { return mDescr; }
		bool getTakesValue() const noexcept { return mFlags.n.takesValue; }
		tFlags getFlags() const noexcept { return mFlags; }

		cParamInfo operator<<(const cParamInfo &B) const;

		tFuncValid GetFuncValid() const { return funcValid; }
		tFuncHint GetFuncHint() const { return funcHint; }
};


void cmd_test( shared_ptr<nUse::cUseOT> useOT );




}; // namespace nNewcli
}; // namespace nOT


#endif

