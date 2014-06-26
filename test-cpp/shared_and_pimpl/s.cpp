
#include "s.hpp"

struct P { int a,b,c,d; }; // <-- pimpl is implemented here

S::S() : pimpl(new P) { }
S::~S() { }

