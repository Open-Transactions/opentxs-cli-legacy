/* See other files here for the LICENCE that applies here. */
/* See header file .hpp for info */

#include "useot.hpp"

#include <OTPaths.hpp>

#include "lib_common3.hpp"

namespace nOT {
namespace nUse {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_3; // <=== namespaces

cUseOT::cUseOT()
: mNymsMy_loaded(false)
, OTAPI_loaded(false)
, OTAPI_error(false)
, mDataFolder( OTPaths::AppDataFolder().Get() )
, mDefaultIDsFile( mDataFolder + "defaults.opt" )
{

}

void cUseOT::CloseApi() {
	if (OTAPI_loaded) {
		_dbg1("Will cleanup OTAPI");
		OTAPI_Wrap::AppCleanup(); // UnInit OTAPI
		_dbg2("Will cleanup OTAPI - DONE");
	} else _dbg3("Will cleanup OTAPI ... was already not loaded");
}

void cUseOT::Done() {
	_dbg2("Will save defaults");
	nUtils::configManager.Save(mDefaultIDsFile, mDefaultIDs);
	_dbg3("Will save defaults - DONE");
	CloseApi();
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
	if (OTAPI_loaded) return true;
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

bool cUseOT::accountCheckIfExists(const string & accountName) {
	vector<string> v = accountsGet();
	if (std::find(v.begin(), v.end(), accountName) != v.end())
		return true;
	return false;
}

const int64_t cUseOT::accountGetBalance(const string & accountName) {
	if(!Init())
		return 0; //FIXME

	int64_t balance = OTAPI_Wrap::GetAccountWallet_Balance	( accountGetId(accountName) );
	return balance;
}

const string cUseOT::accountGetId(const string & accountName) {
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

const string cUseOT::accountGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("AccountID");
}

const string cUseOT::accountGetName(const string & accountID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetAccountWallet_Name(accountID);
}

string cUseOT::accountDelete(const string & accountName) { ///<
	if(!Init())
	return "";

	if(!OTAPI_Wrap::Wallet_CanRemoveAccount (accountGetId(accountName))) {
		// inBox and OutBox must be get from server because without it account not work properly
		// example if you can't delete account without inbox and outbox
		int32_t  inBoxInt = OTAPI_Wrap::getInbox 	(mDefaultIDs.at("ServerID"),mDefaultIDs.at("UserID"),accountGetId(accountName));
		int32_t outBoxInt = OTAPI_Wrap::getOutbox 	(mDefaultIDs.at("ServerID"),mDefaultIDs.at("UserID"),accountGetId(accountName));
	}

	if(OTAPI_Wrap::deleteAssetAccount(mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), accountGetId(accountName))==-1)
		return "Error while deleting account";
	else
		return "";
}

const vector<string> cUseOT::accountGetIds() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving accounts ID's");
	vector<string> accountsIDs;
	for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
		accountsIDs.push_back(OTAPI_Wrap::GetAccountWallet_ID (i));
	}
	return accountsIDs;
}

void cUseOT::accountRefreshAll() {
	if(!Init())
		return;

	// Retrieve Accounts based on Moneychanger

	OT_ME madeEasy;

	int32_t nymCount = OTAPI_Wrap::GetAccountCount();

	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex)
	{
		string serverId = OTAPI_Wrap::GetServer_ID(serverIndex);

		for (int32_t accountIndex = 0; accountIndex < nymCount; ++accountIndex)
		{
			std::string accountId = OTAPI_Wrap::GetAccountWallet_ID(accountIndex);

			std::string acctNymID = OTAPI_Wrap::GetAccountWallet_NymID(accountId);
			std::string acctSvrID = OTAPI_Wrap::GetAccountWallet_ServerID(accountId);

			bool bRetrievalAttempted = false;
			bool bRetrievalSucceeded = false;
			{
				bRetrievalAttempted = true;
				bRetrievalSucceeded = madeEasy.retrieve_account(acctSvrID, acctNymID, accountId, true); // forcing download
			}
		}
	}
}

const string cUseOT::accountRename(const string & oldAccountName, const string & newAccountName) {

		accountSetName (accountGetId(oldAccountName), newAccountName);
	return "";
}

