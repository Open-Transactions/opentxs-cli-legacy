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

		bool AccountCheckIfExists(const string & accountName);
		const int64_t AccountGetBalance(const string & accountName);
		const string AccountGetId(const string & accountName);
		const string AccountGetDefault();
		const string AccountGetName(const string & accountID);
		string AccountDelete(const string & accountName);
		const vector<string> AccountGetIds();
		void AccountRefreshAll();
		const string AccountRename(const string & oldAccountName, const string & newAccountName);
		void AccountSetDefault(const string & accountName);
		const string AccountSetName(const string & accountID, const string & NewAccountName);
		void AccountCreate(const string & MsgName, const string & newAccountName);
		const vector<string> AccountsGet();


		bool AssetCheckIfExists(const string & assetName);
		const string AssetGetId(const string & assetName);
		const vector<string> AssetsGetNames();
		const string AssetIssue(const std::string & serverID, const std::string & nymID, const std::string & signedContract);
		const string AssetNew(const std::string & nymID, const std::string & xmlContents);
		const string AssetGetContract(const std::string & assetID);
		const string AssetGetDefault(); // Also known as purse
		void AssetRemove(const string & assetName);
		void AssetSetDefault(const std::string & assetName); // Also known as purse

		const string ContractSign(const std::string & nymID, const std::string & contract);

		const vector<string> MsgGetAll();
		const vector<string> MsgGetForNym(const string & nymName);
		void MsgSend(const string & nymSender, const string & nymRecipient, const string & msg);
		void MsgSend(const string & nymRecipient, const string & msg);
		void MsgInRemoveByIndex(const string & nymName, const int32_t & nIndex);
		void MsgOutRemoveByIndex(const string & nymName, const int32_t & nIndex);

		void NymCheck(const string & hisNymID);
		bool NymCheckByName(const string & nymName);
		void NymCreate(const string & nymName);
		const string NymGetDefault();
		const string NymGetId(const string & nymName);
		const string NymGetInfo(const string & nymName);
		const string NymGetName(const string & nymID);
		void NymRefresh();
		void NymRegister(const string & nymName);
		void NymRegister(const string & nymName, const string & serverName);
		void NymRemove(const string & nymName);
		void NymSetDefault(const string & nymName);
		const vector<string> NymsGetMy();

		void ServerAdd(const std::string & contract);
		void ServerCheck();
		const string ServerGetDefault();
		const string ServerGetId(const string & serverName);
		const string ServerGetName(const string & serverID);
		void ServerRemove(const string & serverName);
		void ServerSetDefault(const string & serverName);
		const vector<string> ServersGet();

		const string TextEncode(const string & plainText);
		const string TextEncrypt(const string & recipientNymName, const string & plainText);
		const string TextDecode(const string & encodedText);
		const string TextDecrypt(const string & recipientNymName, const string & encryptedText);
	};

	extern cUseOT useOT;

} // nUse
} // namespace nOT

#endif

