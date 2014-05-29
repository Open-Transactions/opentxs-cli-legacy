/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_USEOT
#define INCLUDE_OT_NEWCLI_USEOT

#include "lib_common2.hpp"
#include "othint.hpp"

namespace nOT {
namespace nUse {

	INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

	class cUseOT {

		map<string, string> mDefaultIDs;
		const string mDataFolder;
		const string mDefaultIDsFile;

		void LoadDefaults();

		public:

		// vector<nOT::nNewcli::cNyminfo> mNymsMy; // FIXME Not used class cNyminfo (look: othint.cpp)
		vector<string> mNymsMy_str; // TODO optimize/share memory? or convert on usage

		bool mNymsMy_loaded;
		bool OTAPI_loaded;
		bool OTAPI_error;

		cUseOT();
		~cUseOT();

		bool Init();
		void Done();
		void CloseApi();

		bool accountCheckIfExists(const string & accountName);
		const int64_t accountGetBalance(const string & accountName);
		const string accountGetId(const string & accountName);
		const string accountGetDefault();
		const string accountGetName(const string & accountID);
		string accountDelete(const string & accountName);
		const vector<string> accountGetIds();
		void accountRefreshAll();
		const string accountRename(const string & oldAccountName, const string & newAccountName);
		void accountSetDefault(const string & accountName);
		const string accountSetName(const string & accountID, const string & NewAccountName);
		void accountCreate(const string & assetName, const string & newAccountName);
		const vector<string> accountsGet();


		bool assetCheckIfExists(const string & assetName);
		const string assetGetId(const string & assetName);
		const vector<string> assetsGetNames();
		const string assetIssue(const std::string & serverID, const std::string & nymID, const std::string & signedContract);
		const string assetNew(const std::string & nymID, const std::string & xmlContents);
		const string assetGetContract(const std::string & assetID);
		const string assetGetDefault(); // Also known as purse
		void assetRemove(const string & assetName);
		void assetSetDefault(const std::string & assetName); // Also known as purse

		const string contractSign(const std::string & nymID, const std::string & contract);

		const vector<string> msgGetAll();
		const vector<string> msgGetForNym(const string & nymName);
		void msgSend(const string & nymSender, const string & nymRecipient, const string & msg);
		void msgRemoveByIndex(const string & nymName, const int32_t & nIndex);

		void nymCheck(const string & hisNymID);
		bool nymCheckByName(const string & nymName);
		void nymCreate(const string & nymName);
		const string nymGetDefault();
		const string nymGetId(const string & nymName);
		const string nymGetInfo(const string & nymName);
		const string nymGetName(const string & nymID);
		void nymRefresh();
		void nymRegister(const string & nymName);
		void nymRegister(const string & nymName, const string & serverName);
		void nymRemove(const string & nymName);
		void nymSetDefault(const string & nymName);
		const vector<string> nymsGetMy();

		void serverAdd(const std::string & contract);
		void serverCheck();
		const string serverGetDefault();
		const string serverGetId(const string & serverName);
		const string serverGetName(const string & serverID);
		void serverRemove(const string & serverName);
		void serverSetDefault(const string & serverName);
		const vector<string> serversGet();

		const string textEncode(const string & plainText);
		const string textEncrypt(const string & recipientNymName, const string & plainText);
		const string textDecode(const string & encodedText);
		const string textDecrypt(const string & recipientNymName, const string & encryptedText);
	};

	extern cUseOT useOT;

} // nUse
} // namespace nOT

#endif

