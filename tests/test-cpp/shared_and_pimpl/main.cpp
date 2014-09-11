#include <memory>
#include <iostream>
using namespace std;

#include "s.hpp"

int main () {
	auto ptr = make_shared<S>();
	cout << "Done" << endl;
  return 0;
}

