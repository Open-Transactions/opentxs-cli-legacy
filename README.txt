
Project otcli for Open-Transactions: main documentation.
See directory doc/ and doc/doc.txt for advanced documentation too.


========================================================================
START
------------------------------------------------------------------------
This is "otcli" a sub project for Open-Transactions (secure messages and banking software - OT)
that provides tools for it:
* part otcli - is new Command Line Interface for easy access to advanced modern OT functions
* part othint - is a nice wrapper providing console line and bash autocomplete for otcli commands

This is an addon/wrapper for Open-Transactions (OT), it needs OT sources/builded for headers/libs,
and it usually needs runnint OT server process to connect to (normally it would be running
on same computer/user with data in ~/.ot/ later full networking with external server will work)
------------------------------------------------------------------------
* do "make" or possibly first "cmake ." or see BUILDING
* do "make run" or ./othint will run program - or see USING

After program is built, execute

========================================================================

Relation with Open-Transactions (in run-time)

(current computer):
~/.ot/otcli/ <--- private store for otcli/othint own config/cache/data
~/.ot/client_data/ <--- here otcli will save not-yet-pushed OT data
(possibly on external computer - but easier on localhost same user):
~/.ot/server_data/ <--- here some external server will operate to which we will talk using OT
Once OT federation will work fully, external server will talk to yet another server and push
our data to end users on any server.


========================================================================
USING
------------------------------------------------------------------------
(Assuming project is already compiled or at least set up to compile correctly)

To test othint part:
Run "make run" or execute the binary ./othint --complete-shell
Now you are in interactive shell called
newcli (Open Transactions NEW Command-Line Interface)
here you execute OT commands, and auto-completion (tab key) works!

The otcli part is not currently delivered as separate program, use othint instead.
========================================================================

========================================================================
COOL THINGS TO DEMO

You can try or demonstrate following things: 
* try adding server example_data/ot-servers/DigitalisOTserver.otc
------------------------------------------------------------------------

========================================================================
BUILDING
------------------------------------------------------------------------

How to building/compile otcli from source code:

0) Fast full rebuild
1) Build otcli - choosing compiler
2) Build otcli - dependencies
3) Build otcli - main

0) If you system was already configured then just do ./full-rebuild.sh ; Else:

---------------------------------------
1) Build othint - choosing compiler
Project otcli does use C++11, and therefore requires clang >= 3.3, or gcc >= 4.7 , MSVC12 (2013). 
Maybe some older versions could work too, or other compiler with good enough C++11 support.
Even if Open-Transactions main project builded correctly, it is possible that you will need
to set up newer compiler for this otcli sub-project.

Common solution is to install llvm 3.3 or higher locally (in ~/.local/) and then in ~/.bashrc append:
export CC="ccache $HOME/.local/bin/clang" ; export CXX="ccache $HOME/.local/bin/clang++" ; export CPP="$HOME/.local/bin/clang -E"
read details in global documentation: Open-Transactions/docs/INSTALL-linux-modern.txt
do not forget to reload ~/.bashrc (or start new bash) and delete cmake cache after changes.

If use a not-tested compiler and get error FATAL_ERROR_COMPILER, then see change cmake options
to ignore this error and try to continue anyway. See cmake settings below.

---------------------------------------
2) Build otcli - dependencies
- System libs, tools
- Editline
- OpenTransactions (development - with includes headers)

The dependencies here should build also with older compiler versions, so the point of selecting
compiler applies rather to the main build of ot.

Dependencies installation (Debian):
	* Download OpenTransactions and build it, using instructions from  ../../docs/INSTALL-linux-modern.txt or other suitable
	* Get, build and install latest editline locally (editline in debian 7 has bugs)
		wget http://thrysoee.dk/editline/libedit-20130712-3.1.tar.gz
         	sha256sum libedit-20130712-3.1.tar.gz 
		echo "THE CHECKSUM ABOVE SHOULD BE: 5d9b1a9dd66f1fe28bbd98e4d8ed1a22d8da0d08d902407dcc4a0702c8d88a37  press enter if ok. "
		read ok
		tar -xzf libedit-20130712-3.1.tar.gz
		cd libedit-20130712-3.1
		./configure --prefix=$HOME/.local
		make
		make install

---------------------------------------
3) Build otcli - main

Build with default configuration:
	cmake .
	make

If you need to tweak cmake build options e.g. some library path
or if you get FATAL_ERROR_COMPILER and you want to skip compiler
check and try to compiler anyway then:

ON LINUX/UNIX: (e.g. Debian)
you can make it from command line:
	cmake . -DLOCAL_EDITLINE=ON -DWITH_WRAPPER=ON
or instead use GUI: install this 2 (or one of them):
	aptitude install cmake-curses-gui cmake-gui
and then as user in the otcli sources directory do either of:
	ccmake .     # (then: configure [c] and generate [g] in gui)
	cmake-gui .  # (and follow the graphical instructions)
After doing such change run the command "make" again.

ON OTHER SYSTEMS (e.g. Windows)
follow system specific way to choose/use given compiler.

TODO @vyrly - Windows cmake + MSVC short description.

More about FATAL_ERROR_COMPILER:
to disable this check, you can use
	cmake  -DCHECK_COMPILER_VERSION=OFF . 
or use the gui tools to switch it.

========================================================================
MORE ON OPEN-TRANSACTIONS GENERAL INFO
------------------------------------------------------------------------

Read other text files distributed here for more information;
Also read the main documentation of Open-Transactions itself, their readme, wiki.


========================================================================
OTCLI USER MANUAL - IMPORTANT INFO
------------------------------------------------------------------------

Here we store information for users;
Especially information that is not yet written in more proper place.
Please read this section to be up to date with usage of otcli!

------------------------------------------------------------------------
IMPORTANT SYNTAX AND NOTATION
------------------------------------------------------------------------

%foobar12345 the % marker in certain command means that the text is a fingerprint/nym instead e.g. alias.
For example this can be used in command ot msg sendfrom bob %aaa12345

^alice the ^ marker means this is an alias instead of fingerprint/nym

This options are not yet fully implemented. TODO



========================================================================
MORE DOCUMENTATION - ADVANCED - DEVELOPERS - HACKING - ETC
------------------------------------------------------------------------

See directory doc/ and doc/doc.txt for advanced documentation too.


