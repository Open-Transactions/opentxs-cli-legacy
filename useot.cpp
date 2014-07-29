/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "useot.hpp"

#include <OTPaths.hpp>

#include "lib_common3.hpp"

#include "bprinter/table_printer.h"

// Editline. Check 'git checkout linenoise' to see linenoise version.
#ifndef CFG_USE_EDITLINE // should we use editline?
	#define CFG_USE_EDITLINE 1 // default
#endif

#if CFG_USE_EDITLINE
	#ifdef __unix__
		#include <editline/readline.h> // to display history, TODO move that functionality somewhere else
	#else // not unix
		// TODO: do support MinGWEditline for windows)
	#endif // not unix
#endif // not use editline

namespace nOT {
namespace nUse {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3; // <=== namespaces


cUseCache::cUseCache()
: mNymsLoaded(false)
, mAccountsLoaded(false)
, mAssetsLoaded(false)
, mServersLoaded(false)
{}

cUseOT::cUseOT(const string &mDbgName)
:
	mDbgName(mDbgName)
, mDataFolder( OTPaths::AppDataFolder().Get() )
, mDefaultIDsFile( mDataFolder + "defaults.opt" )
{
	_dbg1("Creating cUseOT "<<DbgName());
	FPTR fptr;
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Account, fptr = &cUseOT::AccountGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Asset, fptr = &cUseOT::AssetGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::User, fptr = &cUseOT::NymGetId) );
	subjectGetIDFunc.insert(std::make_pair(nUtils::eSubjectType::Server, fptr = &cUseOT::ServerGetId) );

	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Account, fptr = &cUseOT::AccountGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Asset, fptr = &cUseOT::AssetGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::User, fptr = &cUseOT::NymGetName) );
	subjectGetNameFunc.insert(std::make_pair(nUtils::eSubjectType::Server, fptr = &cUseOT::ServerGetName) );
}


string cUseOT::DbgName() const noexcept {
	return "cUseOT-" + ToStr((void*)this) + "-" + mDbgName;
}

void cUseOT::CloseApi() {
	if (OTAPI_loaded) {
		_dbg1("Will cleanup OTAPI");
		OTAPI_Wrap::AppCleanup(); // Close OTAPI
		_dbg2("Will cleanup OTAPI - DONE");
	} else _dbg3("Will cleanup OTAPI ... was already not loaded");
}

cUseOT::~cUseOT() {
}

void cUseOT::LoadDefaults() {
	// TODO What if there is, for example no accounts?
	// TODO Check if defaults are correct.
	if ( !configManager.Load(mDefaultIDsFile, mDefaultIDs) ) {
		_dbg1("Cannot open" + mDefaultIDsFile + " file, setting IDs with ID 0 as default"); //TODO check if there is any nym in wallet
		ID accountID = OTAPI_Wrap::GetAccountWallet_ID(0);
		ID assetID = OTAPI_Wrap::GetAssetType_ID(0);
		ID userID = OTAPI_Wrap::GetNym_ID(0);
		ID serverID = OTAPI_Wrap::GetServer_ID(0);

		if ( accountID.empty() )
			_warn("There is no accounts in the wallet, can't set default account");
		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Account, accountID));
		if ( assetID.empty() )
			_warn("There is no assets in the wallet, can't set default asset");
		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Asset, assetID));
		if ( userID.empty() )
			_warn("There is no nyms in the wallet, can't set default nym");
		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::User, userID));
		if ( serverID.empty() )
			_warn("There is no servers in the wallet, can't set default server");
		mDefaultIDs.insert(std::make_pair(nUtils::eSubjectType::Server, serverID));
	}
}

bool cUseOT::DisplayDefaultSubject(const nUtils::eSubjectType type, bool dryrun) {
	_fact("display default " << nUtils::SubjectType2String(type) );
	if(dryrun) return true;
	if(!Init()) return false;
	ID defaultID = mDefaultIDs.at(type);
	string defaultName = (this->*cUseOT::subjectGetNameFunc.at(type))(defaultID);
	nUtils::DisplayStringEndl(cout, "Defaut " + nUtils::SubjectType2String(type) + ":" );
	nUtils::DisplayStringEndl(cout, defaultID + " " + defaultName );
	return true;
}

bool cUseOT::DisplayAllDefaults(bool dryrun) {
	_fact("display all defaults" );
	if(dryrun) return true;
	if(!Init()) return false;
	for (auto var : mDefaultIDs) {
		string defaultName = (this->*cUseOT::subjectGetNameFunc.at(var.first))(var.second);
		nUtils::DisplayStringEndl( cout, SubjectType2String(var.first) + "\t" + var.second + " " + defaultName );
	}
	return true;
}

bool cUseOT::DisplayHistory(bool dryrun) {
	_fact("ot history");
	if(dryrun) return true;

	for (int i=1; i<history_length; i++) {
		DisplayStringEndl( cout, history_get(i)->line );
	}
	return true;
}

string cUseOT::SubjectGetDescr(const nUtils::eSubjectType type, const string & subject) {
	ID subjectID = (this->*cUseOT::subjectGetIDFunc.at(type))(subject);
	string subjectName = (this->*cUseOT::subjectGetNameFunc.at(type))(subjectID);
	string description = subjectName + "(" + subjectID + ")";
	return nUtils::stringToColor(description);
}

bool cUseOT::Refresh(bool dryrun){
	_fact("refresh all");
	if(dryrun) return true;
	if(!Init()) return false;
	bool StatusAccountRefresh=AccountRefresh(AccountGetName(AccountGetDefault()), true, false  );
	bool StatusNymRefresh=NymRefresh("^" + NymGetDefault(), true, false  );
	if( StatusAccountRefresh==true &&  StatusNymRefresh==true) {
		_info("Succesfull refresh");
		return true;
	}
	else if( StatusAccountRefresh==true &&  StatusNymRefresh==false) {
		_dbg1("Can not refresh Nym");
		return false;
	}
	else if( StatusAccountRefresh==false &&  StatusNymRefresh==true) {
		_dbg1("Can not refresh Account");
		return false;
	}
		_dbg1("Can not refresh ");
		return false;

}

