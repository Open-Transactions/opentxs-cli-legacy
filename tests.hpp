/* See other files here for the LICENCE that applies here. */
/*
Template for new files, replace word "template" and later delete this line here.
*/

#ifndef INCLUDE_OT_NEWCLI_tests
#define INCLUDE_OT_NEWCLI_tests

#include "lib_common1.hpp"

#include "othint.hpp"

namespace nOT {
namespace nTests {

INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 // <=== namespaces

bool testcase_run_all_tests();

void exampleOfOT();

int main_main(int argc, char **argv); // some tests will execute the main... e.g. against errors in args parsing TODO move to namespace

} // namespace nTests
} // namespace nOT

#endif

