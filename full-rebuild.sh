echo "This will do full rebuild, using default settings, and assuming you configured things, env, compiler etc"
echo "as explained in README.txt. If you did not, or if you need speciall cmake or other settings then it will not work correctly."

if false ; then
	read  -p "Are you sure? (y=yes) "  -r  ; echo
	if [[ ! $REPLY =~ ^[Yy]$ ]]
	then
		echo "Exiting then." ; exit 1
	fi
fi

./cmake-clear.sh || { echo "cmake clear failed"; exit 2; }
cmake . || { echo "cmake failed"; exit 2; }
make -j 4 || { echo "make failed"; exit 2; }

