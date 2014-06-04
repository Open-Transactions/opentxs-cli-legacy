#!/bin/sh
export MALLOC_CHECK_=3
gdb -silent -batch -x run.gdb  --args ./othint +debugcerr --complete-shell   
