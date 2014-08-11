
#include "cmd.hpp"
#include "cmd_detail.hpp"

#include "lib_common2.hpp"
#include "ccolor.hpp"

//#if CFG_USE_EDITLINE
	#ifdef __unix__
		#include <editline/readline.h>
	#else // not unix
		// TODO: do support MinGWEditline for windows)
	#endif // not unix
//#endif // not use editline

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2 // <=== namespaces

using namespace nUse;
using namespace nText;

void cCmdParser::_AddFormat( const cCmdName &name, shared_ptr<cCmdFormat> format ) {
	if (!format->IsValid()) { _erro("Can not add invalid format, named " << (string)(name) ) ; return ; } // <--- RET

	mI->mTree.insert( cCmdParser_pimpl::tTreePair ( name , format ) );
	_info("Add format for command name (" << (string)name << "), now size=" << mI->mTree.size() << " new format is: ");
	// format->Debug();
}

void cCmdParser::AddFormat(
			const string &name,
			const vector<cParamInfo> &var,
			const vector<cParamInfo> &varExt,
			const map<string, cParamInfo> &opt,
			const cCmdExecutable::tFunc &exec)
{
	using namespace nOper;
	auto format = std::make_shared< cCmdFormat >( cCmdExecutable(exec), var, varExt, opt + mI->mCommonOpt );
	_AddFormat(name, format);
}


