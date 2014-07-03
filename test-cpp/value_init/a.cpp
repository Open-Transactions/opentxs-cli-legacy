#include <iostream>
#include <string>

using namespace std;

// http://stackoverflow.com/questions/2183087/why-cant-i-use-float-value-as-a-template-parameter
// TODO ?  OTHER TYPES

template <class T, T INIT>
class value_init {
	private:
		T data;

	public:
		value_init();
		operator T const &() const { return data; }
		operator T&() { return data;	}
};


template <class T, T INIT>
value_init<T, INIT>::value_init() {
	data=INIT;
}


class cTest {
	public:
		value_init<bool , true> a,b,c;
};

/*
class cTestFloat {
	public:
		value_init<double , 3.14> pi1,pi2,pi3;
//		cTest : pi1(3.14), pi2(3.14), pi3(3.14) { }
}; */

int main() {
	cTest test;

	cout << test.a << test.b << test.c << endl;

	//value_init< double , 3.14 > pi1, pi2, pi3;

//	cout << pi1;


}


