/* See other files here for the LICENCE that applies here. */
/*
Utils provides various utilities and general-purpose functions that
we find helpful in coding this project.
*/

#ifndef INCLUDE_OT_NEWCLI_UTILS
#define INCLUDE_OT_NEWCLI_UTILS

#include "lib_common1.hpp"

#ifndef CFG_WITH_TERMCOLORS
	#error "You requested to turn off terminal colors (CFG_WITH_TERMCOLORS), how ever currently they are hardcoded (this option to turn them off is not yet implemented)."
#endif

namespace nOT {

namespace nUtils {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

// ======================================================================================
// text trimming
std::string & ltrim(std::string &s);
std::string & rtrim(std::string &s);
std::string & trim(std::string &s);

// ======================================================================================
// string conversions
template <class T>
std::string ToStr(const T & obj) {
	std::ostringstream oss;
	oss << obj;
	return oss.str();
}

struct cNullstream : std::ostream {
		cNullstream();
};
extern cNullstream g_nullstream; // a stream that does nothing (eats/discards data)

// ========== debug ==========

#define _dbg3(X) do { gCurrentLogger.write_stream( 20) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0)
#define _dbg2(X) do { gCurrentLogger.write_stream( 30) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0)
#define _dbg1(X) do { gCurrentLogger.write_stream( 40) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // details
#define _info(X) do { gCurrentLogger.write_stream( 50) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // more boring info
#define _note(X) do { gCurrentLogger.write_stream( 70) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // interesting event
#define _mark(X) do { gCurrentLogger.write_stream( 80) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // interesting event
#define _warn(X) do { gCurrentLogger.write_stream( 90) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // some problem
#define _erro(X) do { gCurrentLogger.write_stream(100) << OT_CODE_STAMP << ' ' << X << gCurrentLogger.endline(); } while(0) // error - report

const char* DbgShortenCodeFileName(const char *s);

std::string cSpaceFromEscape(const std::string &s);

// ========== logger ==========

class cLogger {
	public:
		cLogger();
		~cLogger(){ cout << "Logger destructor" << endl; }
		std::ostream & write_stream(int level);
		void setOutStream();
		std::string icon(int level) const;
		std::string endline() const;
		void setDebugLevel(int level);
	protected:
		unique_ptr<std::ofstream> outfile;
		std::ostream * mStream; // pointing only
		int mLevel;
};



// ====================================================================
// vector debug

template <class T>
std::string vectorToStr(const T & v) {
	std::ostringstream oss;
	for(auto rec: v) {
		oss << rec <<",";
		}
	return oss.str();
}

template <class T>
void DisplayVector(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	std::copy( v.begin(), v.end(), std::ostream_iterator<T>(out, delim.c_str()) );
}

template <class T>
void EndlDisplayVector(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	out << std::endl;
	DisplayVector(out,v,delim);
}

template <class T>
void DisplayVectorEndl(std::ostream & out, const std::vector<T> &v, const std::string &delim=" ") {
	DisplayVector(out,v,delim);
	out << std::endl;
}

template <class T>
void DBGDisplayVector(const std::vector<T> &v, const std::string &delim=" ") {
	std::cerr << "[";
	std::copy( v.begin(), v.end(), std::ostream_iterator<T>(std::cerr, delim.c_str()) );
	std::cerr << "]";
}


template <class T, class T2>
void DisplayMap(std::ostream & out, const std::map<T, T2> &m, const std::string &delim=" ") {
	for(auto var : m) {
		out << var.first << delim << var.second << endl;
 	}
}

template <class T, class T2>
void EndlDisplayMap(std::ostream & out, const std::map<T, T2> &m, const std::string &delim=" ") {
	out << endl;
	for(auto var : m) {
		out << var.first << delim << var.second << endl;
 	}
}

template <class T, class T2>
void DBGDisplayMap(const std::map<T, T2> &m, const std::string &delim=" ") {
	for(auto var : m) {
		std::cerr << var.first << delim << var.second << endl;
 	}
}


template <class T>
void DBGDisplayVectorEndl(const std::vector<T> &v, const std::string &delim=" ") {
	DBGDisplayVector(v,delim);
	std::cerr << std::endl;
}

void DisplayStringEndl(std::ostream & out, const std::string text);

bool CheckIfBegins(const std::string & beggining, const std::string & all);
std::string SpaceFromEscape(const std::string &s);
std::string EscapeFromSpace(const std::string &s);
vector<string> WordsThatMatch(const std::string & sofar, const vector<string> & possib);
char GetLastChar(const std::string & str);
std::string GetLastCharIf(const std::string & str); // TODO unicode?
std::string EscapeString(const std::string &s);


template <class T>
std::string DbgVector(const std::vector<T> &v, const std::string &delim="|") {
	std::ostringstream oss;
	oss << "[";
	std::copy( v.begin(), v.end(), std::ostream_iterator<T>(oss, delim.c_str()) );
	oss << "]";
	return oss.str();
}

// ====================================================================
// assert

// ASRT - assert. Name like ASSERT() was too long, and ASS() was just... no.
// Use it like this: ASRT( x>y );  with the semicolon at end, a clever trick forces this syntax :)
#define ASRT(x) do { if (!(x)) Assert(false, OT_CODE_STAMP); } while(0)

void Assert(bool result, const std::string &stamp);

// ====================================================================
// advanced string

const std::string GetMultiline(string endLine = "~");
vector<string> SplitString(const string & str);

const bool checkPrefix(const string & str, char prefix = '^');
// ====================================================================
// operation on files

class ConfigManager {
public:
	bool Load(const string & fileName, map<string, string> & configMap);
	void Save(const string & fileName, const map<string, string> & configMap);
};

extern ConfigManager configManager;

// ====================================================================

namespace nOper { // nOT::nUtils::nOper
// cool shortcut operators, like vector + vecotr operator working same as string (appending)
// isolated to namespace because it's unorthodox ide to implement this

using namespace std;

// TODO use && and move?
template <class T>
vector<T> operator+(const vector<T> &a, const vector<T> &b) {
	vector<T> ret = a;
	ret.insert( ret.end() , b.begin(), b.end() );
	return ret;
}

template <class T>
vector<T> & operator+=(vector<T> &a, const vector<T> &b) {
	return a.insert( a.end() , b.begin(), b.end() );
}

} // nOT::nUtils::nOper

// ====================================================================

}; // namespace nUtils 

}; // namespace nOT


// global namespace
extern nOT::nUtils::cLogger gCurrentLogger;

const std::string * GetObjectName();

#define OT_CODE_STAMP ( nOT::nUtils::ToStr("[") + nOT::nUtils::DbgShortenCodeFileName(__FILE__) + nOT::nUtils::ToStr("+") + nOT::nUtils::ToStr(__LINE__) + nOT::nUtils::ToStr(" ") + nOT::nUtils::ToStr(*GetObjectName()) + nOT::nUtils::ToStr("::") + nOT::nUtils::ToStr(__FUNCTION__) + nOT::nUtils::ToStr("]"))


#endif

