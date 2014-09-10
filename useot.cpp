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

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3 // <=== namespaces


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
	if (!Init()) return false;
	if (subject.empty()) {
		_erro("Subject identifier is empty");
		return false;
	}

	ID subjectID = (this->*cUseOT::subjectGetIDFunc.at(type))(subject);

	if (!subjectID.empty()) {
		_dbg3("Account " + subject + " exists");
		return true;
	}
	_warn("Can't find this Account: " + subject);
	return false;
}

vector<ID> cUseOT::AccountGetAllIds() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving accounts ID's");
	vector<string> accountsIDs;
	for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
		accountsIDs.push_back(OTAPI_Wrap::GetAccountWallet_ID (i));
	}
	return accountsIDs;
}

int64_t cUseOT::AccountGetBalance(const string & accountName) {
	if(!Init()) return 0; //FIXME

	int64_t balance = OTAPI_Wrap::GetAccountWallet_Balance ( AccountGetId(accountName) );
	return balance;
}

string cUseOT::AccountGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::Account);
}

ID cUseOT::AccountGetId(const string & accountName) {
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

string cUseOT::AccountGetName(const ID & accountID) {
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

vector<string> cUseOT::AccountGetAllNames() {
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

vector<string> cUseOT::AssetGetAllNames() {
	if(!Init())
	return vector<string> {};

	vector<string> assets;
	for(int32_t i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
		assets.push_back(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i)));
	}
	return assets;
}

string cUseOT::AssetGetName(const ID & assetID) {
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
		nUtils::DisplayStringEndl(cout, assetID + " - " + AssetGetName( assetID ) );
	}
	return true;
}

