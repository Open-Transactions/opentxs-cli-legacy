#!/bin/sh

rm /tmp/ot.in
rm /tmp/ot.out.*
killall othint
sleep 0.1
killall -9 othint
sleep 0.1


pidfile_cli=$HOME/.ot/client_data/ot.pid
if [ -r $pidfile_cli ] ; then
	echo "Deleting the client flag $pidfile_cli"
	rm "$pidfile_cli"
fi

export MALLOC_CHECK_=3
# gdb -silent -batch -x run.gdb  --args ./othint +debugcerr --complete-shell

./othint +debugcerr --complete-one "ot ms"
./othint +debugcerr --complete-one "ot msg sen"
./othint +debugcerr --complete-one "ot msg sen"