bool cUseOT::Init() { // TODO init on the beginning of application execution
	if (OTAPI_error) return false;
	if (OTAPI_loaded) return true;
	try {
		if (!OTAPI_Wrap::AppInit()) { // Init OTAPI
			_erro("Error while initializing wrapper");
			return false; // <--- RET
		}

		_info("Trying to load wallet now.");
		// if not pWrap it means that AppInit is not initialized
		OTAPI_Exec *pWrap = OTAPI_Wrap::It(); // TODO check why OTAPI_Exec is needed
		if (!pWrap) {
			OTAPI_error = true;
			_erro("Error while init OTAPI (1)");
			return false;
		}

		if (OTAPI_Wrap::LoadWallet()) {
			_info("wallet was loaded.");
			OTAPI_loaded = true;
			LoadDefaults();
		}	else _erro("Error while loading wallet.");
	}
	catch(const std::exception &e) {
		_erro("Error while OTAPI init (2) - " << e.what());
		return false;
	}
	catch(...) {
		_erro("Error while OTAPI init thrown an UNKNOWN exception!");
		OTAPI_error = true;
		return false;
	}
	return OTAPI_loaded;
}

bool cUseOT::CheckIfExists(const nUtils::eSubjectType type, const string & subject) {
	if(!Init()) return false;

	ID subjectID = (this->*cUseOT::subjectGetIDFunc.at(type))(subject);

	if (!subjectID.empty()) {
		_dbg3("Account " + subject + " exists");
		return true;
	}
	_warn("Can't find this Account: " + subject);
	return false;
}

const vector<ID> cUseOT::AccountGetAllIds() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving accounts ID's");
	vector<string> accountsIDs;
	for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
		accountsIDs.push_back(OTAPI_Wrap::GetAccountWallet_ID (i));
	}
	return accountsIDs;
}

const int64_t cUseOT::AccountGetBalance(const string & accountName) {
	if(!Init()) return 0; //FIXME

	int64_t balance = OTAPI_Wrap::GetAccountWallet_Balance ( AccountGetId(accountName) );
	return balance;
}

const string cUseOT::AccountGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::Account);
}

const ID cUseOT::AccountGetId(const string & accountName) {
	if(!Init())
		return "";
	if ( nUtils::checkPrefix(accountName) )
		return accountName.substr(1);
	else {
		for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
			if(OTAPI_Wrap::GetAccountWallet_Name ( OTAPI_Wrap::GetAccountWallet_ID (i))==accountName)
			return OTAPI_Wrap::GetAccountWallet_ID (i);
		}
	}
	return "";
}

const string cUseOT::AccountGetName(const ID & accountID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetAccountWallet_Name(accountID);
}

bool cUseOT::AccountRemove(const string & account, bool dryrun) { ///<
	_fact("account rm " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	if(OTAPI_Wrap::Wallet_CanRemoveAccount (AccountGetId(account))) {
		_erro("Account cannot be deleted: doesn't have a zero balance?/outstanding receipts?");
		return false;
	}

	if( OTAPI_Wrap::deleteAssetAccount( mDefaultIDs.at(nUtils::eSubjectType::Server), mDefaultIDs.at(nUtils::eSubjectType::User), AccountGetId(account) ) ) { //FIXME should be
		_erro("Failure deleting account: " + account);
		return false;
	}
	_info("Account: " + account + " was successfully removed");
	return true;
}

bool cUseOT::AccountRefresh(const string & accountName, bool all, bool dryrun) {
	_fact("account refresh " << accountName << " all=" << all);
	if(dryrun) return true;
	if(!Init()) return false;

	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	if (all) {
		int32_t accountsRetrieved = 0;
		int32_t accountCount = OTAPI_Wrap::GetAccountCount();

		if (accountCount == 0){
			_warn("No accounts to retrieve");
			return true;
		}

		for (int32_t accountIndex = 0; accountIndex < accountCount; ++accountIndex) {
			ID accountID = OTAPI_Wrap::GetAccountWallet_ID(accountIndex);
			ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
			ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
			if ( mMadeEasy.retrieve_account(accountServerID, accountNymID, accountID, true) ) { // forcing download
				_info("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval success from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
				++accountsRetrieved;
			}else
				_erro("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval failure from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
		}
		string count = to_string(accountsRetrieved) + "/" + to_string(accountCount);
		if (accountsRetrieved == accountCount) {
			_info("All accounts were successfully retrieved " << count);
			return true;
		} else if (accountsRetrieved == 0) {
			_erro("Accounts retrieval failure " << count);
			return false;
		} else {
			_erro("Some accounts cannot be retrieved " << count);
			return true;
		}
	}
	else {
		ID accountID = AccountGetId(accountName);
		ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
		ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
		if ( mMadeEasy.retrieve_account(accountServerID, accountNymID, accountID, true) ) { // forcing download
			_info("Account " + accountName + "(" + accountID +  ")" + " retrieval success from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
			return true;
		}
		_warn("Account " + accountName + "(" + accountID +  ")" + " retrieval failure from server " + ServerGetName(accountServerID) + "(" + accountServerID +  ")");
		return false;
	}
	return false;
}

bool cUseOT::AccountRename(const string & account, const string & newAccountName, bool dryrun) {
	_fact("account mv from " << account << " to " << newAccountName);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);

	if( AccountSetName (accountID, newAccountName) ) {
		_info("Account " << AccountGetName(accountID) + "(" + accountID + ")" << " renamed to " << newAccountName);
		return true;
	}
	_erro("Failed to rename account " << AccountGetName(accountID) + "(" + accountID + ")" << " to " << newAccountName);
	return false;
}

bool cUseOT::AccountCreate(const string & nym, const string & asset, const string & newAccountName, bool dryrun) {
	_fact("account new nym=" << nym << " asset=" << asset << " accountName=" << newAccountName);
	if(dryrun) return true;
	if(!Init()) return false;

	if ( CheckIfExists(nUtils::eSubjectType::Account, newAccountName) ) {
		_erro("Cannot create new account: '" << newAccountName << "'. Account with that name exists" );
		return false;
	}

	ID nymID = NymGetId(nym);
	ID assetID = AssetGetId(asset);

	string response;
	response = mMadeEasy.create_asset_acct(mDefaultIDs.at(nUtils::eSubjectType::Server), nymID, assetID); //TODO server as argument

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy.VerifyMessageSuccess(response)) {
		_erro("Failed trying to create Account at Server.");
		return false;
	}

	// Get the ID of the new account.
	ID accountID = OTAPI_Wrap::Message_GetNewAcctID(response);
	if (!accountID.size()){
		_erro("Failed trying to get the new account's ID from the server response.");
		return false;
	}

	// Set the Name of the new account.
	if ( AccountSetName(accountID, newAccountName) ){
		cout << "Account " << newAccountName << "(" << accountID << ")" << " created successfully." << endl;
		AccountRefresh("^" + accountID, false, dryrun);
	}
	return false;
}

const vector<string> cUseOT::AccountGetAllNames() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving all accounts names");
	vector<string> accounts;
	for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
		accounts.push_back(OTAPI_Wrap::GetAccountWallet_Name ( OTAPI_Wrap::GetAccountWallet_ID (i)));
	}
	return accounts;
}

