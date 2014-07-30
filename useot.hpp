/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_USEOT
#define INCLUDE_OT_NEWCLI_USEOT

#include "lib_common3.hpp"

// Use this to mark methods
#define	EXEC
#define	HINT
#define	VALID

namespace nOT {
namespace nUse {

	INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

	using ID = string;
	using name = string;

	class cUseCache { // TODO optimize/share memory? or convert on usage
		friend class cUseOT;
	public:
		cUseCache();
	protected:
		map<ID, name> mNyms;
		map<ID, name> mAccounts;
		map<ID, name> mAssets;
		map<ID, name> mServers;
		bool mNymsLoaded;
		bool mAccountsLoaded;
		bool mAssetsLoaded;
		bool mServersLoaded;
	private:
	};

	class cUseOT {
	public:

		static bool OTAPI_loaded;
		static bool OTAPI_error;

	private:

		string mDbgName;

		OT_ME mMadeEasy;

		cUseCache mCache;

		map<nUtils::eSubjectType, ID> mDefaultIDs; ///< Default IDs are saved to file after changing any default ID
		const string mDataFolder;
		const string mDefaultIDsFile;

		typedef const ID ( cUseOT::*FPTR ) (const string &);

		map<nUtils::eSubjectType, FPTR> subjectGetIDFunc; ///< Map to store pointers to GetID functions
		map<nUtils::eSubjectType, FPTR> subjectGetNameFunc; ///< Map to store pointers to GetName functions

	private:

		void LoadDefaults(); ///< Defaults are loaded when initializing OTAPI

	protected:

		enum class eBoxType { Inbox, Outbox };
		EXEC bool MsgDisplayForNymBox( eBoxType boxType, const string & nymName, int msg_index, bool dryrun);

	public:

		cUseOT(const string &mDbgName);
		~cUseOT();

		string DbgName() const noexcept;

		bool Init();
		void CloseApi();

		VALID bool CheckIfExists(const nUtils::eSubjectType type, const string & subject);
		EXEC bool DisplayDefaultSubject(const nUtils::eSubjectType type, bool dryrun);
		bool DisplayAllDefaults(bool dryrun);
		EXEC bool DisplayHistory(bool dryrun);
		string SubjectGetDescr(const nUtils::eSubjectType type, const string & subject);
		bool Refresh(bool dryrun);
		//================= account =================

		const vector<ID> AccountGetAllIds();
		const int64_t AccountGetBalance(const string & accountName);
		const string AccountGetDefault();
		const ID AccountGetId(const string & account); ///< Gets account ID both from name and ID with prefix
		const string AccountGetName(const ID & accountID);
		bool AccountSetName(const string & accountID, const string & NewAccountName);

		HINT const vector<string> AccountGetAllNames();

		EXEC bool AccountCreate(const string & nym, const string & asset, const string & newAccountName, bool dryrun);
		EXEC bool AccountDisplay(const string & account, bool dryrun);
		EXEC bool AccountDisplayAll(bool dryrun);
		EXEC bool AccountRefresh(const string & accountName, bool all, bool dryrun);
		EXEC bool AccountRemove(const string & account, bool dryrun) ;
		EXEC bool AccountRename(const string & account, const string & newAccountName, bool dryrun);
		EXEC bool AccountSetDefault(const string & account, bool dryrun);
		EXEC bool AccountTransfer(const string & accountFrom, const string & accountTo, const int64_t & amount, const string & note, bool dryrun);

		//================= account-in =================

		EXEC bool AccountInDisplay(const string & account, bool dryrun);
		EXEC bool AccountInAccept(const string & account, const int index, bool all, bool dryrun);

		//================= account-out =================

		EXEC bool AccountOutCancel(const string & account, const int index, bool all, bool dryrun);
		EXEC bool AccountOutDisplay(const string & account, bool dryrun);

		//================= asset =================

		const ID AssetGetId(const string & asset); ///< Gets asset ID both from name and ID with prefix
		const string AssetGetName(const ID & accountID);
		const string AssetGetContract(const string & asset);
		const string AssetGetDefault(); // Get default asset, also known as purse