string cUseOT::AssetGetId(const string & assetName) {
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

string cUseOT::AssetGetContract(const string & asset){
	if(!Init())
		return "";
	string strContract = OTAPI_Wrap::GetAssetType_Contract( AssetGetId(asset) );
	return strContract;
}

string cUseOT::AssetGetDefault(){
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

bool cUseOT::CashExportWrap(const ID & nymSender, const ID & nymRecipient, const string & account, bool dryrun) {
	_fact("cash export from " << nymSender << " to " << nymRecipient << " account " << account );
	if(dryrun) return true;
	if(!Init()) return false;

	ID nymSenderID = NymGetId(nymSender);
	ID nymRecipientID = NymGetId(nymRecipient);
	ID accountID = NymGetId(account);

	string indices = "";
	string retained_copy = "";
	bool passwordProtected = false;

	string exportedCash = CashExport( nymSenderID, nymRecipientID, account, indices, passwordProtected, retained_copy);

	if (exportedCash.empty()) {
		_erro("Exported string is empty");
		return false;
	}

	DisplayStringEndl(cout, exportedCash);
	return true;
}

string cUseOT::CashExport(const ID & nymSenderID, const ID & nymRecipientID, const string & account, const string & indices, const bool passwordProtected, string & retained_copy) {
	_fact("cash export from " << nymSenderID << " to " << nymRecipientID << " account " << account << " indices: " << indices << "passwordProtected: " << passwordProtected);
	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

	string contract = mMadeEasy.load_or_retrieve_contract(accountServerID, nymSenderID, accountAssetID);

	if (contract.empty()) {
		_erro("Unable to load asset contract: " + accountAssetID);
		DisplayStringEndl(cout, "Unable to load asset contract: " + accountAssetID);
		return "";
	}

	string purseValue = OTAPI_Wrap::LoadPurse(accountServerID, accountAssetID, nymSenderID); // returns NULL, or a purse.

	if (purseValue.empty()) {
		_erro("Unable to load purse from local storage. Does it even exist?");
		DisplayStringEndl(cout,  "Unable to load purse from local storage. Does it even exist?");
		return "";
	}

  int32_t count = OTAPI_Wrap::Purse_Count(accountServerID, accountAssetID, purseValue);
	if (count < 0) { // TODO check if integer?
		DisplayStringEndl(cout, "Error: Unexpected bad value returned from OT_API_Purse_Count.");
		_erro("Unexpected bad value returned from OT_API_Purse_Count.");
		return "";
	}
  if (count < 1){
		DisplayStringEndl(cout, "Error: The purse is empty. Export aborted.");
		_erro("The purse is empty. Export aborted.");
		return "";
  }

	string exportedCash = mMadeEasy.export_cash(accountServerID, nymSenderID, accountAssetID, nymRecipientID, indices, passwordProtected, retained_copy);
	_info("Cash was exported");
	return exportedCash;
}

bool cUseOT::CashImport(const string & nym, bool dryrun) {
                               //optional?
	//TODO add input from file support
	_fact("cash import ");
	if (dryrun) return false;
	if (!Init()) return false;

	ID nymID = NymGetId(nym);

	_dbg3("Open text editor for user to paste payment instrument");
	nUtils::cEnvUtils envUtils;
	string instrument = envUtils.Compose();

	if (instrument.empty()) {
		return false;
	}

	string instrumentType = OTAPI_Wrap::Instrmnt_GetType(instrument);

	if (instrumentType.empty()) {
		OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine instrument type. Expected (cash) PURSE.\n");
		return false;
	}

	string serverID = OTAPI_Wrap::Instrmnt_GetServerID(instrument);

	if (serverID.empty()) {
			OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine server ID from purse.\n");
			return false;
	}

	if ("PURSE" == instrumentType) {
	}
	// Todo: case "TOKEN"
	//
	// NOTE: This is commented out because since it is guessing the NymID as MyNym,
	// then it will just create a purse for MyNym and import it into that purse, and
	// then later when doing a deposit, THAT's when it tries to DECRYPT that token
	// and re-encrypt it to the SERVER's nym... and that's when we might find out that
	// it never was encrypted to MyNym in the first place -- we had just assumed it
	// was here, when we did the import. Until I can look at that in more detail, it
	// will remain commented out.
	else {
			//            // This version supports cash tokens (instead of purse...)
			//            bool bImportedToken = importCashPurse(strServerID, MyNym, strAssetID, userInput, isPurse)
			//
			//            if (bImportedToken)
			//            {
			//                OTAPI_Wrap::Output(0, "\n\n Success importing cash token!\nServer: "+strServerID+"\nAsset Type: "+strAssetID+"\nNym: "+MyNym+"\n\n");
			//                return 1;
			//; }

			OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine instrument type. Expected (cash) PURSE.\n");
			return false;
	}

	// This tells us if the purse is password-protected. (Versus being owned
	// by a Nym.)
	bool hasPassword = OTAPI_Wrap::Purse_HasPassword(serverID, instrument);

	// Even if the Purse is owned by a Nym, that Nym's ID may not necessarily
	// be present on the purse itself (it's optional to list it there.)
	// OTAPI_Wrap::Instrmnt_GetRecipientUserID tells us WHAT the recipient User ID
	// is, IF it's on the purse. (But does NOT tell us WHETHER there is a
	// recipient. The above function is for that.)
	//
	ID purseOwner = "";

	if (!hasPassword) {
			purseOwner = OTAPI_Wrap::Instrmnt_GetRecipientUserID(instrument); // TRY and get the Nym ID (it may have been left blank.)
	}

	// Whether the purse was password-protected (and thus had no Nym ID)
	// or whether it does have a Nym ID (but it wasn't listed on the purse)
	// Then either way, in those cases strPurseOwner will still be NULL.
	//
	// (The third case is that the purse is Nym protected and the ID WAS available,
	// in which case we'll skip this block, since we already have it.)
	//
	// But even in the case where there's no Nym at all (password protected)
	// we STILL need to pass a Signer Nym ID into OTAPI_Wrap::Wallet_ImportPurse.
	// So if it's still NULL here, then we use --mynym to make the call.
	// And also, even in the case where there IS a Nym but it's not listed,
	// we must assume the USER knows the appropriate NymID, even if it's not
	// listed on the purse itself. And in that case as well, the user can
	// simply specify the Nym using --mynym.
	//
	// Bottom line: by this point, if it's still not set, then we just use
	// MyNym, and if THAT's not set, then we return failure.
	//
	if (purseOwner.empty()) {
			OTAPI_Wrap::Output(0, "\n\n The NymID isn't evident from the purse itself... (listing it is optional.)\nThe purse may have no Nym at all--it may instead be password-protected.) Either way, a signer nym is still necessary, even for password-protected purses.\n\n Trying MyNym...\n");
			purseOwner = nymID;
	}

	string assetID = OTAPI_Wrap::Instrmnt_GetAssetID(instrument);

	if (assetID.empty()) {
			OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine asset type ID from purse.\n");
			return false;
	}

	bool imported = OTAPI_Wrap::Wallet_ImportPurse(serverID, assetID, purseOwner, instrument);

	if (imported) {
			OTAPI_Wrap::Output(0, "\n\n Success importing purse!\nServer: " + serverID + "\nAsset Type: " + assetID + "\nNym: " + purseOwner + "\n\n");
			return true;
	}

	return false;
}

bool cUseOT::CashDeposit(const string & accountID, const string & nymFromID, const string & serverID,  const string & instrument, bool dryrun){
	//FIXME cleaning arguments
	_fact("Deposit purse to account: " << accountID);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
	//ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

	string purseValue = instrument;

	if (instrument.empty()) {
			// LOAD PURSE
			_dbg3("Loading purse");
			purseValue = OTAPI_Wrap::LoadPurse(serverID, accountAssetID, nymFromID); // returns NULL, or a purse.

			if (purseValue.empty()) {
					OTAPI_Wrap::Output(0, " Unable to load purse from local storage. Does it even exist?\n");
					return false;
			}
	}

	_dbg3("Processing cash deposit to account");
	int32_t nResult = mMadeEasy.deposit_cash(serverID, accountNymID, accountID, purseValue); // TODO pass reciever nym if exists in purse //<<<------------------------- BROKEN???
	if (nResult < 1) {
		DisplayStringEndl(cout, "Unable to deposit purse");
		return false;
	}
	return true;
}

bool cUseOT::CashSend(const string & nymSender, const string & nymRecipient, const string & account, int64_t amount, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("cash send from " << nymSender << " to " << nymRecipient << " account " << account );
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

	ID nymSenderID = NymGetId(nymSender);
	ID nymRecipientID = NymGetId(nymRecipient);

	_info("Withdrawing cash from account: " << account << " amount: " << amount);
	bool withdrawalSuccess = CashWithdraw(account, amount, false);
	if (!withdrawalSuccess) {
		_erro("Withdrawal failed");
		DisplayStringEndl(cout, "Withdrawal from account: " + account + "failed");
		return false;
	}
	string retainedCopy = "";
	string indices = "";
	bool passwordProtected = false; // TODO check if password protected
	string exportedCashPurse = CashExport(nymSenderID, nymRecipientID, account, indices, passwordProtected, retainedCopy);
	_mark(exportedCashPurse);
	if (!exportedCashPurse.empty()) {
		string response = mMadeEasy.send_user_cash(accountServerID, nymSenderID, nymRecipientID, exportedCashPurse, retainedCopy);

		int32_t returnVal = mMadeEasy.VerifyMessageSuccess(response);

		if (1 != returnVal) {
			// It failed sending the cash to the recipient Nym.
			// Re-import strRetainedCopy back into the sender's cash purse.
			//
			bool bImported = OTAPI_Wrap::Wallet_ImportPurse(accountServerID, accountAssetID, nymSenderID, retainedCopy);

			if (bImported) {
				DisplayStringEndl(cout, "Failed sending cash, but at least: success re-importing purse.\nServer: " + accountServerID + "\nAsset Type: " + accountServerID + "\nNym: " + nymSender + "\n\n");
			}
			else {
				DisplayStringEndl(cout, " Failed sending cash AND failed re-importing purse.\nServer: " + accountServerID + "\nAsset Type: " + accountServerID + "\nNym: " + nymSender + "\n\nPurse (SAVE THIS SOMEWHERE!):\n\n" + retainedCopy);
			}
		}
		else {
			DisplayStringEndl(cout, "Success in sending cash from " + nymSender + " to " + nymRecipient + " account " + account );
		}
	}

	return true;
}

bool cUseOT::CashShow(const string & account, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("cash show for purse with account: " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

	string purseValue = OTAPI_Wrap::LoadPurse(accountServerID, accountAssetID, accountNymID); // returns NULL, or a purse

  if (purseValue.empty()) {
		 _erro("Unable to load purse. Does it even exist?");
		 DisplayStringEndl(cout, "Unable to load purse. Does it even exist?");
		 return false;
	}

  int64_t amount = OTAPI_Wrap::Purse_GetTotalValue(accountServerID, accountAssetID, purseValue);
  cout << zkr::cc::fore::lightyellow << "Total value: " << zkr::cc::fore::red << OTAPI_Wrap::FormatAmount(accountAssetID, amount) << zkr::cc::fore::console << endl;

	int32_t count = OTAPI_Wrap::Purse_Count(accountServerID, accountAssetID, purseValue);
	if (count < 0) { // TODO check if integer?
		DisplayStringEndl(cout, "Error: Unexpected bad value returned from OT_API_Purse_Count.");
		_erro("Unexpected bad value returned from OT_API_Purse_Count.");
		return false;
	}

	if (count > 0) {

		cout << zkr::cc::fore::lightyellow << "Token count: " << zkr::cc::fore::red << count << endl;

		bprinter::TablePrinter tp(&std::cout);
		tp.AddColumn("ID", 4);
		tp.AddColumn("Value", 10);
		tp.AddColumn("Series", 10);
		tp.AddColumn("Valid From", 20);
		tp.AddColumn("Valid To", 20);
		tp.AddColumn("Status", 20);
		tp.PrintHeader();

		int32_t index = -1;
		while (count > 0) { // Loop through purse contents and display tokens
			--count;
			++index;  // on first iteration, this is now 0.

			string token = OTAPI_Wrap::Purse_Peek(accountServerID, accountAssetID, accountNymID, purseValue);
			if (token.empty()) {
				_erro("OT_API_Purse_Peek unexpectedly returned NULL instead of token.");
				return false;
			}

			string newPurse = OTAPI_Wrap::Purse_Pop(accountServerID, accountAssetID, accountNymID, purseValue);

			if (newPurse.empty()) {
				_erro("OT_API_Purse_Pop unexpectedly returned NULL instead of updated purse.\n");
				return false;
			}

			purseValue = newPurse;

			int64_t denomination = OTAPI_Wrap::Token_GetDenomination(accountServerID, accountAssetID, token);
			int32_t series = OTAPI_Wrap::Token_GetSeries(accountServerID, accountAssetID, token);
			time64_t validFrom = OTAPI_Wrap::Token_GetValidFrom(accountServerID, accountAssetID, token);
			time64_t validTo = OTAPI_Wrap::Token_GetValidTo(accountServerID, accountAssetID, token);
			time64_t time = OTAPI_Wrap::GetTime();

			if (denomination < 0){
					_erro( "Error while showing purse: bad denomination");
					return false;
			}
			if (series < 0) {
					_erro("Error while showing purse: bad series");
					return false;
			}
			if (validFrom < 0) {
					_erro("Error while showing purse: bad validFrom");
					return false;
			}
			if (validTo < 0) {
					_erro("Error while showing purse: bad validTo");
					return false;
			}
			if (OT_TIME_ZERO > time) {
					_erro("Error while showing purse: bad time");
					return false;
			}

			string status = (time > validTo) ? "expired" : "valid";

			// Display token
			tp << to_string(index) << to_string(denomination) << to_string(series) << to_string(validFrom) << to_string(validTo) << status;

		} // while
		tp.PrintFooter();
	} // if count > 0
	return true;
}

bool cUseOT::CashWithdraw(const string & account, int64_t amount, bool dryrun) {
	_fact("cash withdraw " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);

	// Make sure the appropriate asset contract is available.
	string assetContract = OTAPI_Wrap::LoadAssetContract(accountAssetID);

	if (assetContract.empty()) {
		string strResponse = mMadeEasy.retrieve_contract(mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountAssetID);

		if (1 != mMadeEasy.VerifyMessageSuccess(strResponse)) {
			_erro( "Unable to retreive asset contract for nym " << accountNymID << " and server " << mDefaultIDs.at(nUtils::eSubjectType::Server) );
			DisplayStringEndl(cout, "Unable to retreive asset contract for nym " + accountNymID + " and server " + mDefaultIDs.at(nUtils::eSubjectType::Server) );
			return false;
		}

		assetContract = OTAPI_Wrap::LoadAssetContract(accountAssetID);

		if (assetContract.empty()) {
			_erro("Failure: Unable to load Asset contract even after retrieving it.");
			DisplayStringEndl(cout, "Failure: Unable to load Asset contract even after retrieving it.");
			return false;
		}
	}

	// Make sure the unexpired mint file is available.
	string mint = mMadeEasy.load_or_retrieve_mint(mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountAssetID);

	if (mint.empty()) {
		_erro("Failure: Unable to load or retrieve necessary mint file for withdrawal.");
		return false;
	}

	// Send withdrawal request
	string response = mMadeEasy.withdraw_cash ( mDefaultIDs.at(nUtils::eSubjectType::Server), accountNymID, accountID, amount);//TODO pass server as an argument

	// Check server response
	if (1 != mMadeEasy.VerifyMessageSuccess(response) ) {
		_erro("Failed trying to withdraw cash from account: " << AccountGetName(accountID) );
		return false;
	}
	_info("Successfully withdraw cash from account: " << AccountGetName(accountID));
	DisplayStringEndl(cout, "Successfully withdraw cash from account: " + AccountGetName(accountID));
	return true;
}

string cUseOT::ContractSign(const std::string & nymID, const std::string & contract){ // FIXME can't sign contract with this (assetNew() functionality)
	if(!Init())
		return "";
	return OTAPI_Wrap::AddSignature(nymID, contract);
}

vector<string> cUseOT::MsgGetAll() { ///< Get all messages from all Nyms. FIXME unused
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

  nUtils::DisplayStringEndl( cout, zkr::cc::fore::lightgreen + NymGetName(nymID) + zkr::cc::fore::console+ "(" + nymID + ")" );
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

bool cUseOT::MsgSend(const string & nymSender, vector<string> nymRecipient, const string & subject, const string & msg, int prio, const string & filename, bool dryrun) {
	_fact("MsgSend " << nymSender << " to " << DbgVector(nymRecipient) << " msg=" << msg << " subj="<<subject<<" prio="<<prio);
	if(dryrun) return true;
	if(!Init()) return false;

	string outMsg;

	if ( !filename.empty() ) {
		_mark("try to load message from file: " << filename);
		nUtils::cEnvUtils envUtils;
		outMsg = envUtils.ReadFromFile(filename);
		_dbg3("loaded message: " << outMsg);
	}

	if ( msg.empty() && outMsg.empty()) {
		_dbg3("Message is empty, starting text editor");
		nUtils::cEnvUtils envUtils;
		outMsg = envUtils.Compose();
	}
	else if(outMsg.empty())
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

bool cUseOT::MsgInCheckIndex(const string & nymName, const int32_t & index) {
	if(!Init())
			return false;
	if ( index >= 0 && index < OTAPI_Wrap::GetNym_MailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

bool cUseOT::MsgOutCheckIndex(const string & nymName, const int32_t & index) {
	if(!Init())
			return false;
	if ( index >= 0 && index < OTAPI_Wrap::GetNym_OutmailCount(NymGetId(nymName)) ) {
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

bool cUseOT::NymImport(const string & filename, bool dryrun) {
	if(dryrun) return true;
	if(!Init()) return false;

	std:string toImport;
	nUtils::cEnvUtils envUtils;

	if(!filename.empty()) {
		_dbg3("Loading from file: " << filename);
		toImport = envUtils.ReadFromFile(filename);
		_dbg3("Loaded: " << toImport);
	}

	if ( toImport.empty()) toImport = envUtils.Compose();
	if( toImport.empty() ) {
		_warn("Can't import, empty input");
		return false;
	}

	std::string nym = OTAPI_Wrap::Wallet_ImportNym(toImport);
	//cout << nym << endl;
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

vector<string> cUseOT::NymGetAllIDs() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> IDs;
	for (auto val : mCache.mNyms) {
		IDs.push_back(val.first);
	}
	return IDs;
}

vector<string> cUseOT::NymGetAllNames() {
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

string cUseOT::NymGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::User);
}

string cUseOT::NymGetId(const string & nymName) { // Gets nym aliases and IDs begins with '^'
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

string cUseOT::NymGetName(const ID & nymID) {
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

bool cUseOT::PaymentAccept(const string & account, const int64_t index, bool dryrun) {
	//case ("CHEQUE")
	//case ("VOUCHER")
	//case ("INVOICE")
	//case ("PURSE")
	// TODO make it work with longer version: asset, server, nym
	// TODO accept all payments
	// TODO accept various instruments types
	_fact("Accept incoming payment nr " << index << " for account " << account);
	if (dryrun) return false;
	if(!Init()) return false;

	ID accountID = AccountGetId(account);
	ID accountNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
	ID accountAssetID = OTAPI_Wrap::GetAccountWallet_AssetTypeID(accountID);
	ID accountServerID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

	//payment instrument and myacct must both have same asset type
  //int32_t nAcceptedPurses = accept_from_paymentbox(strMyAcctID, strIndices, "PURSE");
  // Voucher is already interpreted as a form of cheque, so this is redundant.
  //
  //  int32_t nAcceptedVouchers = accept_from_paymentbox(strMyAcctID, strIndices, "VOUCHER")
  //int32_t nAcceptedCheques = accept_from_paymentbox(strMyAcctID, strIndices, "CHEQUE");


  if (accountID.empty()){
		 OTAPI_Wrap::Output(0, "Failure: strMyAcctID not a valid string.\n");
		 return false;
	}

	if (accountNymID.empty()) {
		 OTAPI_Wrap::Output(0, "Failure: Unable to find NymID based on myacct. Use: --myacct ACCT_ID\n");
		 OTAPI_Wrap::Output(0, "The designated asset account must be yours. OT will find the Nym based on the account.\n\n");
		 return false;
	}

	if (accountServerID.empty()) {
		 OTAPI_Wrap::Output(0, "Failure: Unable to find Server ID based on myacct. Use: --myacct ACCT_ID\n");
		 OTAPI_Wrap::Output(0, "The designated asset account must be yours. OT will find the Server based on the account.\n\n");
		 return false;
	}
	// ******************************************************************
	//
	_dbg3("Loading payment inbox");
	string paymentInbox = OTAPI_Wrap::LoadPaymentInbox(accountServerID, accountNymID); // Returns NULL, or an inbox.

	if (paymentInbox.empty()) {
		 OTAPI_Wrap::Output(0, "\n\n accept_from_paymentbox:  OT_API_LoadPaymentInbox Failed.\n\n");
		 return false;
	}
	_dbg3("Get size of inbox ledger");
	int32_t nCount = OTAPI_Wrap::Ledger_GetCount(accountServerID, accountNymID, accountNymID, paymentInbox);
	if (nCount < 0) {
		 OTAPI_Wrap::Output(0, "Unable to retrieve size of payments inbox ledger. (Failure.)\n");
		 return false;
	}

	//int32_t nIndicesCount = VerifyStringVal(strIndices) ? OTAPI_Wrap::NumList_Count(strIndices) : 0;

	// Either we loop through all the instruments and accept them all, or
	// we loop through all the instruments and accept the specified indices.
	//
	// (But either way, we loop through all the instruments.)
	//
//	for (int32_t nIndex = (nCount - 1); nIndex >= 0; --nIndex) { // Loop from back to front, so if any are removed, the indices remain accurate subsequently.
//		 bool bContinue = false;
//
//		 // - If indices are specified, but the current index is not on
//		 //   that list, then continue...
//		 //
//		 // - If NO indices are specified, accept all the ones matching MyAcct's asset type.
//		 //
//		 if ((nIndicesCount > 0) && !OTAPI_Wrap::NumList_VerifyQuery(strIndices, to_string(nIndex))) {
//				 //          continue  // apparently not supported by the language.
//				 bContinue = true;
//		 }
//		 else if (!bContinue) {
//				 int32_t nHandled = handle_payment_index(strMyAcctID, nIndex, strPaymentType, paymentInbox);
//		 }
//	}
	_dbg3("Get payment instrument");
	string instrument = mMadeEasy.get_payment_instrument(accountServerID, accountNymID, index, paymentInbox); // strInbox is optional and avoids having to load it multiple times. This function will just load it itself, if it has to.
	if (instrument.empty()) {
		OTAPI_Wrap::Output(0, "\n\n Unable to get payment instrument based on index: " + to_string(index) + ".\n\n");
		return false;
	}

	_dbg3("Get type of instrument");
	string strType = OTAPI_Wrap::Instrmnt_GetType(instrument);

	if (strType.empty()) {
			OTAPI_Wrap::Output(0, "\n\nFailure: Unable to determine instrument's type. Expected CHEQUE, VOUCHER, INVOICE, or (cash) PURSE.\n");
			return false;
	}

	// If there's a payment type,
	// and it's not "ANY", and it's the wrong type,
	// then skip this one.
	//
//	if (VerifyStringVal(strPaymentType) && (strPaymentType != "ANY") && (strPaymentType != strType))
//	{
//			if ((("CHEQUE" == strPaymentType) && ("VOUCHER" == strType)) || (("VOUCHER" == strPaymentType) && ("CHEQUE" == strType)))
//			{
//					// in this case we allow it to drop through.
//			}
//			else
//			{
//					OTAPI_Wrap::Output(0, "The instrument " + strIndexErrorMsg + "is not a " + strPaymentType + ". (It's a " + strType + ". Skipping.)\n");
//					return -1;
//			}
//	}

	// But we need to make sure the invoice is made out to strMyNymID (or to no one.)
	// Because if it IS endorsed to a Nym, and strMyNymID is NOT that nym, then the
	// transaction will fail. So let's check, before we bother sending it...

	_dbg3("Get recipient user id");

	// Not all instruments have a specified recipient. But if they do, let's make
	// sure the Nym matches.

	string strRecipientUserID = OTAPI_Wrap::Instrmnt_GetRecipientUserID(instrument);
  if ( !strRecipientUserID.empty() && !CheckIfExists(eSubjectType::User, strRecipientUserID) ) {
  	_erro("The instrument " + to_string(index) + " is endorsed to a specific recipient (" + strRecipientUserID + ") and that doesn't match the account's owner Nym (" + accountNymID + "). (Skipping.)");
  	OTAPI_Wrap::Output(0, "The instrument " + to_string(index) + " is endorsed to a specific recipient (" + strRecipientUserID + ") and that doesn't match the account's owner Nym (" + accountNymID + "). (Skipping.) \n");
  	return false;
  }
  _dbg3("Get instrument assetID");
  string instrumentAssetType = OTAPI_Wrap::Instrmnt_GetAssetID(instrument);

  if (accountAssetID != instrumentAssetType) {
  	OTAPI_Wrap::Output(0, "The instrument at index " + to_string(index) + " has a different asset type than the selected account. (Skipping.) \n");
  	return false;
  }

  _dbg3("Check if instrument is valid");

  time64_t tFrom = OTAPI_Wrap::Instrmnt_GetValidFrom(instrument);
  time64_t tTo = OTAPI_Wrap::Instrmnt_GetValidTo(instrument);
  time64_t tTime = OTAPI_Wrap::GetTime();

  if (tTime < tFrom) {
		OTAPI_Wrap::Output(0, "The instrument at index " + to_string(index) + " is not yet within its valid date range. (Skipping.)\n");
		return false;
	}
	if (tTo > OT_TIME_ZERO && tTime > tTo) {
		OTAPI_Wrap::Output(0, "The instrument at index " + to_string(index) + " is expired. (Moving it to the record box.)\n");

		// Since this instrument is expired, remove it from the payments inbox, and move to record box.
		_dbg3("Expired instument - moving into record inbox");
		// Note: this harvests
		if ((index >= 0) && OTAPI_Wrap::RecordPayment(accountServerID, accountNymID, true, // bIsInbox = true;
				index, true)) {// bSaveCopy = true. (Since it's expired, it'll go into the expired box.)
				return false;
		}
		return false;
	}

	// TODO, IMPORTANT: After the below deposits are completed successfully, the wallet
	// will receive a "successful deposit" server reply. When that happens, OT (internally)
	// needs to go and see if the deposited item was a payment in the payments inbox. If so,
	// it should REMOVE it from that box and move it to the record box.
	//
	// That's why you don't see me messing with the payments inbox even when these are successful.
	// They DO need to be removed from the payments inbox, but just not here in the script. (Rather,
	// internally by OT itself.)
	//
	_dbg3("Checking type of instrument");
	if ("CHEQUE" == strType || "VOUCHER" == strType || "INVOICE" == strType) {
			//return details_deposit_cheque(strServerID, strMyAcctID, strMyNymID, instrument, strType); //TODO implement ChequeDeposit
		return true;
	}

	if ("PURSE" == strType) {
		_dbg3("Type of instrument: " << strType );
		int32_t nDepositPurse = CashDeposit( accountID, accountNymID, accountServerID, instrument, false); // strIndices is left blank in this case
		// if nIndex !=  -1, go ahead and call RecordPayment on the purse at that index, to
		// remove it from payments inbox and move it to the recordbox.
		//
		if ((index != -1) && (1 == nDepositPurse)) {
				int32_t nRecorded = OTAPI_Wrap::RecordPayment(accountServerID, accountNymID, true, //bIsInbox=true
						index, true); // bSaveCopy=true.
		}

		return true;
	}

			OTAPI_Wrap::Output(0, "\nSkipping this instrument: Expected CHEQUE, VOUCHER, INVOICE, or (cash) PURSE.\n");

	return false;
}

bool cUseOT::PaymentShow(const string & nym, const string & server, bool dryrun) { // TODO make it work with longer version: asset, server, nym
	_fact("Show incoming payments inbox for nym: " << nym << " and server: " << server);
	if (dryrun) return false;
	if(!Init()) return false;

	ID nymID = NymGetId(nym);
	ID serverID = ServerGetId(server);

	string paymentInbox = OTAPI_Wrap::LoadPaymentInbox(serverID, nymID); // Returns NULL, or an inbox.

	if (paymentInbox.empty()) {
		DisplayStringEndl(cout, "Unable to load the payments inbox (probably doesn't exist yet.)\n(Nym/Server: " + nym + " / " + server + " )");
		return false;
	}

  int32_t count = OTAPI_Wrap::Ledger_GetCount(serverID, nymID, nymID, paymentInbox);
	if (count > 0) {
		OTAPI_Wrap::Output(0, "Show payments inbox (Nym/Server)\n( " + nym + " / " + server + " )\n");
		OTAPI_Wrap::Output(0, "Idx  Amt   Type      Txn#  Asset_Type\n");
		OTAPI_Wrap::Output(0, "---------------------------------------\n");

		for (int32_t index = 0; index < count; ++index)
		{
			string instrument = OTAPI_Wrap::Ledger_GetInstrument(serverID, nymID, nymID, paymentInbox, index);

			if (instrument.empty()) {
				 OTAPI_Wrap::Output(0, "Failed trying to get payment instrument from payments box.\n");
				 return false;
			}
			string strTrans = OTAPI_Wrap::Ledger_GetTransactionByIndex(serverID, nymID, nymID, paymentInbox, index);
			int64_t lTransNumber = OTAPI_Wrap::Ledger_GetTransactionIDByIndex(serverID, nymID, nymID, paymentInbox, index);

			string strTransID = to_string(lTransNumber);

			int64_t lRefNum = OTAPI_Wrap::Transaction_GetDisplayReferenceToNum(serverID, nymID, nymID, strTrans);

			int64_t lAmount = OTAPI_Wrap::Instrmnt_GetAmount(instrument);
			string strType = OTAPI_Wrap::Instrmnt_GetType(instrument);
			string strAssetType = OTAPI_Wrap::Instrmnt_GetAssetID(instrument);  // todo: output this.
			string strSenderUserID = OTAPI_Wrap::Instrmnt_GetSenderUserID(instrument);
			string strSenderAcctID = OTAPI_Wrap::Instrmnt_GetSenderAcctID(instrument);
			string strRecipientUserID = OTAPI_Wrap::Instrmnt_GetRecipientUserID(instrument);
			string strRecipientAcctID = OTAPI_Wrap::Instrmnt_GetRecipientAcctID(instrument);

			string strUserID = strSenderUserID.empty() ? strSenderUserID : strRecipientUserID;
			string strAcctID = strSenderAcctID.empty() ? strSenderAcctID : strRecipientAcctID;

			bool bHasAmount = lAmount >= 0;
			bool bHasAsset = !strAssetType.empty();

			string strAmount = (bHasAmount && bHasAsset) ? OTAPI_Wrap::FormatAmount(strAssetType, lAmount) : "UNKNOWN_AMOUNT";

			bool bUserIDExists = !strUserID.empty();
			bool bAcctIDExists = !strAcctID.empty();
			bool bAssetIDExists = !strAssetType.empty();

			string strAssetDenoter = (bAssetIDExists ? " - " : "");

			string strAssetName = (bAssetIDExists ? ("\"" + OTAPI_Wrap::GetAssetType_Name(strAssetType) + "\"") : "");
			if ( strAssetName.empty() )     { strAssetName = ""; strAssetDenoter = ""; }

			string strOut1 = to_string(index) + "    ";
			string strOut2 = strAmount + (strAmount.size() < 3 ? "    " : "   ");
			string strOut3 = strType;
			string strOut4 = strType.size() > 10 ? " " : "    ";
			string strOut5 = strTransID + (strTransID.size() < 2 ? "    " : "   ");
			string strOut6 = strAssetType + strAssetDenoter + strAssetName;
			string strOut7 = strRecipientUserID;
			DisplayStringEndl(cout, strOut1 + strOut2 + strOut3 + strOut4 + strOut5 + strOut6 + strOut7);
		} // for
	}


	return true;
}

bool cUseOT::PurseCreate(const string & serverName, const string & asset, const string & ownerName, const string & signerName, bool dryrun) {
	_fact("purse create ");
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	string assetTypeID = AssetGetId(asset);
	string ownerID = NymGetId(ownerName);
	string signerID = NymGetId(signerName);

	string purse = OTAPI_Wrap::CreatePurse(serverID,assetTypeID,ownerID,signerID);
	nUtils::DisplayStringEndl(cout, purse);

	//	bool OTAPI_Wrap::SavePurse	(	const std::string & 	SERVER_ID,
	//	const std::string & 	ASSET_TYPE_ID,
	//	const std::string & 	USER_ID,
	//	const std::string & 	THE_PURSE
	//	)
	bool result = OTAPI_Wrap::SavePurse(serverID,assetTypeID,ownerID,purse);
	_info("saving: " << result) ;

	return true;
}
bool cUseOT::PurseDisplay(const string & serverName, const string & asset, const string & nymName, bool dryrun) {
	_fact("purse show= " << serverName << " " << asset << " " << nymName);
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	string assetTypeID = AssetGetId(asset);
	string nymID = NymGetId(nymName);

	string result = OTAPI_Wrap::LoadPurse(serverID,assetTypeID,nymID);
	nUtils::DisplayStringEndl(cout, result);
	_info(result);
	//  TODO:
//	std::string OTAPI_Wrap::LoadPurse	(	const std::string & 	SERVER_ID,
//	const std::string & 	ASSET_TYPE_ID,
//	const std::string & 	USER_ID
//	)

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

string cUseOT::ServerGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at(nUtils::eSubjectType::Server);
}

string cUseOT::ServerGetId(const string & serverName) { ///< Gets nym aliases and IDs begins with '%'
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

string cUseOT::ServerGetName(const string & serverID){
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
bool cUseOT::ServerShowContract(const string & serverName, bool dryrun) {
	_fact("server show-contract " << serverName);
	if(dryrun) return true;
	if(!Init()) return false;

	string serverID = ServerGetId(serverName);
	nUtils::DisplayStringEndl(cout, zkr::cc::fore::lightblue + serverName + zkr::cc::fore::console);
	nUtils::DisplayStringEndl(cout, "ID: " + serverID);
	nUtils::DisplayStringEndl(cout, OTAPI_Wrap::GetServer_Contract(serverID));
	return true;

}
vector<string> cUseOT::ServerGetAllNames() { ///< Gets all servers name
	if(!Init())
	return vector<string> {};

	vector<string> servers;
	for(int i = 0 ; i < OTAPI_Wrap::GetServerCount ();i++) {
		string servID = OTAPI_Wrap::GetServer_ID(i);
		string servName = OTAPI_Wrap::GetServer_Name(servID);
		servers.push_back(servName);
	}
	return servers;
}

bool cUseOT::ServerDisplayAll(bool dryrun) {
	_fact("server ls");
	if(dryrun) return true;
	if(!Init()) return false;

	for(std::int32_t i = 0 ; i < OTAPI_Wrap::GetServerCount();i++) {
		ID serverID = OTAPI_Wrap::GetServer_ID(i);

		nUtils::DisplayStringEndl(cout, ServerGetName( serverID ) + " - " + serverID);
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
} // namespace OT