bool cUseOT::AccountDisplay(const string & account, bool dryrun) {
	_fact("account show account=" << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	string stat = mMadeEasy.stat_asset_account(accountID);
	if ( !stat.empty() ) {
			nUtils::DisplayStringEndl(cout, stat);
			return true;
	}
	_erro("Error trying to stat an account: " + accountID);
	return false;
}

bool cUseOT::AccountDisplayAll(bool dryrun) {
	_fact("account ls");
	if(dryrun) return true;
	if(!Init()) return false;

	bprinter::TablePrinter tp(&std::cout);
  tp.AddColumn("ID", 4);
  tp.AddColumn("Type", 10);
  tp.AddColumn("Account", 60);
  tp.AddColumn("Asset", 60);
  tp.AddColumn("Balance", 10);

  tp.PrintHeader();
	for(int32_t i = 0 ; i < OTAPI_Wrap::GetAccountCount();i++) {
		ID accountID = OTAPI_Wrap::GetAccountWallet_ID(i);
		int64_t balance = OTAPI_Wrap::GetAccountWallet_Balance(accountID);
		ID assetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
		string accountType = OTAPI_Wrap::GetAccountWallet_Type(accountID);
		if(accountType=="issuer") tp.SetContentColor(zkr::cc::fore::lightred);
		else if (accountType=="simple") tp.SetContentColor(zkr::cc::fore::lightgreen);

		tp << std::to_string(i) << "(" + accountType + ")" << AccountGetName(accountID) + "(" + accountID + ")"  << AssetGetName(assetID) + "(" + assetID + ")" << std::to_string(balance);
	}

	tp.PrintFooter();

	return true;
}

bool cUseOT::AccountSetDefault(const string & account, bool dryrun) {
	_fact("account set-default " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Account) = AccountGetId(account);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::AccountTransfer(const string & accountFrom, const string & accountTo, const int64_t & amount, const string & note, bool dryrun) {
	_fact("account transfer  from " << accountFrom << " to " << accountTo << " amount=" << amount << " note=" << note);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountFromID = AccountGetId(accountFrom);
	ID accountToID = AccountGetId(accountTo);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountFromID);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountFromID);

	string response = mMadeEasy.send_transfer(accountServerID, accountNymID, accountFromID, accountToID, amount, note);

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy.VerifyMessageSuccess(response)) {
		_erro("Failed to send transfer from " << accountFrom << " to " << accountTo);
		return false;
	}
	return true;
}

bool cUseOT::AccountInDisplay(const string & account, bool dryrun) {
	_fact("account-in ls " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);

	string inbox = OTAPI_Wrap::LoadInbox(accountServerID, accountNymID, accountID); // Returns NULL, or an inbox.

	if (inbox.empty()) {
		_info("Unable to load inbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
		return false;
	}

	int32_t transactionCount = OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountID, inbox);

	if (transactionCount > 0) {
		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Amount", 10);
		tp.AddColumn("Type", 10);
		tp.AddColumn("TxN", 8);
		tp.AddColumn("InRef", 8);
		tp.AddColumn("From Nym", 60);
		tp.AddColumn("From Account", 60);

		tp.PrintHeader();

		for (int32_t index = 0; index < transactionCount; ++index) {
			string transaction = OTAPI_Wrap::Ledger_GetTransactionByIndex(accountServerID, accountNymID, accountID, inbox, index);
			int64_t transactionID = OTAPI_Wrap::Ledger_GetTransactionIDByIndex(accountServerID, accountNymID, accountID, inbox, index);
			int64_t refNum = OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(accountServerID, accountNymID, accountID, transaction);
			int64_t amount = OTAPI_Wrap::Transaction_GetAmount(accountServerID, accountNymID, accountID, transaction);
			string transactionType = OTAPI_Wrap::Transaction_GetType(accountServerID, accountNymID, accountID, transaction);
			string senderNymID = OTAPI_Wrap::Transaction_GetSenderUserID(accountServerID, accountNymID, accountID, transaction);
			string senderAcctID = OTAPI_Wrap::Transaction_GetSenderAcctID(accountServerID, accountNymID, accountID, transaction);
			string recipientNymID = OTAPI_Wrap::Transaction_GetRecipientUserID(accountServerID, accountNymID, accountID, transaction);
			string recipientAcctID = OTAPI_Wrap::Transaction_GetRecipientAcctID(accountServerID, accountNymID, accountID, transaction);

			//TODO Check if Transaction information needs to be verified!!!

		  tp << to_string(index) << to_string(amount) << transactionType << to_string(transactionID) << to_string(refNum)
				 << NymGetName(senderNymID) + "(" + senderNymID + ")" <<  AccountGetName( senderAcctID ) + "(" + senderAcctID + ")";
		}
		tp.PrintFooter();
	  return true;
	} else {
		_info("There is no transactions in inbox for account "  << AccountGetName(accountID)<< "(" << accountID << ")");
		return true;
	}
	return false;
}

bool cUseOT::AccountInAccept(const string & account, const int index, bool all, bool dryrun) { //TODO make it work with --all, multiple indices
	_fact("account-in accept " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);

	int32_t nItemType = 0; // TODO pass it as an argument

	if (all) {
		int32_t transactionsAccepted = 0;

		ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
		ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);

		string inbox = OTAPI_Wrap::LoadInbox(accountServerID, accountNymID, accountID); // Returns NULL, or an inbox.

		if (inbox.empty()) {
			_info("Unable to load inbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
			return false;
		}
		int32_t transactionCount = OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountID, inbox);
		_dbg3("Transaction count in inbox: " << transactionCount);
		if (transactionCount == 0){
			_warn("No transactions in inbox");
			return true;
		}

		for (int32_t index = 0; index < transactionCount; ++index) { //FIXME work successfully only for first transaction
			if ( mMadeEasy.accept_inbox_items( accountID, nItemType, to_string(index) ) ) {
				_info("Successfully accepted inbox transaction number: " << index);
				++transactionsAccepted;
			} else
				_erro("Failed to accept inbox transaction for number: " << index);
		}
		string count = to_string(transactionsAccepted) + "/" + to_string(transactionCount);
		if (transactionsAccepted == transactionCount) {
			_info("All transactions were successfully accepted " << count);
			return true;
		} else if (transactionsAccepted == 0) {
			_erro("Transactions cannot be accepted " << count);
			return false;
		} else {
			_erro("Some transactions cannot be accepted " << count);
			return true;
		}
	}
	else {
		if ( mMadeEasy.accept_inbox_items( accountID, nItemType, to_string(index) ) ) {
			_info("Successfully accepted inbox transaction number: " << index);
			return true;
		}
		_erro("Failed to accept inbox transaction for number: " << index);
		return false;
	}
	return false;
}

