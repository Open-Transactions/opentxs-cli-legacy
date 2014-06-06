/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "useot.hpp"

#include <OTPaths.hpp>

#include "lib_common3.hpp"

namespace nOT {
namespace nUse {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3; // <=== namespaces

cUseOT::cUseOT(const string &mDbgName)
: 
	mDbgName(mDbgName)
, mNymsMy_loaded(false)
, OTAPI_loaded(false)
, OTAPI_error(false)
, mDataFolder( OTPaths::AppDataFolder().Get() )
, mDefaultIDsFile( mDataFolder + "defaults.opt" )
{
	_dbg1("Creating cUseOT "<<DbgName());
}


string cUseOT::DbgName() const noexcept {
	return "cUseOT-" + ToStr((void*)this) + "-" + mDbgName;
}

void cUseOT::CloseApi() {
	if (OTAPI_loaded) {
		_dbg1("Will cleanup OTAPI");
		OTAPI_Wrap::AppCleanup(); // UnInit OTAPI
		_dbg2("Will cleanup OTAPI - DONE");
	} else _dbg3("Will cleanup OTAPI ... was already not loaded");
}

cUseOT::~cUseOT() {
	CloseApi();
}

void cUseOT::LoadDefaults() {
	// TODO What if there is, for example no accounts?
	// TODO Check if defaults are correct.
	if ( !configManager.Load(mDefaultIDsFile, mDefaultIDs) ) {
		_dbg1("Cannot open" + mDefaultIDsFile + " file, setting IDs with ID 0 as default");
		mDefaultIDs["AccountID"] = OTAPI_Wrap::GetAccountWallet_ID(0);
		mDefaultIDs["PurseID"] = OTAPI_Wrap::GetAssetType_ID(0);
		mDefaultIDs["ServerID"] = OTAPI_Wrap::GetServer_ID(0);
		mDefaultIDs["UserID"] = OTAPI_Wrap::GetNym_ID(0);
	}
}

bool cUseOT::Init() {
	if (OTAPI_error) return false;
	if (OTAPI_loaded) { _note("OTAPI was already initialized"); return true; }
	try {
		if (!OTAPI_Wrap::AppInit()) {// Init OTAPI
			_erro("Error while initializing wrapper");
			return false; // <--- RET
		}

		_info("Trying to load wallet now.");
		// if not pWrap it means that AppInit is not successed
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

bool cUseOT::AccountCheckIfExists(const string & accountName) {
	vector<string> v = AccountGetAllNames();
	if (std::find(v.begin(), v.end(), accountName) != v.end()){
		_dbg3("Account " + accountName + " exists");
		return true;
	}
	_warn("Can't find account: " + accountName);
	return false;
}

const vector<string> cUseOT::AccountGetAllIds() {
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
	if(!Init())
		return 0; //FIXME

	int64_t balance = OTAPI_Wrap::GetAccountWallet_Balance	( AccountGetId(accountName) );
	return balance;
}

const string cUseOT::AccountGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("AccountID");
}

const string cUseOT::AccountGetId(const string & accountName) {
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

const string cUseOT::AccountGetName(const string & accountID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetAccountWallet_Name(accountID);
}

void cUseOT::AccountRemove(const string & accountName) { ///<
	if(!Init())
	return;

	if(OTAPI_Wrap::Wallet_CanRemoveAccount (AccountGetId(accountName))) {
		_erro("Account cannot be deleted: doesn't have a zero balance?/outstanding receipts?");
		return;
	}

	if( OTAPI_Wrap::deleteAssetAccount( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), AccountGetId(accountName) ) ) { //FIXME should be
		_erro("Failure deleting account: " + accountName);
		return;
	}
	_info("Account: " + accountName + " was successfully removed");
}

void cUseOT::AccountRefresh(const string & accountName) {
	if(!Init())
		return;

	OT_ME madeEasy;

	ID accountID = AccountGetId(accountName);

	ID acctSvrID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);
	ID acctNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);

