#!/bin/sh

pidfile_cli=$HOME/.ot/client_data/ot.pid
if [ -r $pidfile_cli ] ; then
	echo "Deleting the client flag $pidfile_cli"
	rm "$pidfile_cli"
fi

export MALLOC_CHECK_=3
# gdb -silent -batch -x run.gdb  --args ./othint +debugcerr --complete-shell
gdb -return-child-result -ex run -ex "thread apply all bt" -ex "quit" --args ./othint +debugcerr --complete-shell