bool cUseOT::AccountOutCancel(const string & account, const int index, bool all, bool dryrun) { //TODO make it work with --all, multiple indices
	_fact("account-out cancel " << account << " transaction=" << index);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	int32_t nItemType = 0; // TODO pass it as an argument

	if ( mMadeEasy.cancel_outgoing_payments( accountNymID, accountID, to_string(index) ) ) { //TODO cancel_outgoing_payments is not for account outbox
		_info("Successfully cancelled outbox transaction: " << index);
		return true;
	}
	_erro("Failed to cancel outbox transaction: " << index);
	return false;
}

bool cUseOT::AccountOutDisplay(const string & account, bool dryrun) {
	_fact("account-out ls " << account);
	if(dryrun) return true;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);

	string outbox = OTAPI_Wrap::LoadOutbox(accountServerID, accountNymID, accountID); // Returns NULL, or an inbox.

	if (outbox.empty()) {
		_info("Unable to load outbox for account " << AccountGetName(accountID)<< "(" << accountID << "). Perhaps it doesn't exist yet?");
		return false;
	}

	int32_t transactionCount = OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountID, outbox);

	if (transactionCount > 0) {
		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Amount", 10);
		tp.AddColumn("Type", 10);
		tp.AddColumn("TxN", 8);
		tp.AddColumn("InRef", 8);
		tp.AddColumn("To Nym", 60);
		tp.AddColumn("To Account", 60);

		tp.PrintHeader();
	  for (int32_t index = 0; index < transactionCount; ++index) {
			string transaction = OTAPI_Wrap::Ledger_GetTransactionByIndex(accountServerID, accountNymID, accountID, outbox, index);
			int64_t transactionID = OTAPI_Wrap::Ledger_GetTransactionIDByIndex(accountServerID, accountNymID, accountID, outbox, index);
			int64_t refNum = OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(accountServerID, accountNymID, accountID, transaction);
			int64_t amount = OTAPI_Wrap::Transaction_GetAmount(accountServerID, accountNymID, accountID, transaction);
			string transactionType = OTAPI_Wrap::Transaction_GetType(accountServerID, accountNymID, accountID, transaction);
			string recipientNymID = OTAPI_Wrap::Transaction_GetRecipientUserID(accountServerID, accountNymID, accountID, transaction); //FIXME transaction recipientID=NULL
			string recipientAcctID = OTAPI_Wrap::Transaction_GetRecipientAcctID(accountServerID, accountNymID, accountID, transaction);

			//TODO Check if Transaction information needs to be verified!!!
		 tp << to_string(index) << to_string(amount) << transactionType << to_string(transactionID) << to_string(refNum) << "BUG - working on it" << "BUG - working on it" ;
//			  << NymGetName(recipientNymID) + "(" + recipientNymID + ")" <<  AccountGetName( recipientAcctID ) + "(" + recipientAcctID + ")";
		}
		tp.PrintFooter();
	  return true;
	} else {
		_info("There is no transactions in outbox for account "  << AccountGetName(accountID)<< "(" << accountID << ")");
		return true;
	}
	return false;
}

bool cUseOT::AccountSetName(const string & accountID, const string & newAccountName) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	if(!Init()) return false;

	if ( !OTAPI_Wrap::SetAccountWallet_Name (accountID, mDefaultIDs.at(nUtils::eSubjectType::User), newAccountName) ) {
		_erro("Failed trying to name new account: " << accountID);
		return false;
	}
	_info("Set account " << accountID << "name to " << newAccountName);
	return true;
}

const vector<string> cUseOT::AssetGetAllNames() {
	if(!Init())
	return vector<string> {};

	vector<string> assets;
	for(int32_t i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
		assets.push_back(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i)));
	}
	return assets;
}

const string cUseOT::AssetGetName(const ID & assetID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetAssetType_Name(assetID);
}

bool cUseOT::AssetDisplayAll(bool dryrun) {
	_fact("asset ls");
	if(dryrun) return true;
	if(!Init()) return false;

	_dbg3("Retrieving all asset names");
	for(std::int32_t i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount();i++) {
		ID assetID = OTAPI_Wrap::GetAssetType_ID(i);
		nUtils::DisplayStringEndl(cout, AssetGetName( assetID ) + " - " + assetID );
	}
	return true;
}

const string cUseOT::AssetGetId(const string & assetName) {
	if(!Init())
		return "";
	if ( nUtils::checkPrefix(assetName) )
		return assetName.substr(1);
	else {
		for(std::int32_t i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
			if(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i))==assetName)
				return OTAPI_Wrap::GetAssetType_ID (i);
		}
	}
	return "";
}

const string cUseOT::AssetGetContract(const string & asset){
	if(!Init())
		return "";
	string strContract = OTAPI_Wrap::GetAssetType_Contract( AssetGetId(asset) );
	return strContract;
}

const string cUseOT::AssetGetDefault(){
	return mDefaultIDs.at(nUtils::eSubjectType::Asset);
}

bool cUseOT::AssetIssue(const string & serverID, const string & nymID, bool dryrun) { // Issue new asset type
	_fact("asset ls");
	if(dryrun) return true;
	if(!Init()) return false;

	string signedContract;
	_dbg3("Message is empty, starting text editor");
	nUtils::cEnvUtils envUtils;
	signedContract = envUtils.Compose();

	string strResponse = mMadeEasy.issue_asset_type(serverID, nymID, signedContract);

	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to issue asset at Server.");
		return false;
	}
	return true;
}

bool cUseOT::AssetNew(const string & nym, bool dryrun) {
	_fact("asset new for nym=" << nym);
	if(dryrun) return true;
	if(!Init()) return false;
	string xmlContents;
	nUtils::cEnvUtils envUtils;
	xmlContents = envUtils.Compose();

	nUtils::DisplayStringEndl(cout, OTAPI_Wrap::CreateAssetContract(NymGetId(nym), xmlContents) ); //TODO save contract to file
	return true;
}