	if ( madeEasy.retrieve_account(acctSvrID, acctNymID, accountID, true) ){ // forcing download
		_info("Account " + accountName + "(" + accountID +  ")" + " retrieval success");
		return;
	}
	_warn("Account " + accountName + "(" + accountID +  ")" + " retrieval failure");
}

void cUseOT::AccountRefreshAll() {
	if(!Init())
		return;

	OT_ME madeEasy;

	int32_t nymCount = OTAPI_Wrap::GetAccountCount();
	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex){
		ID serverId = OTAPI_Wrap::GetServer_ID(serverIndex);

		for (int32_t accountIndex = 0; accountIndex < nymCount; ++accountIndex){
			ID accountID = OTAPI_Wrap::GetAccountWallet_ID(accountIndex);

			ID acctNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountID);
			ID acctSvrID = OTAPI_Wrap::GetAccountWallet_ServerID(accountID);

			if ( madeEasy.retrieve_account(acctSvrID, acctNymID, accountID, true) ){ // forcing download
				_info("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval success");
				return;
			}
			_warn("Account " + AccountGetName(accountID) + "(" + accountID +  ")" + " retrieval failure");
		}
	}
}

const string cUseOT::AccountRename(const string & oldAccountName, const string & newAccountName) {

		AccountSetName (AccountGetId(oldAccountName), newAccountName);
	return "";
}

const string cUseOT::AccountSetName(const string & accountID, const string & NewAccountName) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	if(!Init())
	return "";

		OTAPI_Wrap::SetAccountWallet_Name (accountID, mDefaultIDs.at("UserID"), NewAccountName);
	return "";
}

void cUseOT::AccountCreate(const string & assetName, const string & newAccountName) {
	if(!Init())
	return ;

	OT_ME madeEasy;
	string strResponse;
	strResponse = madeEasy.create_asset_acct(mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), AssetGetId(assetName));

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to create Account at Server.");
		return;
	}

	// Get the ID of the new account.
	string strID = OTAPI_Wrap::Message_GetNewAcctID(strResponse);
	if (!strID.size()){
		_erro("Failed trying to get the new account's ID from the server response.");
		return;
	}

	// Set the Name of the new account.
	AccountSetName(strID,newAccountName);

	cout << "Account " << newAccountName << "(" << strID << ")" << " created successfully." << endl;
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

void cUseOT::AccountSetDefault(const string & accountName) {
	if(!Init())
		return ;
	mDefaultIDs.at("AccountID") = AccountGetId(accountName);
}

bool cUseOT::AssetCheckIfExists(const string & assetName) {
	vector<string> v = AssetGetAllNames();
	if (std::find(v.begin(), v.end(), assetName) != v.end()) {
		_dbg3("Asset " + assetName + " exists");
		return true;
	}
	_warn("Can't find that asset: " + assetName);
	return false;
}

const vector<string> cUseOT::AssetGetAllNames() {
	if(!Init())
	return vector<string> {};

	vector<string> assets;
	for(int i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
		assets.push_back(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i)));
	}
	return assets;
}

const string cUseOT::AssetGetId(const string & assetName) {
	if(!Init())
		return "";
	if ( nUtils::checkPrefix(assetName) )
		return assetName.substr(1);
	else {
		for(int i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
			if(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i))==assetName)
				return OTAPI_Wrap::GetAssetType_ID (i);
		}
	}
	return "";
}

const string cUseOT::AssetGetContract(const std::string & assetID){
	if(!Init())
		return "";
	string strContract = OTAPI_Wrap::GetAssetType_Contract(assetID);
	return strContract;
}

const string cUseOT::AssetGetDefault(){
	return mDefaultIDs.at("PurseID");
}

const string cUseOT::AssetIssue(const std::string & serverID, const std::string & nymID, const std::string & signedContract) { // Issue new asset type
	if(!Init())
	return "";

	OT_ME madeEasy;
	//std::string OT_ME::issue_asset_type(const std::string  & SERVER_ID, const std::string  & NYM_ID, const std::string  & THE_CONTRACT)
	string strResponse = madeEasy.issue_asset_type(serverID, nymID, signedContract);

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to issue asset at Server.");
		return "";
	}
	return strResponse;
}

const string cUseOT::AssetNew(const std::string & nymID, const std::string & xmlContents){
	if(!Init())
			return "";
	return OTAPI_Wrap::CreateAssetContract(nymID, xmlContents);
}

void cUseOT::AssetRemove(const string & assetName) {
	if(!Init())
		return ;
	string nymID = AssetGetId(assetName);
	if ( OTAPI_Wrap::Wallet_CanRemoveAssetType(assetName) ) {
		if ( OTAPI_Wrap::Wallet_RemoveAssetType(assetName) )
			_info("Asset was deleted successfully");
		else
			_warn("Asset cannot be removed");
	}
}

void cUseOT::AssetSetDefault(const std::string & assetName){
	if(!Init())
		return ;
	mDefaultIDs.at("PurseID") = AssetGetId(assetName);
}

