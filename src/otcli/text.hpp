
#ifndef INCLUDE_OT_SHELL_TEXT
#define INCLUDE_OT_SHELL_TEXT

#include "lib_common1.hpp"

namespace nOT {
namespace nText {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

/* TRANSLATIONS
 * Modules of the program have separate translations files.
 * Every translation file has version in other languages.
 *
 * Example:
 *
 *  translate/cmdhelp.en.txt
 *  translate/cmdhelp.pl.txt
 *  translate/cmdhelp.de.txt
 *
 *  translate/cmdhelpdev.pl.txt
 *  translate/cmdhelpdev.en.txt
 */

#define q(x) #x

enum class eDictType { help, helpdev }; ///< Type of language for different modules

using textType = string;

class cDict;

class cTranslations {
	// Singleton
	// TODO get list of languages - best option is to get it from file names
	public:
		static cTranslations& getInstance() {
			static cTranslations instance; // Guaranteed to be destroyed. Instantiated on first use.
			return instance;
		}
	private:
		cTranslations() {}; // Private so that it can  not be called
		cTranslations(cTranslations const&); // Don't Implement!
		void operator=(cTranslations const&); // Don't implement!

	private:
		map<eDictType, cDict> mDefaultLang; ///< Default language (eng)
		map<eDictType, cDict> mCurrentLang; ///< Current chosen language
		vector <string> mLanguages;
	public:
		bool LoadLang(const string & lang, bool def = false); ///< Set language
		textType GetText(eDictType type, const string & key) const; ///< Get text from dictionary of specific type
		vector<string> GetLanguages(bool forceReload); ///< Get all supported languages
};

extern cTranslations * gTranslations;

class cDict {
	private:
		map<string, textType> mData; ///< Basic dictionary data
		map<string, textType> ParseFile(const string & fileName);
	public:
		cDict(const string & fileName); ///< Load dictionary from file
		bool LoadDict(const string & fileName); ///< Load/Reload dictionary from file
		textType GetValue(const string & key) const; ///< Get text based on the sting key
};

} // namespace nText
} // namespace nOT

#endif