bool cUseOT::AssetRemove(const string & asset, bool dryrun) {
	_fact("asset rm " << asset);
	if(dryrun) return true;
	if(!Init()) return false;

	string assetID = AssetGetId(asset);
	if ( OTAPI_Wrap::Wallet_CanRemoveAssetType(assetID) ) {
		if ( OTAPI_Wrap::Wallet_RemoveAssetType(assetID) ) {
			_info("Asset was deleted successfully");
			return true;
		}
	}
	_warn("Asset cannot be removed");
	return false;
}

bool cUseOT::AssetSetDefault(const std::string & asset, bool dryrun){
	_fact("asset set-default " << asset);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Asset) = AssetGetId(asset);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::CashWithdraw(const string & account, int64_t amount, bool dryrun) { ///< withdraw cash from account on server into local purse
	_fact("cash withdraw " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);

	string response = mMadeEasy.withdraw_cash ( mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountID, amount);//TODO pass server as an argument
	if (1 != mMadeEasy.VerifyMessageSuccess(response) ) {
		_erro("Failed trying to withdraw cash from account: " << AccountGetName(accountID) );
		return false;
	}
	_info("Successfully withdraw cash from account: " << AccountGetName(accountID));
	return true;
}


const string cUseOT::ContractSign(const std::string & nymID, const std::string & contract){ // FIXME can't sign contract with this (assetNew() functionality)
	if(!Init())
		return "";
	return OTAPI_Wrap::AddSignature(nymID, contract);
}

const vector<string> cUseOT::MsgGetAll() { ///< Get all messages from all Nyms. FIXME unused
	if(!Init())
	return vector<string> {};

	for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
		MsgDisplayForNym( NymGetName( OTAPI_Wrap::GetNym_ID(i) ), false );
	}
	return vector<string> {};
}

bool cUseOT::MsgDisplayForNym(const string & nymName, bool dryrun) { ///< Get all messages from Nym.
	_fact("msg ls " << nymName);
	if (dryrun) return false;
	if(!Init())	return false;
	string nymID = NymGetId(nymName);

	bprinter::TablePrinter tpIn(&std::cout);
  tpIn.AddColumn("ID", 4);
  tpIn.AddColumn("From", 20);
  tpIn.AddColumn("Content", 50);

  nUtils::DisplayStringEndl( cout, NymGetName(nymID) + "(" + nymID + ")" );
	nUtils::DisplayStringEndl( cout, "INBOX" );
	tpIn.PrintHeader();
	for(int i = 0 ; i < OTAPI_Wrap::GetNym_MailCount (nymID);i++) {
		tpIn << i << NymGetName(OTAPI_Wrap::GetNym_MailSenderIDByIndex(nymID, i)) << OTAPI_Wrap::GetNym_MailContentsByIndex(nymID,i);
	}
	tpIn.PrintFooter();

  bprinter::TablePrinter tpOut(&std::cout);
	tpOut.AddColumn("ID", 4);
	tpOut.AddColumn("To", 20);
	tpOut.AddColumn("Content", 50);

	nUtils::DisplayStringEndl(cout, "OUTBOX");
	tpOut.PrintHeader();
	for(int i = 0 ; i < OTAPI_Wrap::GetNym_OutmailCount (nymID);i++) {
		tpOut << i << NymGetName(OTAPI_Wrap::GetNym_OutmailRecipientIDByIndex(nymID, i)) << OTAPI_Wrap::GetNym_OutmailContentsByIndex(nymID,i);
	}
	tpOut.PrintFooter();
	return true;
}

bool cUseOT::MsgDisplayForNymInbox(const string & nymName, int msg_index, bool dryrun) {
	return cUseOT::MsgDisplayForNymBox( eBoxType::Inbox , nymName, msg_index, dryrun);
}

bool cUseOT::MsgDisplayForNymOutbox(const string & nymName, int msg_index, bool dryrun) {
	return cUseOT::MsgDisplayForNymBox( eBoxType::Outbox , nymName, msg_index, dryrun);
}

bool cUseOT::MsgDisplayForNymBox(eBoxType boxType, const string & nymName, int msg_index, bool dryrun) {
	_fact("msg ls box " << nymName << " " << msg_index);
	using namespace zkr;

	if (dryrun) return false;
	if (!Init()) return false;

	string nymID = NymGetId(nymName);
	string data_msg;
	auto &col1 = cc::fore::lightblue;
	auto &col2 = cc::fore::lightyellow;
	auto &nocol = cc::fore::console;
	auto &col3 = cc::fore::lightred;
	auto &col4 = cc::fore::lightgreen;

	auto errMessage = [](int index, string type)->void {
		cout << cc::fore::red << "No message with index " << index << ". in " << type << cc::fore::console << endl;
		_mark("Lambda: Can't get message with index: " << index << " from " << type);
	};

	cout << col4;
	nUtils::DisplayStringEndl(cout, NymGetName(nymID) + "(" + nymID + ")");
	cout << col1;
	if (boxType == eBoxType::Inbox) {
		nUtils::DisplayStringEndl(cout, "INBOX");

		data_msg = OTAPI_Wrap::GetNym_MailContentsByIndex(nymID, msg_index);

		if (data_msg.empty()) {
			errMessage(msg_index,"inbox");
			return false;
		}

		const string& data_from = NymGetName(OTAPI_Wrap::GetNym_MailSenderIDByIndex(nymID, msg_index));
		const string& data_server = ServerGetName(OTAPI_Wrap::GetNym_MailServerIDByIndex(nymID, msg_index));

		cout << col1 << "          To: " << col2 << nymName << endl;
		cout << col1 << "        From: " << col2 << data_from << endl;
		cout << col1 << " Data server: " << col2 << data_server << endl;

	} else if (boxType == eBoxType::Outbox) {
		nUtils::DisplayStringEndl(cout, "OUTBOX");
		data_msg = OTAPI_Wrap::GetNym_OutmailContentsByIndex(nymID, msg_index);

		if (data_msg.empty() ) {
			errMessage(msg_index,"outbox");
			return false;
		}

		const string& data_to = NymGetName(OTAPI_Wrap::GetNym_OutmailRecipientIDByIndex(nymID, msg_index));
		const string& data_server = ServerGetName(OTAPI_Wrap::GetNym_OutmailServerIDByIndex(nymID, msg_index));

		// printing
		cout << col1 << "          To: " << col2 << nymName << endl;
		cout << col1 << "        From: " << col2 << data_to << endl;
		cout << col1 << " Data server: " << col2 << data_server << endl;

	}

	cout << col1 << "\n     Message: " << col2 << data_msg << endl;
	cout << col1 << "--- end of message ---" << nocol << endl;

	return true;
}

