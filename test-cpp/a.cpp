#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <functional>

using namespace std;

class cLine {
	public:

		vector<string> words;

		cLine(vector<string> _words) : words(_words) { } 

		string cmd_string() const { 
			return words[0] + " " + words[1];
		}
};

class cCommand {
	public:
		typedef function < void (void) > tExec;
		typedef function < void (const cLine&) > tHint;

		tExec exec;
		tHint hint;

		cCommand() = default;
		cCommand(tHint _hint, tExec _exec) : hint(_hint), exec(_exec) { };

		void Demo(const cLine &line) {
			if (hint) hint(line);
			if (exec) exec();
		}
};

void hint_from(const cLine& line) { 
	cout << "FUNCTION: VALIDATING from, on current line:";
	for(auto x:line.words) cout<<x<<" ";
	cout <<"." << endl;
}
void exec_sendfrom() { 
	cout << "FUNCTION: sending from..." << endl;
}

class cParser {
	protected:
		map<string,cCommand> tree;


	public:

	void BuildTree() {
		tree["msg sendfrom"] = cCommand(hint_from, exec_sendfrom);	
		tree["msg help"] = cCommand(hint_from, []{ cout<<"This is the HELP for msg: ........... ." << endl; } );	
	}

	void Run(const cLine &line) {
		tree[ line.cmd_string() ].Demo(line);
	}
};

int main() {	


	auto x = [] { cout<<"Lambda!"<<endl; };
	x();
	x();

	cParser parser;
	parser.BuildTree();
	parser.Run( cLine( vector<string>{"msg","help"} ) );
	parser.Run( cLine( vector<string>{"msg","sendfrom"} ) );
	parser.Run( cLine( vector<string>{"msg","sendfrom","ra"} ) );
	parser.Run( cLine( vector<string>{"msg","sendfrom","rafa"} ) );
}

