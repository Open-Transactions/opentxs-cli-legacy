#!/bin/sh
valgrind --leak-check=full --show-reachable=yes --suppressions=./readline.supp  -- ./othint +debugcerr +normal --complete-shell