bool cUseOT::MsgSend(const string & nymSender, vector<string> nymRecipient, const string & subject, const string & msg, int prio, bool dryrun) {
	_fact("MsgSend " << nymSender << " to " << DbgVector(nymRecipient) << " msg=" << msg << " subj="<<subject<<" prio="<<prio);
	if(dryrun) return true;
	if(!Init()) return false;

	string outMsg;

	if ( msg.empty() ) {
		_dbg3("Message is empty, starting text editor");
		nUtils::cEnvUtils envUtils;
		outMsg = envUtils.Compose();
	}
	else
		outMsg = msg;

	if ( outMsg.empty() ) {
		_warn("Can't send the message: message is empty");
		return false;
	}

	ID senderID = NymGetId(nymSender);
	vector<ID> recipientID;
	for (auto varName : nymRecipient)
		recipientID.push_back( NymGetId(varName) );

	for (auto varID : recipientID) {
		_dbg1("Sending message from " + senderID + " to " + varID + "using server " + nUtils::SubjectType2String(nUtils::eSubjectType::Server) );

		string strResponse = mMadeEasy.send_user_msg ( mDefaultIDs.at(nUtils::eSubjectType::Server), senderID, varID, outMsg);

		// -1 error, 0 failure, 1 success.
		if (1 != mMadeEasy.VerifyMessageSuccess(strResponse)) {
			_erro("Failed trying to send the message");
			return false;
		}
		_dbg3("Message from " + senderID + " to " + varID + " was sent successfully.");
	}
	_info("All messages were sent successfully.");
	return true;
}

bool cUseOT::MsgInCheckIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return false;
	if ( nIndex >= 0 && nIndex < OTAPI_Wrap::GetNym_MailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

bool cUseOT::MsgOutCheckIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return false;
	if ( nIndex >= 0 && nIndex < OTAPI_Wrap::GetNym_OutmailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

bool cUseOT::MsgInRemoveByIndex(const string & nymName, const int32_t & index, bool dryrun) {
	_fact("msg rm " << nymName << " index=" << index);
	if (dryrun) return false;
	if(!Init()) return false;
	if(OTAPI_Wrap::Nym_RemoveMailByIndex (NymGetId(nymName), index)){
		_info("Message " << index << " removed successfully from " << nymName << " inbox");
	return true;
	}
	return false;
}

bool cUseOT::MsgOutRemoveByIndex(const string & nymName, const int32_t & index, bool dryrun) {
	_fact("msg rm-out " << nymName << " index=" << index);
	if (dryrun) return false;
	if(!Init()) return false;
	if( OTAPI_Wrap::Nym_RemoveOutmailByIndex(NymGetId(nymName), index) ) {
		_info("Message " << index << " removed successfully from " << nymName << " outbox");
		return true;
	}
	return false;
}

bool cUseOT::NymCheck(const string & nymName, bool dryrun) { // wip
	_fact("nym check " << nymName);
	if (dryrun) return false;
	if(!Init()) return false;

	ID nymID = NymGetId(nymName);

	string strResponse = mMadeEasy.check_user( mDefaultIDs.at(nUtils::eSubjectType::Server), mDefaultIDs.at(nUtils::eSubjectType::User), nymID );
	// -1 error, 0 failure, 1 success.
	if (1 != mMadeEasy.VerifyMessageSuccess(strResponse)) {
		_erro("Failed trying to download public key for nym: " << nymName << "(" << nymID << ")" );
		return false;
	}
	_info("Successfully downloaded user public key for nym: " << nymName << "(" << nymID << ")" );
	return true;
}

bool cUseOT::NymCreate(const string & nymName, bool registerOnServer, bool dryrun) {
	_fact("nym create " << nymName);
	if (dryrun) return false;
	if(!Init()) return false;

	if ( CheckIfExists(nUtils::eSubjectType::User, nymName) ) {
		_erro("Cannot create new nym: '" << nymName << "'. Nym with that name exists" );
		return false;
	}

	int32_t nKeybits = 1024;
	string NYM_ID_SOURCE = ""; //TODO: check
	string ALT_LOCATION = "";
	string nymID = mMadeEasy.create_pseudonym(nKeybits, NYM_ID_SOURCE, ALT_LOCATION);

	if (nymID.empty()) {
		_erro("Failed trying to create new Nym: " << nymName);
		return false;
	}
	// Set the Name of the new Nym.

	if ( !OTAPI_Wrap::SetNym_Name(nymID, nymID, nymName) ) { //Signer Nym? When testing, there is only one nym, so you just pass it twice. But in real production, a user will have a default signing nym, the same way that he might have a default signing key in PGP, and that must be passed in whenever he changes the name on any of the other nyms in his wallet. (In order to properly sign and save the change.)
		_erro("Failed trying to name new Nym: " << nymID);
		return false;
	}
	_info("Nym " << nymName << "(" << nymID << ")" << " created successfully.");
	//	TODO add nym to the cache

	mCache.mNyms.insert( std::make_pair(nymID, nymName) ); // insert nym to nyms cache

	if ( registerOnServer )
		NymRegister(nymName, "^" + ServerGetDefault(), dryrun);
	return true;
}

bool cUseOT::NymExport(const string & nymName, bool dryrun) {
	if(dryrun) return true;
	if(!Init()) return false;

	std::string nymID = NymGetId(nymName);
	_fact("nym export: " << nymName << ", id: " << nymID);

	std::string exported = OTAPI_Wrap::Wallet_ExportNym(nymID);
	// FIXME Bug in OTAPI? Can't export nym twice
	std::cout << zkr::cc::fore::lightblue << exported << zkr::cc::console ;

	return true;
}

bool cUseOT::NymImport(bool dryrun) {
	if(dryrun) return true;
	if(!Init()) return false;

	std:string toImport;
	nUtils::cEnvUtils envUtils;
	toImport = envUtils.Compose();

	if( toImport.empty() ) {
		_warn("Can't import, empty input");
		return false;
	}

	std::string nym = OTAPI_Wrap::Wallet_ImportNym(toImport);
	cout << nym << endl;
	return true;
}

void cUseOT::NymGetAll(bool force) {
	if(!Init())
		return;

	if (force || mCache.mNyms.size() != OTAPI_Wrap::GetNymCount()) { //TODO optimize?
		mCache.mNyms.clear();
		_dbg3("Reloading nyms cache");
		for(int i = 0 ; i < OTAPI_Wrap::GetNymCount();i++) {
			string nym_ID = OTAPI_Wrap::GetNym_ID (i);
			string nym_Name = OTAPI_Wrap::GetNym_Name (nym_ID);

			mCache.mNyms.insert( std::make_pair(nym_ID, nym_Name) );
		}
	}
}

const vector<string> cUseOT::NymGetAllIDs() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> IDs;
	for (auto val : mCache.mNyms) {
		IDs.push_back(val.first);
	}
	return IDs;
}

const vector<string> cUseOT::NymGetAllNames() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> names;
	for (auto val : mCache.mNyms) {
		names.push_back(val.second);
	}
	return names;
}