const string cUseOT::accountSetName(const string & accountID, const string & NewAccountName) { //TODO: passing to function: const string & nymName, const string & signerNymName,
	if(!Init())
	return "";

		OTAPI_Wrap::SetAccountWallet_Name (accountID, mDefaultIDs.at("UserID"), NewAccountName);
	return "";
}

void cUseOT::accountCreate(const string & assetName, const string & newAccountName) {
	if(!Init())
	return ;

	OT_ME madeEasy;
	string strResponse;
	strResponse = madeEasy.create_asset_acct(mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), assetGetId(assetName));

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
	accountSetName(strID,newAccountName);

	cout << "Account " << newAccountName << "(" << strID << ")" << " created successfully." << endl;
}
void cUseOT::accountSetDefault(const string & accountName) {
	if(!Init())
		return ;
	mDefaultIDs.at("AccountID") = accountGetId(accountName);
}

const vector<string> cUseOT::accountsGet() {
	if(!Init())
	return vector<string> {};

	_dbg3("Retrieving accounts names");
	vector<string> accounts;
	for(int i = 0 ; i < OTAPI_Wrap::GetAccountCount ();i++) {
		accounts.push_back(OTAPI_Wrap::GetAccountWallet_Name ( OTAPI_Wrap::GetAccountWallet_ID (i)));
	}
	return accounts;
}

bool cUseOT::assetCheckIfExists(const string & assetName) {
	vector<string> v = assetsGetNames();
	if (std::find(v.begin(), v.end(), assetName) != v.end())
		return true;
	return false;
}

