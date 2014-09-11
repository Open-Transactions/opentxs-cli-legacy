#include <iostream>
#include <string>
#include <functional>

using namespace std;


string g(){return "[glob] ";}
string g_static(){return "[glob2] ";}

template<class c> void use(const c *t) {
	cerr << t->g();
}

template<class c> void use(const c *t) {
	cerr<<" other ";
}

struct S {
	string uid;
	S(string uid) : uid(uid) { }

	string g() const { return string("[uid=")+uid+string("] "); }
	static string g_static() { return "[static-method] "; }

	void f(){ cerr<<g(); }

	void test() {
		S s("intest");
		s.f();
		cerr<<g();

		function<void (void)> f = []() {
			cerr << ::g_static();
		};
		f();
	}
};

int main() {
	S s("inmain");
	s.test();
	cerr<<g();
	use(s);
}