const string cUseOT::ContractSign(const std::string & nymID, const std::string & contract){ // FIXME can't sign contract with this (assetNew() functionality)
	if(!Init())
		return "";
	return OTAPI_Wrap::AddSignature(nymID, contract);
}

const vector<string> cUseOT::MsgGetAll() { ///< Get all messages from all Nyms.
	if(!Init())
	return vector<string> {};

	for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
		MsgGetForNym( NymGetName( OTAPI_Wrap::GetNym_ID(i) ) );
	}
	return vector<string> {};
}

const vector<string> cUseOT::MsgGetForNym(const string & nymName) { ///< Get all messages from Nym.
	if(!Init())
		return vector<string> {};
	string nymID = NymGetId(nymName);
	cout << "===" << nymName << "(" << nymID << ")"  << "===" << endl;
	cout << "INBOX" << endl;
	cout << "id\tfrom\t\tcontent:" << endl;
	for(int i = 0 ; i < OTAPI_Wrap::GetNym_MailCount (nymID);i++) {
		cout << i << "\t" << OTAPI_Wrap::GetNym_Name(OTAPI_Wrap::GetNym_MailSenderIDByIndex(nymID, i))  << "\t" << OTAPI_Wrap::GetNym_MailContentsByIndex (nymID,i) << endl;
	}
	cout << "OUTBOX" << endl;
	cout << "id\tto\t\tcontent:" << endl;
	for(int i = 0 ; i < OTAPI_Wrap::GetNym_OutmailCount (nymID);i++) {
		cout << i << "\t" << OTAPI_Wrap::GetNym_Name(OTAPI_Wrap::GetNym_OutmailRecipientIDByIndex(nymID, i)) << "\t" << OTAPI_Wrap::GetNym_OutmailContentsByIndex (nymID,i) << endl;
	}

	return vector<string> {};
}

void cUseOT::MsgSend(const string & nymSender, vector<string> nymRecipient, const string & msg, const string & subject, int prio, bool dryrun) {
	_note("MsgSend " << nymSender << " to " << DbgVector(nymRecipient) << " msg=" << msg << " subj="<<subject<<" prio="<<prio);
	if (dryrun) return;
	// TODO
}

void cUseOT::MsgSend(const string & nymSender, const string & nymRecipient, const string & msg) { ///< Send message from Nym1 to Nym2
	_note("MsgSend " << nymSender << " to " << nymRecipient << " msg=" << msg);
	
	if(!Init())
		return;

	if ( !NymCheckIfExists(nymSender) ) {
		_erro("Can't recognize sender name: " + nymSender);
		return;
	}
	if ( !NymCheckIfExists(nymRecipient) ) {
		_erro("Can't recognize recipient name: " + nymRecipient);
		return;
	}

	OT_ME madeEasy;
	string sender = NymGetId(nymSender);
	string recipient = NymGetId(nymRecipient);

	_dbg1("Sending message from" + sender + "to" + recipient );

	string strResponse = madeEasy.send_user_msg ( mDefaultIDs.at("ServerID"), sender, recipient, msg);

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to send the message");
		return;
	}

	_info("Message was sent successfully.");
}

void cUseOT::MsgSend(const string & nymRecipient, const string & msg) { ///< Send message from default Nym to Nym2
	if(!Init())
		return;

	if ( !NymCheckIfExists(nymRecipient) ) {
		_erro("Can't recognize recipient name: " + nymRecipient);
		return;
	}

	OT_ME madeEasy;
	string recipient = NymGetId(nymRecipient);

	_dbg1("Sending message from" + mDefaultIDs.at("UserID") + "to" + nymRecipient);

	string strResponse = madeEasy.send_user_msg ( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), recipient, msg);

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to send the message");
		return;
	}
	_info("Message was sent successfully.");
}

const bool cUseOT::MsgInCheckIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return false;
	if ( nIndex >= 0 && nIndex < OTAPI_Wrap::GetNym_MailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

const bool cUseOT::MsgOutCheckIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return false;
	if ( nIndex >= 0 && nIndex < OTAPI_Wrap::GetNym_OutmailCount(NymGetId(nymName)) ) {
		return true;
	}
	return false;
}

void cUseOT::MsgInRemoveByIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return;
	if(OTAPI_Wrap::Nym_RemoveMailByIndex (NymGetId(nymName), nIndex)){
		_info("Message removed successfully from inbox");
	}
}

