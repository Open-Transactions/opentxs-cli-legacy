/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "text.hpp"

#include "lib_common2.hpp"

namespace nOT {
namespace nText {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2 // <=== namespaces

cTranslations * gTranslations = &cTranslations::getInstance();

map <eDictType, string> dictTypeStr	{ { eDictType::help, "help" },
												 	 	 	 	  		{ eDictType::helpdev, "helpdev" }
};

bool cTranslations::LoadLang(const string & lang, bool def) {
	_info("Load language: " << lang << " as default?: " << std::to_string(def));
	string dir = "translations/";
	string path;
	if (def) mDefaultLang.clear();
	else mCurrentLang.clear();

	for (auto type : dictTypeStr) {
		path = dir + type.second + "." + lang + ".txt";
		_dbg3("dictionary path: " << path);
		if (def) {
			mDefaultLang.insert( std::pair<eDictType, cDict>(type.first, cDict(path)) );
		}
		else
			mCurrentLang.insert( std::pair<eDictType, cDict>(type.first, cDict(path)) );
	}
	return true;
}

textType cTranslations::GetText(eDictType type, const string & key) const {
	textType text = mCurrentLang.at(type).GetValue(key);
	if (text.empty()) {
		text = mDefaultLang.at(type).GetValue(key);
	}
	return text;
}

//===============================================================================

map<textType, textType> cDict::ParseFile(const string & fileName) {
	map<string, textType> data;
	std::ifstream DictFile(fileName);
	string line;
	string key;
	textType text;
	if (DictFile.is_open()) {
		while ( std::getline(DictFile, line) ) {
			auto pos = line.find_first_of(' ');
			key = line.substr(0, pos);
			text = line.erase(0, pos);
			data.insert( std::pair<textType, textType>(key,text) );
		}
		DictFile.close();
	}
	return data;
}

cDict::cDict(const string & fileName) {
	mData = ParseFile(fileName);
}

bool cDict::LoadDict(const string & fileName) {
	mData.clear();
	mData = ParseFile(fileName);
	if (mData.empty()) {
		return false;
	}
	return true;
}

textType cDict::GetValue(const string & key) const {
	textType value;
	try {
		value = mData.at(key);
	}
	catch (const std::out_of_range& oor) {
		_erro("Key for translation not found - error: " << oor.what());
		return "Key not found!";
	}
	return value;
}

} // namespace nText
} // namespace nOT
