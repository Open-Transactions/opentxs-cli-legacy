#!/bin/sh

function cleanup() {
rm /tmp/ot.in
rm /tmp/ot.out.*
killall othint
sleep 0.1
killall -9 othint
sleep 0.1
}


pidfile_cli=$HOME/.ot/client_data/ot.pid
if [ -r $pidfile_cli ] ; then
	echo "Deleting the client flag $pidfile_cli"
	rm "$pidfile_cli"
fi

export MALLOC_CHECK_=3
# gdb -silent -batch -x run.gdb  --args ./othint +debugcerr --complete-shell


function sep() {
	echo "=============================================================================="
	echo "=============================================================================="
	echo "$*"
	echo "=============================================================================="
	echo "=============================================================================="
}

cleanup

sep "1"
./othint +debugfile --complete-one "ot ms"

sep "2"
./othint +debugfile --complete-one "ot msg sen"

sep "3"
./othint +debugfile --complete-one "ot msg sen"

cleanup