bool cUseOT::NymDisplayAll(bool dryrun) {
	_fact("nym ls ");
	if(dryrun) return true;
	if(!Init()) return false;

	NymGetAll();
	nUtils::DisplayMap(cout, mCache.mNyms);// display Nyms cache

	return true;
}

const string cUseOT::NymGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::User);
}

const string cUseOT::NymGetId(const string & nymName) { // Gets nym aliases and IDs begins with '^'
	if(!Init())
		return "";

	if ( nUtils::checkPrefix(nymName) ) // nym ID
		return nymName.substr(1);
	else { // look in cache
		string key = nUtils::FindMapValue(mCache.mNyms, nymName);
		if(!key.empty()){
			_dbg3("Found nymID in cache");
			return key;
		}
	}
//
	for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
		string nymID = OTAPI_Wrap::GetNym_ID (i);
		string nymName_ = OTAPI_Wrap::GetNym_Name (nymID);
		if (nymName_ == nymName)
			return nymID;
	}
	return "";
}

bool cUseOT::NymDisplayInfo(const string & nymName, bool dryrun) {
	_fact("nym info " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	cout << OTAPI_Wrap::GetNym_Stats( NymGetId(nymName) );
	return true;
}

const string cUseOT::NymGetName(const ID & nymID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetNym_Name(nymID);
}

bool cUseOT::NymRefresh(const string & nymName, bool all, bool dryrun) { //TODO arguments for server, all servers
	_fact("nym refresh " << nymName << " all?=" << all);
	if(dryrun) return true;
	if(!Init()) return false;

	int32_t serverCount = OTAPI_Wrap::GetServerCount();
	if (all) {
		int32_t nymsRetrieved = 0;
		int32_t nymCount = OTAPI_Wrap::GetNymCount();
		if (nymCount == 0){
			_warn("No Nyms to retrieve");
			return true;
		}

		for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex) { // FIXME Working for all available servers!
			for (int32_t nymIndex = 0; nymIndex < nymCount; ++nymIndex) {
				ID nymID = OTAPI_Wrap::GetNym_ID(nymIndex);
				ID serverID = OTAPI_Wrap::GetServer_ID(serverIndex);
				if (OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)) {
					if ( mMadeEasy.retrieve_nym(serverID, nymID, true) ){ // forcing download
						_info("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval success from server " + ServerGetName(serverID) + "(" + serverID +  ")");
						++nymsRetrieved;
					} else
					_erro("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval failure from server " + ServerGetName(serverID) + "(" + serverID +  ")");
				}
			}
		}
		string count = to_string(nymsRetrieved) + "/" + to_string(nymCount);
		if (nymsRetrieved == nymCount) {
			_info("All Nyms were successfully retrieved " << count);
			return true;
		} else if (nymsRetrieved == 0) {
			_erro("Nyms retrieval failure " << count);
			return false;
		} else {
			_erro("Some Nyms cannot be retrieved (not registered?) " << count); //TODO check if nym is regstered on server
			return true;
		}
	}
	else {
		ID nymID = NymGetId(nymName);
		for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex) { // Working for all available servers!
			ID serverID = OTAPI_Wrap::GetServer_ID(serverIndex);
			if (OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)) {
				if ( mMadeEasy.retrieve_nym(serverID,nymID, true) ) { // forcing download
					_info("Nym " + nymName + "(" + nymID +  ")" + " retrieval success from server " + ServerGetName(serverID) + "(" + serverID +  ")");
					return true;
				}
				_warn("Nym " + nymName + "(" + nymID +  ")" + " retrieval failure from server " + ServerGetName(serverID) + "(" + serverID +  ")");
				return false;
			}
		}
	}
	return false;
}

bool cUseOT::NymRegister(const string & nymName, const string & serverName, bool dryrun) {
	_fact("nym register " << nymName << " on server " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymID = NymGetId(nymName);
	ID serverID = ServerGetId(serverName);

	if (!OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)) {
		string response = mMadeEasy.register_nym(serverID, nymID);
		nOT::nUtils::DisplayStringEndl(cout, response);
		_info("Nym " << nymName << "(" << nymID << ")" << " was registered successfully on server");
		return true;
	}
	_info("Nym " << nymName << "(" << nymID << ")" << " was already registered" << endl);
	return true;
}

bool cUseOT::NymRemove(const string & nymName, bool dryrun) {
	_fact("nym rm " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string nymID = NymGetId(nymName);
	if ( OTAPI_Wrap::Wallet_CanRemoveNym(nymID) ) {
		if ( OTAPI_Wrap::Wallet_RemoveNym(nymID) ) {
			_info("Nym " << nymName  <<  "(" << nymID << ")" << " was deleted successfully");
			mCache.mNyms.erase(nymID);
			return true;
		}
	}
	_warn("Nym " << nymName  <<  "(" << nymID << ")" << " cannot be removed");
	return false;
}

bool cUseOT::NymSetName(const ID & nymID, const string & newNymName) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	if(!Init()) return false;

	if ( !OTAPI_Wrap::SetNym_Name(nymID, nymID, newNymName) ) {
		_erro("Failed trying to set name " << newNymName << " to nym " << nymID);
		return false;
	}
	_info("Set Nym " << nymID << " name to " << newNymName);
	return true;
}

bool cUseOT::NymRename(const string & nym, const string & newNymName, bool dryrun) {
	_fact("nym rename from " << nym << " to " << newNymName);
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymID = NymGetId(nym);

	if( NymSetName(nymID, newNymName) ) {
		_info("Nym " << NymGetName(nymID) << "(" << nymID << ")" << " renamed to " << newNymName);
		mCache.mNyms.insert( std::make_pair(nymID, newNymName) ); // insert nym to nyms cache
		return true;
	}
	_erro("Failed to rename Nym " << NymGetName(nymID) << "(" << nymID << ")" << " to " << newNymName);
	return false;
}

