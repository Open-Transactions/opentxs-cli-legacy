
#include "cmd.hpp"
#include "cmd_detail.hpp"

#include "lib_common2.hpp"
#include "ccolor.hpp"

namespace nOT {
namespace nNewcli {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_2; // <=== namespaces

/*
 * expected output - saved (previously) and expected results of unit tests
 * current output - what program generates NOW
 */

void HintingToTxtTest(string path, string command, vector <string> &completions, std::ifstream & file) {
	std:: ifstream Questions("script/test/hint/test1-questions.txt");	
	_mark("testing "<<command);
	bool test_ok=true;		//if he have all the same
	if(file.good()) {
		string line;
		getline(file,line);
		while(line.size()>0) {
		bool cmdexists=false; //if you command exists in file of expected output
		if(line==command) {
			cmdexists=true;
			string nextline;
			getline(file,nextline);
			vector <string>	proposals=SplitString(nextline); // current line is splited to expected outputs
			for(auto a: proposals) {// looking for a words which is not in completions but we have this in Hinting.txt
				std::vector<string>::iterator it;
				it = find (completions.begin(), completions.end(), a);
				if(it==completions.end()) {
					_erro("You don't have "<<a<<" in current output");
					test_ok=false;
				}
			}
			for(auto a: completions){	// looking for a words which is not in Hinting.txt but we have this in completions
				std::vector<string>::iterator it;
				it = find (proposals.begin(), proposals.end(), a);
				if(it==proposals.end()) {
					cmdexists=true;
				}
			}
		if(cmdexists==false) {
			_erro("You don't have "<<command<<" in file of expected output");
			test_ok=false;
		}
		}
		getline(file,line);
		}
	}	//end if
	if(test_ok==true) _mark("Test from "<<path<<" completed succesfully");
}
void ParseTest(string command,std::ifstream & file,const vector<string> & mVars , const vector<string> & mVarExts,const map<string,vector<string>> & mOptions) {
	bool test_ok=true;
	if(file.good()) {
		bool cmdexists=false; //if you command exists in file of expected output
		string line;
		getline(file,line);
		while(line.size()>0) {
			if(line==command) {
				string number_of_var = std::to_string(mVars.size());
				cmdexists=true;
				string nextline;
				getline(file,nextline);
				if (nextline== number_of_var) {
					_mark("good number of variables");
					getline(file,nextline);
					for(auto a: mVars){
					_note(nextline<<"="<<a);
					getline(file,nextline);
					}
				string number_of_var = std::to_string(mVarExts.size());
				if(nextline== number_of_var){
				_note("good number of Extvariables");
					getline(file,nextline);
					for(auto a: mVarExts){
					_mark(nextline<<"="<<a);
					getline(file,nextline);
					}
				}
				for(auto var: mOptions) {
					number_of_var = std::to_string(var.second.size());
					if(nextline==number_of_var ){
					getline(file,nextline);
					}
					for(auto a: var.second){
					_mark(nextline<<"="<<a);
					getline(file,nextline);
					}
				}
				}
				else _erro("Bad Parsing!");
					bool test_ok=false;
				}	
		getline(file,line);
		}
	}
	if(test_ok==true) _mark("Test ok");
}

	

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
	std::fstream file;
	file.open("Hintingtest.txt",  std::ios::out);
	std::fstream Questions;
	Questions.open("script/test/hint/test1-questions.txt",  std::ios::out);
	std::fstream Answers;
	Answers.open("script/test/hint/test1-answers.txt",  std::ios::out);
	auto alltest = vector<string>{ ""
//	,"~"
//	,"ot~"
//	,"ot msg send~ ali"
//	,"ot msg send ali~"
	,"ot a~"
	,"ot msg s~"
	,"ot hel~"
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
			nUtils:: hintingToTxt(file, cmd, completions);
			generateQuestions (Questions,cmd_raw);
			generateAnswers (Answers,cmd_raw, completions); 
		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
		// continue anyway
	}
}

void cCmdParser::_cmd_test_safe_completion(shared_ptr<cUseOT> use ) {
	_mark("TEST SAFE COMPLETION");
	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();
	string path="Hintingtest.txt";
	std::ifstream file2(path);
	auto alltest = vector<string>{ ""
//	,"~"
//	,"ot~"
//	,"ot msg send~ ali"
//	,"ot msg send ali~"
	,"ot a~"
	,"ot msg s~"
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
			HintingToTxtTest("Hintingtest.txt", cmd_raw, completions,file2);
			_note("Completions: " << DbgVector(completions));

		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
	}
}

void cCmdParser::_cmd_test_completion_answers(shared_ptr<cUseOT> use ) {
	_mark("TEST ANSWERS");

	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();
	std:: ifstream Answers("script/test/hint/test1-answers.txt");
	std:: ifstream Questions("script/test/hint/test1-questions.txt");
	vector<string> alltest;
	if (Questions.good()){
		string line;
		getline (Questions,line);
		while(line.size()>0)
		{
		alltest.push_back(line);
		char c=line.back();
		size_t i=line.size()-1;
		string subcommand=line.erase(i);
		while(i>0){
          if(c!=' ') {	
						subcommand=subcommand+"~";
						alltest.push_back(subcommand);
						i=i-1;
						c=subcommand.back();
						subcommand=subcommand.erase(i);
					}
					else if(c==' ') {
						i=i-1;
						c=subcommand.back();
						subcommand=subcommand.erase(i);
					}
			}
		getline(Questions,line);
		}
	}
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
			HintingToTxtTest("script/test/hint/test1-answers.txt", cmd_raw, completions,Answers);
			_note("Completions: " << DbgVector(completions));

		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
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
void cCmdParser::_parse_test(shared_ptr<cUseOT> use) {
	_mark(" PARSE TEST");
	shared_ptr<cCmdParser> parser(new cCmdParser);
	parser->Init();
	std:: ifstream Questions("script/test/parse/test1-questions.txt");
	std:: ifstream Answers("script/test/parse/test1-answers.txt");
	vector<string> alltest;
	if (Questions.good()){
		string line;
		getline (Questions,line);
		while(line.size()>0)
		{
			alltest.push_back(line);
			getline (Questions,line);
		}
	}
	
	for (const auto cmd_raw : alltest) {
		try {
			if (!cmd_raw.length()) continue;
			auto cmd = cmd_raw;
			auto pos = cmd_raw.find_first_of("~");
			bool good_format=true;
			if (pos!= string::npos) {
				good_format=false;
				_erro("Bad example -  Cmdname is not complete!");
				continue; // <---
			}
			auto processing = parser->StartProcessing(cmd, use);
			processing.Parse(good_format);
			shared_ptr<cCmdDataParse> TestData=processing.getmData();
			map<string, vector<string> > mOptions=TestData->getmOption();
			ParseTest(cmd_raw, Answers,TestData->getmVar(),TestData->getmVarExt(),TestData->getmOption());	
		}
		catch (const myexception &e) { e.Report(); }
		catch (const std::exception &e) { _erro("Exception " << e.what()); }
		// continue anyway
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


