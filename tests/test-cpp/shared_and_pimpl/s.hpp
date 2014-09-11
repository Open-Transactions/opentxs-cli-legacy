#include <memory>
using namespace std;

struct P; // the pimpl

struct S : enable_shared_from_this<S> {
	unique_ptr<P> pimpl;
	S();
	~S();
};

