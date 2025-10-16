#!/bin/bash

project=Terrain
path=$(pwd)

run_command ()
{
	if [[ $1 == "build" ]] || [[ $1 == "bd" ]]; then
		if ! test -d $path/build; then
			mkdir build
		fi
		cd $path/build
		cmake ..

	elif [[ $1 == "clean" ]] || [[ $1 == "cl" ]]; then
		rm -rf $path/build

	elif [[ $1 == "run" ]] || [[ $1 == "r" ]]; then
		if [[ $OSTYPE == "linux-gnu" ]] || [[ $OSTYPE == "darwin" ]]; then
			if test -f $path/build/$project; then
				cd $path/build
				./$project $args
			else
				echo "ERROR: '"$path"/build/"$project"' could not be found."
				exit 1
			fi
		elif [[ $OSTYPE == "msys" ]]; then
			if test -f $path/build/Release/$project; then
				cd $path/build
				./Release/$project $args
			else
				echo "ERROR: '"$path"/build/Release/"$project"' could not be found."
				exit 1
			fi
		fi

	elif [[ $1 == "compile" ]] || [[ $1 == "cmp" ]]; then
		if [[ $OSTYPE == "linux-gnu" ]]; then
			if test -f $path/build/Makefile; then
				cd $path/build
				make -j
			else
				echo "ERROR: '"$path"/build/Makefile' could not be found."
				exit 1
			fi
		elif [[ $OSTYPE == "msys" ]]; then
			cd $path/build
			cmake --build . -j --config Release
		fi
	fi
}

if ! test -f setup.sh; then
	echo "ERROR: please run the setup script from the root project directory."
	exit 1
fi

for x in $@; do
	if [[ $x == "-"* ]]; then
		args+=${x/-/}
		args+=" "
	fi
done

for x in $@; do
	run_command $x
done