void cUseOT::MsgOutRemoveByIndex(const string & nymName, const int32_t & nIndex) {
	if(!Init())
			return;
	if( OTAPI_Wrap::Nym_RemoveOutmailByIndex(NymGetId(nymName), nIndex) ){
		_info("Message removed successfully from outbox");
	}
}

void cUseOT::NymCheck(const string & hisNymID) { // wip
	if(!Init())
		return;

	OT_ME madeEasy;
	string strResponse = madeEasy.check_user( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), hisNymID );
	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to download user public key.");
		return;
	}
	_info("Successfully downloaded user public key.");
}

void cUseOT::NymCreate(const string & nymName) {
	if(!Init())
	return ;

	OT_ME madeEasy;
	int32_t nKeybits = 1024;
	string NYM_ID_SOURCE = ""; //TODO: check
	string ALT_LOCATION = "";
	string strID = madeEasy.create_pseudonym(nKeybits, NYM_ID_SOURCE, ALT_LOCATION);

	if (strID.empty())
	{
		_erro("Failed trying to create new Nym.");
		return;
	}

	// Set the Name of the new Nym.
	OTAPI_Wrap::SetNym_Name(strID, strID, nymName);

	_info("Nym " << nymName << "(" << strID << ")" << " created successfully.");
}

bool cUseOT::NymCheckIfExists(const string & nymName) { ///< Check only name!
	if(!Init())
			return false;
	vector<string> v = NymGetAllNames();
	if (std::find(v.begin(), v.end(), nymName) != v.end()) {
		_dbg3("Nym " + nymName + " exists");
		return true;
	}
	_warn("Can't find this Nym: " + nymName);
	return false;
}

void cUseOT::NymGetAll() {
	if(!Init())
		return;

	if (mNyms.size() != OTAPI_Wrap::GetNymCount()) { //TODO optimize?
		mNyms.clear();

		for(int i = 0 ; i < OTAPI_Wrap::GetNymCount();i++) {
			string nym_ID = OTAPI_Wrap::GetNym_ID (i);
			string nym_Name = OTAPI_Wrap::GetNym_Name (nym_ID);

			mNyms.insert( std::make_pair(nym_ID, nym_Name) );
		}
	}
}

const vector<string> cUseOT::NymGetAllIDs() {
	if(!Init())
		return vector<string> {};
	NymGetAllNames();
	vector<string> IDs;
	for (auto val : mNyms) {
		IDs.push_back(val.first);
	}
	return IDs;
}

const vector<string> cUseOT::NymGetAllNames() {
	if(!Init())
		return vector<string> {};
	NymGetAll();
	vector<string> names;
	for (auto val : mNyms) {
		names.push_back(val.second);
	}
	return names;
}

const string cUseOT::NymGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("UserID");
}

const string cUseOT::NymGetId(const string & nymName) { // Gets nym aliases and IDs begins with '^'
	if(!Init())
		return "";
	if ( nUtils::checkPrefix(nymName) )
		return nymName.substr(1);
	else { // nym Name
		for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
			string nymID = OTAPI_Wrap::GetNym_ID (i);
			string nymName_ = OTAPI_Wrap::GetNym_Name (nymID);
			if (nymName_ == nymName)
				return nymID;
		}
	}
	return "";
}

const string cUseOT::NymGetInfo(const string & nymName) {
	if(!Init())
		return "";

	if (NymCheckIfExists(nymName)){
		return OTAPI_Wrap::GetNym_Stats( NymGetId(nymName) );
	}
	else {
		_erro("Nym not found");
	}

	return "";
}

const string cUseOT::NymGetName(const string & nymID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetNym_Name(nymID);
}

void cUseOT::NymRefresh(const string & nymName) { //TODO arguments for server, all servers
	if(!Init())
		return;

	OT_ME madeEasy;
	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	ID nymID = AccountGetId(nymName);

	for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex){ // Working for all available servers!
		ID serverID = OTAPI_Wrap::GetServer_ID(serverIndex);
		if (OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)){
			if ( madeEasy.retrieve_nym(nymID, serverID, true) ){ // forcing download
				_info("Nym " + nymName + "(" + nymID +  ")" + " retrieval success");
				return;
			}
			_warn("Nym " + nymName + "(" + nymID +  ")" + " retrieval failure");
		}
	}
}

