#!/bin/sh

# please install here a wrapper script or link to g++(>=4.7.2 ?) or clang(>=3.3 ?)
COMPILER="$HOME/.local/bin/compile-cxx"

# LEGAL warning: check if this commands are linking with any libraries that would require you, in some law systems, to change licence of your code too.
# Read the licence remarks text (see source code and readme and documentation).

$COMPILER -std=c++11 -DOT_ALLOW_GNU_LIBRARIES=1 main.cpp -g3 -O0 -o othint -ledit -lcurses && echo "Starting program:" && bash start.sh ; echo "Done."

# -lreadline

