#!/bin/sh
valgrind --leak-check=full --show-reachable=yes --suppressions=./readline.supp  -- ./othint +debugfile --complete-shell