void cUseOT::NymRefreshAll() {
	if(!Init())
		return;

	OT_ME madeEasy;

	int32_t nymCount = OTAPI_Wrap::GetNymCount();
	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex){
		ID serverID = OTAPI_Wrap::GetServer_ID(serverIndex);

		for (int32_t accountIndex = 0; accountIndex < nymCount; ++accountIndex){
			ID nymID = OTAPI_Wrap::GetNym_ID(accountIndex);

			if (OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID)){
				if ( madeEasy.retrieve_nym(nymID, serverID, true) ){ // forcing download
					_info("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval success");
					return;
				}
				_warn("Nym " + NymGetName(nymID) + "(" + nymID +  ")" + " retrieval failure");
			}
		}
	}
}

void cUseOT::NymRegister(const string & nymName, const string & serverName) {
	if(!Init())
	return ;

	OT_ME madeEasy;

	_warn("Checking for default server only");

	ID nymID = NymGetId(nymName);
	ID serverID = ServerGetId(serverName);

	bool isReg = OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, serverID);

	if (!isReg)
	{
		string response = madeEasy.register_nym(serverID, nymID);
		nOT::nUtils::DisplayStringEndl(cout, response);
		nOT::nUtils::DisplayStringEndl(cout, "Nym " + nymName + "(" + nymID + ")" + " was registered successfully on server");
	}
	else
		cout << "Nym " << nymName << "(" << nymID << ")" << " was already registered" << endl;
}

void cUseOT::NymRemove(const string & nymName) {
	if(!Init())
	return ;
	string nymID = NymGetId(nymName);
	if ( OTAPI_Wrap::Wallet_CanRemoveNym(nymID) ) {
		if ( OTAPI_Wrap::Wallet_RemoveNym(nymID) )
			_info("Nym was deleted successfully");
		else
			_warn("Nym cannot be removed");
	}
}

void cUseOT::NymSetDefault(const string & nymName) {
	if(!Init())
		return ;
	mDefaultIDs.at("UserID") = NymGetId(nymName);
}

void cUseOT::ServerAdd(const std::string & contract) {
	if(!Init())
			return ;
	OTAPI_Wrap::AddServerContract( contract );
}

void cUseOT::ServerCheck() { ///< Use it to ping server
	if(!Init())
			return ;

	if( !OTAPI_Wrap::checkServerID( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID") ) ){
		_erro( "No response from server: " + mDefaultIDs.at("ServerID") );
	}
	_info("Server " + mDefaultIDs.at("ServerID") + " is OK");
}

bool cUseOT::ServerCheckIfExists(const string & serverName) {
	vector<string> v = ServerGetAllNames();
	if (std::find(v.begin(), v.end(), serverName) != v.end()) {
		_dbg3("Server " + serverName + " exists");
		return true;
	}
	_warn("Can't find this server: " + serverName);
	return false;
}


const string cUseOT::ServerGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("ServerID");
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

void cUseOT::ServerRemove(const string & serverName) {
	if(!Init())
		return ;
	string nymID = ServerGetId(serverName);
	if ( OTAPI_Wrap::Wallet_CanRemoveServer(serverName) ) {
		if ( OTAPI_Wrap::Wallet_RemoveServer(serverName) )
			_info("Server was deleted successfully");
		else
			_warn("Server cannot be removed");
	}
}


void cUseOT::ServerSetDefault(const string & serverName) {
	if(!Init())
		return ;
	mDefaultIDs.at("ServerID") = ServerGetId(serverName);
	_info("Default server: " + mDefaultIDs.at("ServerID"));
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

const string cUseOT::TextEncode(const string & plainText) {
	if(!Init())
		return "";

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string encodedText;
	encodedText = OTAPI_Wrap::Encode (plainText, bLineBreaks);
	return encodedText;
}

const string cUseOT::TextEncrypt(const string & recipientNymName, const string & plainText) {
	if(!Init())
		return "";
	string encryptedText;
	encryptedText = OTAPI_Wrap::Encrypt(NymGetId(recipientNymName), plainText);
	return encryptedText;
}

const string cUseOT::TextDecode(const string & encodedText) {
	if(!Init())
		return "";

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string plainText;
	plainText = OTAPI_Wrap::Decode (encodedText, bLineBreaks);
	return plainText;
}

const string cUseOT::TextDecrypt(const string & recipientNymName, const string & encryptedText) {
	if(!Init())
		return "";
	string plainText;
	plainText = OTAPI_Wrap::Decrypt(NymGetId(recipientNymName), encryptedText);
	return plainText;
}

} // nUse
}; // namespace OT


