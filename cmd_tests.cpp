
#include "cmd.hpp"
#include "cmd_detail.hpp"

#include "lib_common2.hpp"
#include "ccolor.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

using namespace nUse;
vector<string> cCmdParser::EndingCmdNames (const string sofar) {
	vector<string> CmdNames;
	for(auto var : mI->mTree) {
		bool Begin=nUtils::CheckIfBegins(sofar,std::string(var.first));
		if(Begin==true) {	// if our word begins some kind of command
			std:: string propose=var.first;
			size_t pos = sofar.find(" ");
			if(pos==std::string::npos ) {	// if we haven't " " in our word

				string str=" ";
				size_t found=propose.find_first_of(str);	// looking for first " " in command
				propose.resize(found);
				bool ifexists=false;
				for(auto prop: CmdNames) {		// we must chcek that it not exists in vecor of proposals
					if(prop==propose) {
						ifexists=true;
						}
					}
				if(ifexists==false) {	// if it not exists we can push back
					CmdNames.push_back(propose);
					}
				}
			else if(pos!=std::string::npos) {	// if we have " " in our word
				size_t pos2 = propose.find(" ");
				std::string formated_propose = propose.substr (pos2);
				CmdNames.push_back(formated_propose);
			}
		}	//end if
	}	//end for
	for (auto str: CmdNames) {
	_dbg1(str+" ");
	}
	return CmdNames;
}
void cCmdParser::_cmd_test(  shared_ptr<cUseOT> use  ) {
	_cmd_test_completion( use );
//	_cmd_test_tree( use );
}

void cCmdParser::_cmd_test_completion( shared_ptr<cUseOT> use ) {
	_mark("TEST COMPLETION");
	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();

	auto alltest = vector<string>{ ""
//	,"~"
//	,"ot~"
//	,"ot msg send~ ali"
//	,"ot msg send ali~"
	,"ot a~"
	,"ot msg s~"
	,"ot help help~"
//  ,"msg send-from al~"
//  ,"ot msg sen~ alice"
//	,"ot msg sen~ alice bob"
//	,"ot msg send-from ali~ bo"
//	,"ot msg send-from ali bo~"
//	,"ot msg send-from alice bob subject message --prio 3 --dryr~"
//	,"ot msg send-from alice bob subject message --pr~ 3 --dryrun"
//	,"ot msg send-from alice bob subject message --prio 3 --cc al~ --dryrun"
//	,"ot help securi~"
//	,"help securi~"
//	,"ot msg sendfrom ali bobxxxxx~"
//	,"ot msg sendfrom ali       bob      subject message_hello --cc charlie --cc dave --prio 4 --cc eve --dry~ --cc xray"
	};
	for (const auto cmd_raw : alltest) {
		try {
			if (!cmd_raw.length()) continue;

			auto pos = cmd_raw.find_first_of("~");
			if (pos == string::npos) {
				_erro("Bad example - no TAB position given!");
				continue; // <---
			}
			auto cmd = cmd_raw;
			cmd.erase( pos , 1 );

			_mark("====== Testing completion: [" << cmd << "] for position pos=" << pos << " (from cmd_raw="<<cmd_raw<<")" );
			auto processing = parser->StartProcessing(cmd, use);
			vector<string> completions = processing.UseComplete( pos  );
			_note("Completions: " << DbgVector(completions));

		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
		// continue anyway
	}
}


void cCmdParser::_cmd_test_tree( shared_ptr<cUseOT> use ) {
	_mark("TEST TREE");
	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();

	auto alltest = vector<string>{ ""
	,"ot xxxx yyyy"
	,"ot xxxx"
	,"ot xxxx aaa bbb ccc ddd --eee fff --dryrun"
	,"ot nym"
	,"ot nym --dryrun"
	,"ot nym ls"
	,"ot nym ls --dryrun"
	,"ot msg"
	,"ot msg xxxx"
	,"ot msg send-from"
	,"ot msg send-from alice"
	,"ot msg send-from alice bob test test"
	};
	bool panic_on_throw=false;
	for (auto cmd : alltest) {
		try {
			if (!cmd.length()) continue;
			_mark("====== Testing command: " << cmd );
			auto processing = parser->StartProcessing(cmd, use);
			processing.UseExecute();
		}
		catch (const myexception &e) { e.Report();
			if (panic_on_throw) throw ;
		}
		catch (const std::exception &e) { _erro("Exception " << e.what());
			if (panic_on_throw) throw ;
		}
	}

}

void cCmdParser::cmd_test( shared_ptr<cUseOT> use ) {
	try {
		_cmd_test(use);
	} catch (const myexception &e) { e.Report(); throw ; } catch (const std::exception &e) { _erro("Exception " << e.what()); throw ; }
}


void cCmdParser::cmd_test_EndingCmdNames(shared_ptr<cUseOT> use) {
	_mark("TEST ENDING_CMD_NAMES");


	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();
	auto alltest = vector<string> {
	 "msg s~"
	,"m~"
	,"~"
	};
	for (const auto cmd_raw : alltest) {

		try {
			if (!cmd_raw.length()) continue;
			auto pos = cmd_raw.find_first_of("~");
			if (pos == string::npos) {
				_erro("Bad example - no TAB position given!");
				continue; // <---
			}
			auto cmd = cmd_raw;
			cmd.erase( pos , 1 );

			_mark("====== Testing completion: [" << cmd << "] for position pos=" << pos << " (from cmd_raw="<<cmd_raw<<")" );
			auto processing = parser->StartProcessing(cmd, use);
			vector<string> completions = parser->EndingCmdNames( cmd  );
			_note("Completions: " << DbgVector(completions));

		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
		// continue anyway
	}
}


} // namespace
} // namespace