bool cUseOT::NymSetDefault(const string & nymName, bool dryrun) {
	_fact("nym set-default " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::User) = NymGetId(nymName);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

bool cUseOT::ServerAdd(bool dryrun) {
	_fact("server ls");
	if(dryrun) return true;
	if(!Init()) return false;

	string contract;
	nUtils::cEnvUtils envUtils;
	contract = envUtils.Compose();

	if (!contract.empty()) {
		if( OTAPI_Wrap::AddServerContract(contract) ) {
			_info("Server added");
			return true;
		}
	}
	else {
		nUtils::DisplayStringEndl(cout, "Provided contract was empty");
		_erro("Provided contract was empty");
	}
	_erro("Failure to add server");
	return false;
}

bool cUseOT::ServerCreate(const string & nym, bool dryrun) {
	_fact("server new for nym=" << nym);
	if(dryrun) return true;
	if(!Init()) return false;

	string xmlContents;
	nUtils::cEnvUtils envUtils;
	xmlContents = envUtils.Compose();

	ID nymID = NymGetId(nym);
	ID serverID = OTAPI_Wrap::CreateServerContract(nymID, xmlContents);
	if( !serverID.empty() ) {
		_info( "Contract created for Nym: " << NymGetName(nymID) << "(" << nymID << ")" );
		nUtils::DisplayStringEndl( cout, OTAPI_Wrap::GetServer_Contract(serverID) );
		return true;
	}
	_erro( "Failure to create contract for nym: " << NymGetName(nymID) << "(" << nymID << ")" );
	return false;
}

void cUseOT::ServerCheck() {
	if(!Init()) return;

	if( !OTAPI_Wrap::checkServerID( mDefaultIDs.at(nUtils::eSubjectType::Server), mDefaultIDs.at(nUtils::eSubjectType::User) ) ) {
		_erro( "No response from server: " + mDefaultIDs.at(nUtils::eSubjectType::Server) );
	}
	_info("Server " + mDefaultIDs.at(nUtils::eSubjectType::Server) + " is OK");
}

const string cUseOT::ServerGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::Server);
}

const string cUseOT::ServerGetId(const string & serverName) { ///< Gets nym aliases and IDs begins with '%'
	if(!Init())
		return "";

	if ( nUtils::checkPrefix(serverName) )
		return serverName.substr(1);
	else {
		for(int i = 0 ; i < OTAPI_Wrap::GetServerCount(); i++) {
			string serverID = OTAPI_Wrap::GetServer_ID(i);
			string serverName_ = OTAPI_Wrap::GetServer_Name(serverID);
			if (serverName_ == serverName)
				return serverID;
		}
	}
	return "";
}

const string cUseOT::ServerGetName(const string & serverID){
	if(!Init())
		return "";
	return OTAPI_Wrap::GetServer_Name(serverID);
}

bool cUseOT::ServerRemove(const string & serverName, bool dryrun) {
	_fact("server rm " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;
	string serverID = ServerGetId(serverName);
	if ( OTAPI_Wrap::Wallet_CanRemoveServer(serverID) ) {
		if ( OTAPI_Wrap::Wallet_RemoveServer(serverID) ) {
			_info("Server " << serverName << " was deleted successfully");
			return true;
		}
		_warn("Failed to remove server " << serverName);
		return false;
	}
	_warn("Server " << serverName << " cannot be removed");
	return false;
}

bool cUseOT::ServerSetDefault(const string & serverName, bool dryrun) {
	_fact("server set-default " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;

	mDefaultIDs.at(nUtils::eSubjectType::Server) = ServerGetId(serverName);
	// Save defaults to config file:
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	return true;
}

const vector<string> cUseOT::ServerGetAllNames() { ///< Gets all servers name
	if(!Init())
	return vector<string> {};

	vector<string> servers;
	for(int i = 0 ; i < OTAPI_Wrap::GetServerCount ();i++) {
		string servID = OTAPI_Wrap::GetServer_ID(i);
		servers.push_back(servID);
	}
	return servers;
}

bool cUseOT::ServerDisplayAll(bool dryrun) {
	_fact("server ls");
	if(dryrun) return true;
	if(!Init()) return false;

	for(std::int32_t i = 0 ; i < OTAPI_Wrap::GetServerCount();i++) {
		ID serverID = OTAPI_Wrap::GetServer_ID(i);
		nUtils::DisplayStringEndl(cout, ServerGetName( serverID ) + " - " + serverID );
	}
	return true;
}

bool cUseOT::TextEncode(const string & plainText, bool dryrun) {
	_fact("text encode");
	if(dryrun) return true;
	if(!Init()) return false;

	string plainTextIn;
	if ( plainText.empty() ) {
		nUtils::cEnvUtils envUtils;
		plainTextIn = envUtils.Compose();
	}
	else
		plainTextIn = plainText;

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string encodedText;
	encodedText = OTAPI_Wrap::Encode (plainTextIn, bLineBreaks);
	nUtils::DisplayStringEndl(cout, encodedText);
	return true;
}

bool cUseOT::TextEncrypt(const string & recipientNymName, const string & plainText, bool dryrun) {
	_fact("text encrypt to " << recipientNymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string plainTextIn;
	if ( plainText.empty() ) {
		nUtils::cEnvUtils envUtils;
		plainTextIn = envUtils.Compose();
	}
	else
		plainTextIn = plainText;

	string encryptedText;
	encryptedText = OTAPI_Wrap::Encrypt(NymGetId(recipientNymName), plainTextIn);
	nUtils::DisplayStringEndl(cout, encryptedText);
	return true;
}

bool cUseOT::TextDecode(const string & encodedText, bool dryrun) {
	_fact("text decode");
	if(dryrun) return true;
	if(!Init()) return false;

	string encodedTextIn;
	if ( encodedText.empty() ) {
		nUtils::cEnvUtils envUtils;
		encodedTextIn = envUtils.Compose();
	}
	else
		encodedTextIn = encodedText;

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string plainText;
	plainText = OTAPI_Wrap::Decode (encodedTextIn, bLineBreaks);
	nUtils::DisplayStringEndl(cout, plainText);
	return true;
}

bool cUseOT::TextDecrypt(const string & recipientNymName, const string & encryptedText, bool dryrun) {
	_fact("text decrypt for " << recipientNymName );
	if(dryrun) return true;
	if(!Init()) return false;

	string encryptedTextIn;
		if ( encryptedText.empty() ) {
			nUtils::cEnvUtils envUtils;
			encryptedTextIn = envUtils.Compose();
		}
		else
			encryptedTextIn = encryptedText;

	string plainText;
	plainText = OTAPI_Wrap::Decrypt(NymGetId(recipientNymName), encryptedTextIn);
	nUtils::DisplayStringEndl(cout, plainText);
	return true;
}

bool cUseOT::OTAPI_loaded = false;
bool cUseOT::OTAPI_error = false;

} // nUse
}; // namespace OT