void cCmdParser::Init() {
	_mark("Init tree");

	auto this_shared_do_not_use = shared_from_this(); // make_shared<cCmdProcessing>(this);
	weak_ptr<cCmdParser> this_weak( this_shared_do_not_use );

// TODO hinting for ID?

	#define Tr(x,y) gTranslations->GetText(x,y);

	gTranslations->LoadLang("en", true); // Load default language
	gTranslations->LoadLang("pl"); // Load language

	cParamInfo pNym( "nym", [] () -> string { return Tr(eDictType::help, "nym") } ,
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Nym validation");
			return use.CheckIfExists(nUtils::eSubjectType::User, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Nym hinting");
			return use.NymGetAllNames();
		}
	);

	cParamInfo pNymMy( "nym-my", [] () -> string { return Tr(eDictType::help, "nym-my") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Nym validation");
				return use.CheckIfExists(nUtils::eSubjectType::User, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Nym hinting");
			return use.NymGetAllNames();
		}
	);

	cParamInfo pNymTo = pNym << cParamInfo("nym-to",[] () -> string { return Tr(eDictType::help, "nym-to") }); // TODO suggest not the same nym as was used already before
	cParamInfo pNymFrom = pNymMy << cParamInfo("nym-from", [] () -> string { return Tr(eDictType::help, "nym-from") });

	cParamInfo pNymNewName( "nym-new-name", [] () -> string { return Tr(eDictType::help, "nym-new-name") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Nym name validation");
				return true; // Takes all input TODO check if Nym with tis name exists
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Nym name hinting");
			return vector<string> {}; // No hinting for new Nym name
		}
	);
	// ot send cash

	cParamInfo pAccount( "account", [] () -> string { return Tr(eDictType::help, "account") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Account validation");
				return use.CheckIfExists(nUtils::eSubjectType::Account, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Account hinting");
			return use.AccountGetAllNames();
		}
	);

	cParamInfo pAccountMy( "account-my", [] () -> string { return Tr(eDictType::help, "account-my") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Account validation");
				return use.CheckIfExists(nUtils::eSubjectType::Account, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Account hinting");
			return use.AccountGetAllNames();
		}
	);
	cParamInfo pAccountTo = pAccount << cParamInfo("account-to", [] () -> string { return Tr(eDictType::help, "account-to") }); // TODO suggest not the same account as was used already before
	cParamInfo pAccountFrom = pAccountMy << cParamInfo("account-from", [] () -> string { return Tr(eDictType::help, "account-from") });

	cParamInfo pAccountNewName( "account-new", [] () -> string { return Tr(eDictType::help, "account-new") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Account name validation");
				return true; // Takes all input TODO check if Account with this name exists
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Account name hinting");
			return vector<string> {}; // No hinting for new Nym name
		}
	);


	cParamInfo pAsset( "asset", [] () -> string { return Tr(eDictType::help, "asset") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Asset validation");
				return use.CheckIfExists(nUtils::eSubjectType::Asset, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Asset hinting");
			return use.AssetGetAllNames();
		}
	);

	cParamInfo pServer( "server", [] () -> string { return Tr(eDictType::help, "server") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			_dbg3("Server validation");
				return use.CheckIfExists(nUtils::eSubjectType::Server, data.Var(curr_word_ix + 1));
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			_dbg3("Server hinting");
			return use.ServerGetAllNames();
		}
	);

	cParamInfo pOnceInt( "int", [] () -> string { return Tr(eDictType::help, "int") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			// TODO check if is any integer
			// TODO check if not present in data
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> { "-1", "0", "1", "2", "100" };
		}
	);

	cParamInfo pAmount( "amount", [] () -> string { return Tr(eDictType::help, "amount") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			// TODO check if is any integer
			// TODO check if can send that amount
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {"1", "10", "100" };
		}
	);

	cParamInfo pSubject( "subject", [] () -> string { return Tr(eDictType::help, "subject") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> { "hello","hi","test","subject" };
		}
	);

	cParamInfo pBool( "yes-no", [] () -> string { return Tr(eDictType::help, "yes-no") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true; // option's value should be null
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {}; // this should be empty option, let's continue
		}
		, 0
	);

	cParamInfo pBoolBoring( "yes-no", [] () -> string { return Tr(eDictType::help, "yes-no") },
		pBool.funcValid , pBool.funcHint
		, cParamInfo::eFlags::isBoring
	);

	cParamInfo pText( "text", [] () -> string { return Tr(eDictType::help, "text") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {}; // this should be empty option, let's continue
		}
	);

	cParamInfo pCmdName1("cmdword1", [] () -> string { return Tr(eDictType::help, "cmdword1") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true;
		} ,
		[this_weak] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			shared_ptr<cCmdParser> this_lock( this_weak );
			return this_lock->GetCmdNamesWord1();
		}
	);

	cParamInfo pCmdName2("cmdword2", [] () -> string { return Tr(eDictType::help, "cmdword2") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			return true;
		} ,
		[this_weak] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			shared_ptr<cCmdParser> this_lock( this_weak );
			try {
				return this_lock->GetCmdNamesWord2( data.V(1) );
			} catch(const cErrParseName &e) { return vector<string>{}; }
		}
	);

	// sendmoney alice gold 1000
	// sendmsgto alice hi    --addmoney 1000 --addmoney 2000
	//           arg=1 arg=2           arg=3           arg=4
	// TODO

	cParamInfo pMsgInIndex( "msg-index-inbox", [] () -> string { return Tr(eDictType::help, "msg-index-inbox") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			if ( ( data.Var(nr) == "-1" && use.MsgInCheckIndex(data.Var(nr-1), 0) ) || use.MsgInCheckIndex(data.Var(nr-1), std::stoi( data.Var(nr)) ) ) {//TODO check if integer
				return true;
			}
			return false;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {}; //TODO hinting function for msg index
		}
	);

	cParamInfo pMsgOutIndex( "msg-index-outbox", [] () -> string { return Tr(eDictType::help, "msg-index-outbox") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			cout << "nym:"  << data.Var(nr-1) << endl;
			if ( ( data.Var(nr) == "-1" && use.MsgOutCheckIndex(data.Var(nr-1), 0) ) || use.MsgOutCheckIndex(data.Var(nr-1), std::stoi( data.Var(nr)) ) ) {//TODO check if integer
				return true;
			}
			return false;
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {

			return vector<string> {}; //TODO hinting function for msg index
		}
	);

	cParamInfo pInboxIndex( "inbox-index", [] () -> string { return Tr(eDictType::help, "inbox-index") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			return true; //TODO
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {}; //TODO hinting function for msg index
		}
	);

	cParamInfo pPaymetInboxIndex( "payment-inbox-index", [] () -> string { return Tr(eDictType::help, "payment-inbox-index") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			return true; //TODO
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return vector<string> {}; //TODO
		}
	);

	cParamInfo pReadFile( "file", [] () -> string { return Tr(eDictType::help, "file") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			return true; //TODO
		} ,
		[this] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			mEnableFilenameCompletion = true; // Enable filename autocompletion
			return vector<string> {};
		}
	);

	cParamInfo pLang( "lang", [] () -> string { return Tr(eDictType::help, "lang") },
		[] (cUseOT & use, cCmdData & data, size_t curr_word_ix ) -> bool {
			const int nr = curr_word_ix+1;
			return true; //TODO
		} ,
		[] ( cUseOT & use, cCmdData & data, size_t curr_word_ix  ) -> vector<string> {
			return gTranslations->GetLanguages(true);
		}
	);

	// ===========================================================================
	// COMMON OPTIONS

	mI->mCommonOpt.clear();
	auto option_dryrun = std::make_pair( string("--dryrun"), pBoolBoring );
	mI->mCommonOpt.insert( option_dryrun );

	// ===========================================================================

	using namespace nOper; // vector + is available inside lambdas
	using std::stoi;

	// types used in lambda header:
	typedef shared_ptr<cCmdData> tData;
	typedef nUse::cUseOT & tUse;
	typedef cCmdExecutable::tExitCode tExit;
	#define LAMBDA [] (tData d, tUse U) -> tExit

	auto & pFrom = pNymFrom;
	auto & pTo = pNymTo;
	auto & pSubj = pSubject;
	auto & pMsg = pSubject; // TODO
	auto & pInt = pOnceInt;


	//======== special, test, developer: ========

	AddFormat("help", {}, {}, {},
		[this_weak] (tData d, tUse U) -> tExit { auto &D=*d;
			shared_ptr<cCmdParser> this_lock( this_weak );
			this_lock->PrintUsage();
			return true;
		} );

	AddFormat("help cmd", { pCmdName1 } , { pCmdName2 }, {},
		[this_weak] (tData d, tUse U) -> tExit { auto &D=*d;
			_mark("command help!");
			shared_ptr<cCmdParser> this_lock( this_weak );
			string cmd = D.V(1);
			if (D.v(2).size()>0) cmd += " " + D.V(2);
			this_lock->PrintUsageCommand( cmd );
			return true;
		} );

	AddFormat("lang set", { pLang }, {}, {},
		[] (tData d, tUse U) -> tExit { auto &D=*d;
			gTranslations->LoadLang(D.V(1));
			return true;
		} );

	AddFormat("x", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test(Utmp); return true; } );

	AddFormat("test", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test(Utmp); return true; } );

	AddFormat("test tab", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test_completion(Utmp); return true; } );

	AddFormat("test completion", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test_safe_completion(Utmp); return true; } );

	AddFormat("test tree", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test_tree(Utmp); return true; } );

	AddFormat("history", {}, {}, { },
		LAMBDA { auto &D=*d; return U.DisplayHistory(D.has("--dryrun") ); } );

	AddFormat("defaults", {}, {}, { },
		LAMBDA { auto &D=*d; return U.DisplayAllDefaults(D.has("--dryrun") ); } );

	AddFormat("refresh", {}, {} ,{ },
		LAMBDA { auto &D=*d; return	U.Refresh( D.has("--dryrun") ); } ) ;

	AddFormat("test complete", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); cmd_test_EndingCmdNames(Utmp); return true; } );

	AddFormat("test answers", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _cmd_test_completion_answers(Utmp); return true; } );

	AddFormat("test parse", {}, {}, {},
		LAMBDA { auto &D=*d; auto Utmp = make_shared<cUseOT>( U ); _parse_test(Utmp); return true; } );

	//======== ot account ========

	AddFormat("account", {}, {}, {},
		LAMBDA { auto &D=*d; return U.DisplayDefaultSubject(nUtils::eSubjectType::Account, D.has("--dryrun") ); } );

	AddFormat("account new", {pNymMy, pAsset, pAccountNewName}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountCreate( D.V(1), D.V(2), D.V(3), D.has("--dryrun") ); } );

	AddFormat("account refresh", {}, {pAccount}, { {"--all", pBool } },
		LAMBDA { auto &D=*d; return U.AccountRefresh( D.v(1, U.AccountGetName(U.AccountGetDefault())), D.has("--all"), D.has("--dryrun") ); } );

	AddFormat("account set-default", {pAccount}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountSetDefault( D.V(1), D.has("--dryrun") ); } );

	AddFormat("account rm", {pAccount}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountRemove( D.V(1), D.has("--dryrun") ); } );

	AddFormat("account ls", {}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountDisplayAll( D.has("--dryrun") ); } );

	AddFormat("account show", {pAccount}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountDisplay( D.V(1), D.has("--dryrun") ); } );

	AddFormat("account rename", {pAccount, pAccountNewName}, {}, {},
		LAMBDA { auto &D=*d; return U.AccountRename(D.V(1), D.V(2), D.has("--dryrun") ); } );

	AddFormat("account transfer-from", {pAccountFrom, pAccountTo, pAmount}, {pText}, {},
		LAMBDA { auto &D=*d; return U.AccountTransfer(D.V(1), D.V(2), stoi( D.V(3) ), D.v(4), D.has("--dryrun") ); } );

	AddFormat("account transfer-to", {pAccountTo, pAmount}, {pText}, {},
		LAMBDA { auto &D=*d; return U.AccountTransfer(U.AccountGetName(U.AccountGetDefault()), D.V(1), stoi( D.V(2) ), D.v(3), D.has("--dryrun") ); } );

	//======== ot account-in ========

	AddFormat("account-in ls", {}, {pAccountMy}, {},
		LAMBDA { auto &D=*d; return U.AccountInDisplay(D.v(1, U.AccountGetName(U.AccountGetDefault())), D.has("--dryrun") ); } );

	AddFormat("account-in accept", {}, {pAccountMy, pInboxIndex}, { {"--all", pBool } },
		LAMBDA { auto &D=*d; return U.AccountInAccept(D.v(1, U.AccountGetName(U.AccountGetDefault())), stoi( D.v(2, "0") ), D.has("--all"), D.has("--dryrun") ); } ); //TODO index

	//======== ot account-out ========

	AddFormat("account-out cancel", {}, {pAccountMy, pInboxIndex}, { {"--all", pBool } },
		LAMBDA { auto &D=*d; return U.AccountOutCancel(D.v(1, U.AccountGetName(U.AccountGetDefault())), stoi( D.v(2, "0") ), D.has("--all"), D.has("--dryrun") ); } ); //FIXME

	AddFormat("account-out ls", {}, {pAccountMy}, {},
		LAMBDA { auto &D=*d; return U.AccountOutDisplay(D.v(1, U.AccountGetName(U.AccountGetDefault())), D.has("--dryrun") ); } );

	//======== ot asset ========

	AddFormat("asset", {}, {}, {},
		LAMBDA { auto &D=*d; return U.DisplayDefaultSubject(nUtils::eSubjectType::Asset, D.has("--dryrun") ); } );


	AddFormat("asset ls", {}, {}, {},
		LAMBDA { auto &D=*d; return U.AssetDisplayAll( D.has("--dryrun") ); } );

	AddFormat("asset issue", {}, {pServer, pNym}, {},
		LAMBDA { auto &D=*d; return U.AssetIssue(	D.v(1, U.ServerGetName( U.ServerGetDefault())),
																							D.v(2, U.NymGetName( U.NymGetDefault())),
																							D.has("--dryrun") ); } );

	AddFormat("asset new", {pNym}, {}, {},
		LAMBDA { auto &D=*d; return U.AssetNew(D.V(1), D.has("--dryrun") ); } );

	AddFormat("asset rm", {pNym}, {}, {},
		LAMBDA { auto &D=*d; return U.AssetRemove(D.V(1), D.has("--dryrun") ); } );

	AddFormat("asset set-default", {pAsset}, {}, {},
		LAMBDA { auto &D=*d; return U.AssetSetDefault( D.V(1), D.has("--dryrun") ); } );

	//======== ot cash ========

	AddFormat("cash export-from", {pNymFrom, pNymTo, pAccountMy}, {}, { {"--pass",pBool} },
		LAMBDA { auto &D=*d; return U.CashExportWrap( D.V(1), D.V(2), D.V(3), D.has("--pass"), D.has("--dryrun") ); } );

	AddFormat("cash export-to", {pNymTo, pAccountMy}, {}, { {"--pass",pBool} },
		LAMBDA { auto &D=*d; return U.CashExportWrap( U.NymGetName(U.NymGetDefault()), D.V(1), D.V(2), D.has("--pass"), D.has("--dryrun") ); } );

	AddFormat("cash deposit", {pAccountMy}, {}, {},
		LAMBDA { auto &D=*d; return U.CashDepositWrap( D.V(1), D.has("--dryrun") ); } );

	AddFormat("cash import", {pNymMy}, {}, {},
		LAMBDA { auto &D=*d; return U.CashImport( D.V(1), D.has("--dryrun") ); } );

	AddFormat("cash send-to", {pTo, pAccountMy, pAmount}, {}, {},
		LAMBDA { auto &D=*d; return U.CashSend( U.NymGetName(U.NymGetDefault()), D.V(1), D.V(2), stoi(D.V(3)), D.has("--dryrun") ); } );

	AddFormat("cash send-from", {pFrom, pTo, pAccountMy, pAmount}, {}, {},
		LAMBDA { auto &D=*d; return U.CashSend( D.V(1), D.V(2), D.V(3), stoi(D.V(4)), D.has("--dryrun") ); } );

	AddFormat("cash ls", {}, {pAccountMy}, {},
		LAMBDA { auto &D=*d; return U.CashShow( D.v(1, U.AccountGetName(U.AccountGetDefault())), D.has("--dryrun") ); } );

	AddFormat("cash withdraw", {pAmount}, {}, {},
		LAMBDA { auto &D=*d; return U.CashWithdraw(U.AccountGetName(U.AccountGetDefault()), stoi(D.V(1)), D.has("--dryrun") ); } );

	AddFormat("cash withdraw-from", {pAccountMy, pAmount}, {}, {},
		LAMBDA { auto &D=*d; return U.CashWithdraw( D.V(1), stoi(D.V(2)), D.has("--dryrun") ); } );

	//======== ot msg-in and msg-out ========

	AddFormat( "msg-in show", {}, {pNym, pMsgInIndex}, {},
		LAMBDA { auto &D=*d; return U.MsgDisplayForNymInbox( D.v(1, U.NymGetName(U.NymGetDefault())) , stoi(D.v(2,"0")) ,  D.has("--dryrun") ); } );
	AddFormat("msg-out show", {}, {pNym, pMsgOutIndex}, {},
		LAMBDA { auto &D=*d; return U.MsgDisplayForNymOutbox( D.v(1, U.NymGetName(U.NymGetDefault())) , stoi(D.v(2,"0")) ,  D.has("--dryrun") ); } );

	//======== ot msg ========

	AddFormat("msg ls", {}, {pNym}, {},
		LAMBDA { auto &D=*d; return U.MsgDisplayForNym( D.v(1, U.NymGetName(U.NymGetDefault())), D.has("--dryrun") ); } );

	AddFormat("msg send-from", {pFrom, pTo}, {pSubj, pMsg}, { {"--cc",pNym} , {"--bcc",pNym} , {"--prio",pInt}, {"--file",pReadFile} },
		LAMBDA { auto &D=*d; return U.MsgSend(D.V(1), D.V(2) + D.o("--cc") , D.v(3,"nosubject"), D.v(4), stoi(D.o1("--prio","0")), D.o1("--file",""), D.has("--dryrun")); }	);

	AddFormat("msg send-to", {pTo}, {pSubj, pMsg}, { {"--cc",pNym} , {"--bcc",pNym} , {"--prio",pInt}, {"--file",pReadFile} },
		LAMBDA { auto &D=*d; return U.MsgSend(U.NymGetName(U.NymGetDefault()), D.V(1) + D.o("--cc"), D.v(2,"nosubject"), D.v(3), stoi(D.o1("--prio","0")), D.o1("--file",""), D.has("--dryrun")); }	);

	AddFormat("msg rm", {pNym, pOnceInt}, {}, {/*{"--all", pBool}*/ }, // FIXME proper handle option without parameter!
		LAMBDA { auto &D=*d; return U.MsgInRemoveByIndex(D.V(1), stoi(D.V(2)), D.has("--dryrun"));} );

	AddFormat("msg rm-out", {pNym, pOnceInt}, {}, {/*{"--all", , pBool}*/ }, // FIXME proper handle option without parameter!
		LAMBDA { auto &D=*d; return U.MsgOutRemoveByIndex(D.V(1), stoi(D.V(2)), D.has("--dryrun")); } );

	//======== ot nym ========

	AddFormat("nym", {}, {}, {},
		LAMBDA { auto &D=*d; return U.DisplayDefaultSubject(nUtils::eSubjectType::User, D.has("--dryrun") ); } );

	AddFormat("nym check", {pNym}, {}, {},
		LAMBDA { auto &D=*d; return U.NymCheck( D.V(1), D.has("--dryrun") ); } );

	AddFormat("nym export", {pNym}, {}, {},
		LAMBDA { auto &D=*d; return U.NymExport( D.V(1), D.has("--dryrun") ); } );

	AddFormat("nym import", {}, {}, {{"--file",pReadFile}},
		LAMBDA { auto &D=*d; return U.NymImport( D.o1("--file",""), D.has("--dryrun") ); } );

	AddFormat("nym info", {pNym}, {}, {},
		LAMBDA { auto &D=*d; return U.NymDisplayInfo( D.V(1), D.has("--dryrun") ); } );

	AddFormat("nym register", {pNym}, {pServer}, {} ,
		LAMBDA { auto &D=*d; return U.NymRegister( D.V(1), D.v(2, U.ServerGetName(U.ServerGetDefault())), D.has("--dryrun") ); } );

	AddFormat("nym rm", {pNym}, {}, { },
		LAMBDA { auto &D=*d; return U.NymRemove( D.V(1), D.has("--dryrun") ); } );

	AddFormat("nym new", {pNymNewName}, {}, { {"--register", pBool} },
		LAMBDA { auto &D=*d; return U.NymCreate( D.V(1), D.has("--register"), D.has("--dryrun") ); } );

	AddFormat("nym set-default", {pNym}, {}, { },
		LAMBDA { auto &D=*d; return U.NymSetDefault( D.V(1), D.has("--dryrun") ); } );

	AddFormat("nym refresh", {}, {pNym}, { {"--all", pBool} },
		LAMBDA { auto &D=*d; return U.NymRefresh( D.v(1, U.NymGetName( U.NymGetDefault() ) ), D.has("--all"), D.has("--dryrun") ); } );

	AddFormat("nym ls", {}, {}, {},
		LAMBDA { auto &D=*d; return U.NymDisplayAll( D.has("--dryrun") ); } );

	AddFormat("nym rename", {pNymMy, pNymNewName}, {}, {},
		LAMBDA { auto &D=*d; return U.NymRename(D.V(1), D.V(2), D.has("--dryrun") ); } );

	//======== ot payment ========

	AddFormat("payment ls", {}, {pNymMy, pServer}, {},
		LAMBDA { auto &D=*d; return U.PaymentShow( D.v(1, U.NymGetName( U.NymGetDefault())), D.v(2, U.ServerGetName(U.ServerGetDefault())), D.has("--dryrun") ); } );

	AddFormat("payment accept", {pAccountMy, pPaymetInboxIndex}, {}, {},
		LAMBDA { auto &D=*d; return U.PaymentAccept( D.V(1), stoi(D.V(2)), D.has("--dryrun") ); } );
		
	AddFormat("payment discard", {}, {pNymMy, pPaymetInboxIndex}, {},
		LAMBDA { auto &D=*d; return U.PaymentDiscard( D.v(1), D.v(2), D.has("--all"), D.has("--dryrun") ); } );
			
	AddFormat("payment discard-all", {}, {}, {},
		LAMBDA { auto &D=*d; return U.PaymentDiscardAll( D.has("--dryrun") ); } );
		
	//======== ot purse ========

	AddFormat("purse create", {pServer, pAsset, pNym, pNym}, {}, { },
		LAMBDA { auto &D=*d; return U.PurseCreate( D.V(1), D.V(2), D.V(3), D.V(4), D.has("--dryrun") ); } );

	AddFormat("purse show", {}, {pServer, pAsset, pNym}, { },
		LAMBDA { auto &D=*d; return U.PurseDisplay( D.v(1, U.ServerGetName(U.ServerGetDefault())), D.v(2, U.AssetGetName(U.AssetGetDefault())), D.v(3, U.NymGetName(U.NymGetDefault())), D.has("--dryrun") ); } );

	//======== ot server ========

	AddFormat("server", {}, {}, {},
		LAMBDA { auto &D=*d; return U.DisplayDefaultSubject(nUtils::eSubjectType::Server, D.has("--dryrun") ); } );

	AddFormat("server ls", {}, {}, {},
		LAMBDA { auto &D=*d; return U.ServerDisplayAll(D.has("--dryrun") ); } );

	AddFormat("server add", {}, {}, {},
		LAMBDA { auto &D=*d; return U.ServerAdd(D.has("--dryrun") ); } );

	AddFormat("server new", {}, {pNymMy}, {},
		LAMBDA { auto &D=*d; return U.ServerCreate(D.v(1, U.NymGetName( U.NymGetDefault())), D.has("--dryrun") ); } );

	AddFormat("server rm", {pServer}, {}, {},
		LAMBDA { auto &D=*d; return U.ServerRemove(D.V(1), D.has("--dryrun") ); } );

	AddFormat("server set-default", {pServer}, {}, {},
		LAMBDA { auto &D=*d; return U.ServerSetDefault( D.V(1), D.has("--dryrun") ); } );

	AddFormat("server show-contract", {pServer}, {}, {},
		LAMBDA { auto &D=*d; return U.ServerShowContract(D.V(1), D.has("--dryrun") ); } );

	//======== ot text ========

	AddFormat("text encode", {}, {pText}, {},
		LAMBDA { auto &D=*d; return U.TextEncode(D.v(1, ""), D.has("--dryrun") ); } );

	AddFormat("text decode", {}, {pText}, {},
		LAMBDA { auto &D=*d; return U.TextDecode(D.v(1, ""), D.has("--dryrun") ); } );

	AddFormat("text encrypt", {pNymTo}, {pText}, {},
		LAMBDA { auto &D=*d; return U.TextEncrypt(D.V(1), D.v(2, ""), D.has("--dryrun") ); } );

	AddFormat("text decrypt", {pNymMy}, {pText}, {},
		LAMBDA { auto &D=*d; return U.TextDecrypt(D.V(1), D.v(2, ""), D.has("--dryrun") ); } );

	mI->BuildCache_CmdNames();
}




} // namespace
} // namespace

