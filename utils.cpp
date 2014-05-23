/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#include "utils.hpp"

#include "ccolor.hpp"

#include "lib_common1.hpp"

#include "runoptions.hpp"

namespace nOT {
namespace nUtils {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1; // <=== namespaces

// text trimming
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
std::string & ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

std::string & rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
} 

std::string & trim(std::string &s) {
	return ltrim(rtrim(s));
}

cNullstream::cNullstream() { } 

cNullstream g_nullstream; // extern a stream that does nothing (eats/discards data)

// ====================================================================

const char* DbgShortenCodeFileName(const char *s) {
	const char *p = s;
	const char *a = s;
	while (*p) {
		++p;
		if ((*p)=='/') a=p;
	}
	return a;
}


// ====================================================================

std::ostream & cLogger::write_stream(int level) {
	if ((level >= mLevel) && (mStream)) { *mStream << icon(level) << ' '; return *mStream; } 
	return g_nullstream; 
}

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

void cLogger::setOutStream() {
	// cerr<<__FUNCTION__<<endl; // XXX
	// cerr<<"mDebug="<<gRunOptions.getDebug()<<endl; // XXX
	if ( gRunOptions.getDebug() ) {
		if ( gRunOptions.getDebugSendToFile() ) {
			cerr << "file" << endl;
			outfile =  make_unique<std::ofstream> ("debuglog.txt");
			mStream = & (*outfile);
		}
		else if ( gRunOptions.getDebugSendToCerr() ) {
			cerr << "cerr" << endl;
			mStream = & std::cerr;
		}
		else {
			cerr << "null" << endl;
			mStream = & g_nullstream;
		}
	}
	else {
		cerr << "no debug" << endl;
		mStream = & g_nullstream;
	}
}

void cLogger::setDebugLevel(int level) {
	bool note_before = (mLevel > level); // report the level change before or after the change? (on higher level)
	if (note_before) _note("Setting debug level to "<<level);
	mLevel = level;
	if (!note_before) _note("Setting debug level to "<<level);
}

cLogger::cLogger() : mStream(NULL), mLevel(20) { mStream = & std::cout; }

std::string cLogger::icon(int level) const {
	// TODO replan to avoid needles converting back and forth char*, string etc

	using namespace zkr;

	if (level >= 100) return cc::fore::red     + ToStr("ERROR ");
	if (level >=  90) return cc::fore::yellow  + ToStr("Warn  ");
	if (level >=  80) return cc::fore::magenta + ToStr("MARK  ");
	if (level >=  70) return cc::fore::cyan    + ToStr("Note  ");
	if (level >=  50) return cc::fore::green   + ToStr("info  ");
	if (level >=  40) return cc::fore::lightwhite    + ToStr("dbg   ");
	if (level >=  30) return cc::fore::lightblue   + ToStr("dbg   ");
	if (level >=  20) return cc::fore::blue    + ToStr("dbg   ");

	return "  ";
}

std::string cLogger::endline() const {
	return ToStr("\n") + zkr::cc::console; // TODO replan to avoid needles converting back and forth char*, string etc
}

cLogger current_logger; // extern TODO gCurrentLogger

// ====================================================================
// vector debug

void DisplayStringEndl(std::ostream & out, const std::string text) {
	out << text;
	out << std::endl;
}

std::string SpaceFromEscape(const std::string &s) {
	std::ostringstream  newStr;
		for(int i = 0; i < s.length();i++) {
			if(s[i] == '\\' && s[i+1] ==32)
				newStr<<"";
			else
				newStr<<s[i];
			}
	return newStr.str();
}

std::string EscapeFromSpace(const std::string &s) {
	std::ostringstream  newStr;
	for(int i = 0; i < s.length();i++) {
		if(s[i] == 32)
			newStr << "\\" << " ";
		else
			newStr << s[i];
	}
	return newStr.str();
}


std::string EscapeString(const std::string &s) {
	std::ostringstream  newStr;
		for(int i = 0; i < s.length();i++) {
			if(s[i] >=32 && s[i] <= 126)
				newStr<<s[i];
			else
				newStr<<"\\"<< (int) s[i];
			}

	return newStr.str();
}

bool CheckIfBegins(const std::string & beggining, const std::string & all) {
	if (all.compare(0, beggining.length(), beggining) == 0) {
		return 1;
	}
	else {
		return 0;
	}
}

vector<string> WordsThatMatch(const std::string & sofar, const vector<string> & possib) {
	vector<string> ret;
	for ( auto rec : possib) { // check of possibilities
		if (CheckIfBegins(sofar,rec)) {
			rec = EscapeFromSpace(rec);
			ret.push_back(rec); // this record matches
		}
	}
	return ret;
}

char GetLastChar(const std::string & str) { // TODO unicode?
	auto s = str.length();
	if (s==0) throw std::runtime_error("Getting last character of empty string (" + ToStr(s) + ")" + OT_CODE_STAMP);
	return str.at( s - 1);
}

std::string GetLastCharIf(const std::string & str) { // TODO unicode?
	auto s = str.length();
	if (s==0) return ""; // empty string signalizes ther is nothing to be returned
	return std::string( 1 , str.at( s - 1) );
}

// ====================================================================

// ASRT - assert. Name like ASSERT() was too long, and ASS() was just... no.
// Use it like this: ASRT( x>y );  with the semicolon at end, a clever trick forces this syntax :)
#define ASRT(x) do { if (!(x)) Assert(false, OT_CODE_STAMP); } while(0)

void Assert(bool result, const std::string &stamp) {
	if (!result) throw std::runtime_error("Assert failed at "+stamp);
}

// ====================================================================
// advanced string

const std::string GetMultiline(string endLine) {
	std::string result(""); // Taken from OT_CLI_ReadUntilEOF
	while (true) {
		std::string input_line("");
		if (std::getline(std::cin, input_line, '\n'))
		{
			input_line += "\n";
				if (input_line[0] == '~')
					break;
			result += input_line;
		}
		if (std::cin.eof() )
		{
			std::cin.clear();
				break;
		}
		if (std::cin.fail() )
		{
			std::cin.clear();
				break;
		}
		if (std::cin.bad())
		{
			std::cin.clear();
				break;
		}
	}
	return result;
}

vector<string> SplitString(const string & str){
		std::istringstream iss(str);
		vector<string> vec { std::istream_iterator<string>{iss}, std::istream_iterator<string>{} };
		return vec;
}

// ====================================================================
// operation on files

bool ConfigManager::Load(const string & fileName, map<string, string> & configMap){
	_dbg3("Loading defaults.");
	std::ifstream inFile(fileName.c_str());
	if( inFile.good() ) {
		string line;
		while( std::getline (inFile, line) ) {
			vector<string> vec = SplitString(line);
			if (vec.size() == 2)
				configMap.insert ( std::pair<string, string>( vec.at(0), vec.at(1) ) );
			else
				configMap.insert ( std::pair<string, string>( vec.at(0), "-" ) );
		}
		return true;
	}
	return false;
}

void ConfigManager::Save(const string & fileName, const map<string, string> & configMap){
	std::ofstream outFile(fileName.c_str());
	for (auto pair : configMap) {
		outFile << pair.first << " ";
		outFile << pair.second;
		outFile << endl;
	}
}

ConfigManager configManager;

}; // namespace nUtil


}; // namespace OT


std::string GetObjectName() {	return GetObjectName_global_string; }
std::string GetObjectName_global_string="(global)"; // extern definition/initialization



