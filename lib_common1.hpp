/* See other files here for the LICENCE that applies here. */


#ifndef INCLUDE_OT_NEWCLI_COMMON1
#define INCLUDE_OT_NEWCLI_COMMON1

#include <string>
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <cstring>

#include <functional>
#include <memory>


// list of thigs from libraries that we pull into namespace nOT::nNewcli
// we might still need to copy/paste it in few places to make IDEs pick it up correctly
#define INJECT_OT_COMMON_USING_NAMESPACE_COMMON_1 \
	using std::string; \
	using std::vector; \
	using std::list; \
	using std::set; \
	using std::map; \
	using std::cin; \
	using std::cerr; \
	using std::cout; \
	using std::cerr; \
	using std::endl; \
	using std::function; \
	using std::unique_ptr; \
	using std::shared_ptr; \
	using std::weak_ptr; \
	using std::enable_shared_from_this; \

#endif

