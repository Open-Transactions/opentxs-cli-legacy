/* See other files here for the LICENCE that applies here. */
/*
This file demonstrates the coding style that is to be used in this project.
*/

#include "lib_common1.hpp"

// Please read and follow this syntax examples:
namespace nExamplesOfConvention {
// Welcome, to the world of C++11 !

int gGlobalVariable; // g - global variable

class cFooBar {
private:
	static int sStaticVariable; // s - static variable

	int mFooBarBaz; // m - variable, member of class

	int Fooberize();
	int DerpTheHerp();

public:
	cFooBar(int x)
	: mFooBarBaz(x)
	{}

	inline int Foo() {
		int abc=42;
		return abc;
	}
};
int cFooBar::sStaticVariable = 0;

// In case of simple parameters, we can skip "m" prefix. In this case:
struct cSimpleParam {
	int x;
	int y;

	cSimpleParam(int x, int y)
	: x(x), y(y)
	{}
};


} // namespace