		HINT const vector<string> AssetGetAllNames();

		EXEC bool AssetSetDefault(const std::string & asset, bool dryrun); // Set default asset, also known as purse
		EXEC bool AssetDisplayAll(bool dryrun);
		EXEC bool AssetIssue(const string & serverID, const string & nymID, bool dryrun) ;
		EXEC bool AssetNew(const string & nym, bool dryrun);
		EXEC bool AssetRemove(const string & asset, bool dryrun);

		//================= cash =================

		EXEC bool CashWithdraw(const string & account, int64_t amount, bool dryrun); ///< withdraw cash from account on server into local purse

		//================= ?contract =================

		const string ContractSign(const std::string & nymID, const std::string & contract);

		//================= msg =================

		const vector<string> MsgGetAll();
		bool MsgSend(const string & nymSender, const string & nymRecipient, const string & msg);

		VALID bool MsgInCheckIndex(const string & nymName, const int32_t & nIndex);
		VALID bool MsgOutCheckIndex(const string & nymName, const int32_t & nIndex);

		EXEC bool MsgDisplayForNym(const string & nymName, bool dryrun);

		EXEC bool MsgDisplayForNymInbox(const string & nymName, int msg_index, bool dryrun);
		EXEC bool MsgDisplayForNymOutbox(const string & nymName, int msg_index, bool dryrun);

		EXEC bool MsgSend(const string & nymSender, vector<string> nymRecipient, const string & subject, const string & msg, int prio, bool dryrun);
		EXEC bool MsgInRemoveByIndex(const string & nymName, const int32_t & nIndex, bool dryrun);
		EXEC bool MsgOutRemoveByIndex(const string & nymName, const int32_t & nIndex, bool dryrun);

		//================= nym =================

		void NymGetAll(bool force=false);
		const vector<string> NymGetAllIDs();
		const string NymGetDefault();
		const ID NymGetId(const string & nym); ///< Gets Nym ID both from name and ID with prefix
		const string NymGetName(const ID & nymID);
		bool NymSetName(const ID & nymID, const string & newNymName);

		HINT const vector<string> NymGetAllNames();

		EXEC bool NymCheck(const string & nymName, bool dryrun);
		EXEC bool NymCreate(const string & nymName, bool registerOnServer, bool dryrun);
		EXEC bool NymDisplayAll(bool dryrun);
		EXEC bool NymDisplayInfo(const string & nymName, bool dryrun);
		EXEC bool NymExport(const string & nymName, bool dryrun);
		EXEC bool NymImport(bool dryrun);
		EXEC bool NymRefresh(const string & nymName, bool all, bool dryrun);
		EXEC bool NymRegister(const string & nymName, const string & serverName, bool dryrun);
		EXEC bool NymRemove(const string & nymName, bool dryrun);
		EXEC bool NymRename(const string & oldNymName, const string & newNymName, bool dryrun);
		EXEC bool NymSetDefault(const string & nymName, bool dryrun);

		//================= server =================

		void ServerCheck(); ///< Check server availability (ping server)
		const ID ServerGetDefault(); ///< Gets ID of default server
		const ID ServerGetId(const string & server); ///< Gets server ID both from name and ID with prefix
		const string ServerGetName(const string & serverID); ///< Gets Name of default server

		HINT const vector<string> ServerGetAllNames();

		EXEC bool ServerAdd(bool dryrun); ///< Add new server contract
		EXEC bool ServerCreate(const string & nymName, bool dryrun); ///< Create new server contract
		EXEC bool ServerRemove(const string & serverName, bool dryrun);
		EXEC bool ServerSetDefault(const string & serverName, bool dryrun);
		EXEC bool ServerDisplayAll(bool dryrun);

		//================= text =================

		EXEC bool TextEncode(const string & plainText, bool dryrun);
		EXEC bool TextEncrypt(const string & recipientNymName, const string & plainText, bool dryrun);
		EXEC bool TextDecode(const string & encodedText, bool dryrun);
		EXEC bool TextDecrypt(const string & recipientNymName, const string & encryptedText, bool dryrun);
	};

} // nUse
} // namespace nOT

#endif