const string cUseOT::assetGetId(const string & assetName) {
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

const vector<string> cUseOT::assetsGetNames() {
	if(!Init())
	return vector<string> {};

	vector<string> assets;
	for(int i = 0 ; i < OTAPI_Wrap::GetAssetTypeCount ();i++) {
		assets.push_back(OTAPI_Wrap::GetAssetType_Name ( OTAPI_Wrap::GetAssetType_ID (i)));
	}
	return assets;
}

const string cUseOT::assetIssue(const std::string & serverID, const std::string & nymID, const std::string & signedContract) { // Issue new asset type
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

const string cUseOT::assetNew(const std::string & nymID, const std::string & xmlContents){
	if(!Init())
			return "";
	return OTAPI_Wrap::CreateAssetContract(nymID, xmlContents);
}

const string cUseOT::assetGetContract(const std::string & assetID){
	if(!Init())
		return "";
	string strContract = OTAPI_Wrap::GetAssetType_Contract(assetID);
	return strContract;
}

const string cUseOT::assetGetDefault(){
	return mDefaultIDs.at("PurseID");
}

void cUseOT::assetRemove(const string & assetName) {
	if(!Init())
		return ;
	string nymID = assetGetId(assetName);
	if ( OTAPI_Wrap::Wallet_CanRemoveAssetType(assetName) ) {
		if ( OTAPI_Wrap::Wallet_RemoveAssetType(assetName) )
			_info("Asset was deleted successfully");
		else
			_warn("Asset cannot be removed");
	}
}

void cUseOT::assetSetDefault(const std::string & assetName){
	if(!Init())
		return ;
	mDefaultIDs.at("PurseID") = assetGetId(assetName);
}

const string cUseOT::contractSign(const std::string & nymID, const std::string & contract){ // FIXME can't sign contract with this (assetNew() functionality)
	if(!Init())
		return "";
	return OTAPI_Wrap::AddSignature(nymID, contract);
}

const vector<string> cUseOT::msgGetAll() { ///< Get all messages from all Nyms.
	if(!Init())
	return vector<string> {};

	for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
		msgGetForNym( nymGetName( OTAPI_Wrap::GetNym_ID(i) ) );
	}
	return vector<string> {};
}

const vector<string> cUseOT::msgGetForNym(const string & nymName) { ///< Get all messages from Nym.
	if(!Init())
		return vector<string> {};
	string nymID = nymGetId(nymName);
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

void cUseOT::msgSend(const string & nymSender, const string & nymRecipient, const string & msg) { ///< Send message from Nym1 to Nym2
	if(!Init())
		return;

	OT_ME madeEasy;
	string sender = nymGetId(nymSender);
	string recipient = nymGetId(nymRecipient);

	_dbg1(sender);
	_dbg1(recipient);

	string strResponse = madeEasy.send_user_msg ( mDefaultIDs.at("ServerID"), sender, recipient, msg);

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to send the message");
		return;
	}

	_info("Message was sent successfully.");
}

void cUseOT::msgSend(const string & nymRecipient, const string & msg) { ///< Send message from default Nym to Nym2
	if(!Init())
		return;

	OT_ME madeEasy;
	string recipient = nymGetId(nymRecipient);

	_dbg1(recipient);

	string strResponse = madeEasy.send_user_msg ( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID"), recipient, msg);

	// -1 error, 0 failure, 1 success.
	if (1 != madeEasy.VerifyMessageSuccess(strResponse))
	{
		_erro("Failed trying to send the message");
		return;
	}
	_info("Message was sent successfully.");
}


void cUseOT::msgInRemoveByIndex(const string & nymName, const int32_t & nIndex) {
	if(OTAPI_Wrap::Nym_RemoveMailByIndex (nymGetId(nymName), nIndex)){
		_info("Message removed successfully from inbox");
	}
}

void cUseOT::msgOutRemoveByIndex(const string & nymName, const int32_t & nIndex) {
	if( OTAPI_Wrap::Nym_RemoveOutmailByIndex(nymGetId(nymName), nIndex) ){
		_info("Message removed successfully from outbox");
	}
}

void cUseOT::nymCheck(const string & hisNymID) { // wip
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

bool cUseOT::nymCheckByName(const string & nymName) {
	vector<string> v = nymsGetMy();
	if (std::find(v.begin(), v.end(), nymName) != v.end())
		return true;
	return false;
}

void cUseOT::nymCreate(const string & nymName) {
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

const string cUseOT::nymGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("UserID");
}

const string cUseOT::nymGetId(const string & nymName) { // Gets nym aliases and IDs begins with '^'
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

const string cUseOT::nymGetInfo(const string & nymName) {
	if(!Init())
		return "";

	if (nymCheckByName(nymName)){
		return OTAPI_Wrap::GetNym_Stats( nymGetId(nymName) );
	}
	else {
		_erro("Nym not found");
	}

	return "";
}

const string cUseOT::nymGetName(const string & nymID) {
	if(!Init())
		return "";
	return OTAPI_Wrap::GetNym_Name(nymID);
}

void cUseOT::nymRefresh() {
	if(!Init())
		return;

	// Retrieve Nyms based on Moneychanger

	OT_ME madeEasy;

	int32_t nymCount = OTAPI_Wrap::GetNymCount();

	int32_t serverCount = OTAPI_Wrap::GetServerCount();

	for (int32_t serverIndex = 0; serverIndex < serverCount; ++serverIndex)
	{
		string serverId = OTAPI_Wrap::GetServer_ID(serverIndex);

		for (int32_t nymIndex = 0; nymIndex < nymCount; ++nymIndex)
		{
			string nymId = OTAPI_Wrap::GetNym_ID(nymIndex);

			bool bRetrievalAttempted = false;
			bool bRetrievalSucceeded = false;

			if (OTAPI_Wrap::IsNym_RegisteredAtServer(nymId, serverId))
			{

				bRetrievalAttempted = true;
				bRetrievalSucceeded = madeEasy.retrieve_nym(serverId, nymId, true);
			}
		}
	}

}

void cUseOT::nymRegister(const string & nymName) {
	if(!Init())
	return ;

	OT_ME madeEasy;

	_warn("Checking for default server only");

	string nymID = nymGetId(nymName);

	bool isReg = OTAPI_Wrap::IsNym_RegisteredAtServer(nymID, mDefaultIDs.at("ServerID"));

	if (!isReg)
	{
		string response = madeEasy.register_nym(mDefaultIDs.at("ServerID"), nymID);
		nOT::nUtils::DisplayStringEndl(cout, response);
		nOT::nUtils::DisplayStringEndl(cout, "Nym " + ToStr(nymName) + "(" + ToStr(nymID) + ")" + " was registered successfully");
	}
	else
		cout << "Nym " << nymName << "(" << nymID << ")" << " was already registered" << endl;
}

void cUseOT::nymRegister(const string & nymName, const string & serverName) {
	if(!Init())
	return ;
	//TODO: Make work with servers aliases
}

void cUseOT::nymRemove(const string & nymName) {
	if(!Init())
	return ;
	string nymID = nymGetId(nymName);
	if ( OTAPI_Wrap::Wallet_CanRemoveNym(nymID) ) {
		if ( OTAPI_Wrap::Wallet_RemoveNym(nymID) )
			_info("Nym was deleted successfully");
		else
			_warn("Nym cannot be removed");
	}
}

void cUseOT::nymSetDefault(const string & nymName) {
	if(!Init())
		return ;
	mDefaultIDs.at("UserID") = nymGetId(nymName);
}

const vector<string> cUseOT::nymsGetMy() {
	if(!Init())	return vector<string> {};

	if (!mNymsMy_loaded) {
		try {
		mNymsMy_loaded=0; // to mark that we start to delete data/data is inconsistent
		//mNymsMy.clear(); // FIXME Not used class cNyminfo (look: othint.cpp)
		mNymsMy_str.clear();

		for(int i = 0 ; i < OTAPI_Wrap::GetNymCount ();i++) {
			string nym_ID = OTAPI_Wrap::GetNym_ID (i);
			string nym_Name = OTAPI_Wrap::GetNym_Name (nym_ID);

			mNymsMy_str.push_back(nym_Name);
		}
	}
	catch(...) { }
	mNymsMy_loaded = true;
	}
return mNymsMy_str;
}

void cUseOT::serverAdd(const std::string & contract) {
	if(!Init())
			return ;
	OTAPI_Wrap::AddServerContract( contract );
}

void cUseOT::serverCheck() { // Use it to ping server
	if(!Init())
			return ;

	if( !OTAPI_Wrap::checkServerID( mDefaultIDs.at("ServerID"), mDefaultIDs.at("UserID") ) ){
		_erro("No response from server");
	}
	_info("Server OK");
}


const string cUseOT::serverGetDefault() {
	if(!Init())
		return "";
	return mDefaultIDs.at("ServerID");
}

const string cUseOT::serverGetId(const string & serverName) { // Gets nym aliases and IDs begins with '%'
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

const string cUseOT::serverGetName(const string & serverID){
	if(!Init())
		return "";
	return OTAPI_Wrap::GetServer_Name(serverID);
}

void cUseOT::serverRemove(const string & serverName) {
	if(!Init())
		return ;
	string nymID = serverGetId(serverName);
	if ( OTAPI_Wrap::Wallet_CanRemoveServer(serverName) ) {
		if ( OTAPI_Wrap::Wallet_RemoveServer(serverName) )
			_info("Server was deleted successfully");
		else
			_warn("Server cannot be removed");
	}
}


void cUseOT::serverSetDefault(const string & serverName) {
	if(!Init())
		return ;
	mDefaultIDs.at("ServerID") = serverGetId(serverName);
	_info("Default server: " + mDefaultIDs.at("ServerID"));
}

const vector<string> cUseOT::serversGet() { ///< Get all servers name
	if(!Init())
	return vector<string> {};

	vector<string> servers;
	for(int i = 0 ; i < OTAPI_Wrap::GetServerCount ();i++) {
		string servID = OTAPI_Wrap::GetServer_ID(i);
		servers.push_back(servID);
	}
	return servers;
}

const string cUseOT::textEncode(const string & plainText) {
	if(!Init())
		return "";

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string encodedText;
	encodedText = OTAPI_Wrap::Encode (plainText, bLineBreaks);
	return encodedText;
}

const string cUseOT::textEncrypt(const string & recipientNymName, const string & plainText) {
	if(!Init())
		return "";
	string encryptedText;
	encryptedText = OTAPI_Wrap::Encrypt(nymGetId(recipientNymName), plainText);
	return encryptedText;
}

const string cUseOT::textDecode(const string & encodedText) {
	if(!Init())
		return "";

	bool bLineBreaks = true; // FIXME? OTAPI_Wrap - bLineBreaks should usually be set to true
	string plainText;
	plainText = OTAPI_Wrap::Decode (encodedText, bLineBreaks);
	return plainText;
}

const string cUseOT::textDecrypt(const string & recipientNymName, const string & encryptedText) {
	if(!Init())
		return "";
	string plainText;
	plainText = OTAPI_Wrap::Decrypt(nymGetId(recipientNymName), encryptedText);
	return plainText;
}

cUseOT useOT;
} // nUse
}; // namespace OT


