#!/bin/bash
#
#  LinuxBIOS autobuild
#
#  This script builds LinuxBIOS images for all available targets.
#
#  (C) 2004 by Stefan Reinauer <stepan@openbios.org>
#
#  This file is subject to the terms and conditions of the GNU General
#  Public License. See the file COPYING in the main directory of this
#  archive for more details.
#     

#set -x # Turn echo on....

# Where shall we place all the build trees?
TARGET=$( pwd )/linuxbios-builds

# path to payload. Should be more generic
PAYLOAD=/dev/null

# Lines of error context to be printed in FAILURE case
CONTEXT=5

# One might want to adjust these in case of cross compiling
MAKE="make"
PYTHON=python

ARCH=`uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
	-e s/arm.*/arm/ -e s/sa110/arm/ -e s/x86_64/amd64/ \
	-e "s/Power Macintosh/ppc/"`

function vendors
{
	# make this a function so we can easily select
	# without breaking readability
	ls -1 "$LBROOT/src/mainboard" | grep -v CVS
}

function mainboards
{
	# make this a function so we can easily select
	# without breaking readability
	
	VENDOR=$1
	
	ls -1 $LBROOT/src/mainboard/$VENDOR | grep -v CVS 
}

function architecture
{
	VENDOR=$1
	MAINBOARD=$2
	cat $LBROOT/src/mainboard/$VENDOR/$MAINBOARD/Config.lb | \
		grep ^arch | cut -f 2 -d\ 
}

function create_config
{
	VENDOR=$1
	MAINBOARD=$2
	echo -n "  Creating config file..."
	mkdir -p $TARGET
	( cat << EOF
# This will make a target directory of ./VENDOR_MAINBOARD

target VENDOR_MAINBOARD
mainboard VENDOR/MAINBOARD

option CC="CROSSCC"
# not supported yet
# option LD="CROSSLD"

romimage "normal"
	option USE_FALLBACK_IMAGE=0
	option ROM_IMAGE_SIZE=0x12000
	option LINUXBIOS_EXTRA_VERSION=".0-normal"
	payload PAYLOAD
end

romimage "fallback" 
	option USE_FALLBACK_IMAGE=1
	option ROM_IMAGE_SIZE=0x12000
	option LINUXBIOS_EXTRA_VERSION=".0-fallback"
	payload PAYLOAD
end

buildrom ./VENDOR_MAINBOARD.rom ROM_SIZE "normal" "fallback"
EOF
	) | sed -e s,VENDOR,$VENDOR,g \
		-e s,MAINBOARD,$MAINBOARD,g \
		-e s,PAYLOAD,$PAYLOAD,g \
		-e s,CROSSCC,"$CC",g \
		-e s,CROSSLD,"$LD",g \
		> $TARGET/Config-${VENDOR}_${MAINBOARD}.lb
	echo " ok"
}

function create_builddir
{	
	VENDOR=$1
	MAINBOARD=$2
	
	echo -n "  Creating builddir..."

	target_dir=$TARGET
	config_dir=$LBROOT/util/newconfig
	yapps2_py=$config_dir/yapps2.py
	config_g=$config_dir/config.g
	config_lb=Config-${VENDOR}_${MAINBOARD}.lb

	cd $target_dir

	build_dir=${VENDOR}_${MAINBOARD}
	config_py=$build_dir/config.py

	if [ ! -d $build_dir ] ; then
		mkdir -p $build_dir
	fi
	if [ ! -f $config_py ]; then
		$PYTHON $yapps2_py $config_g $config_py &> $build_dir/py.log
	fi

	# make sure config.py is up-to-date

	export PYTHONPATH=$config_dir
	$PYTHON $config_py $config_lb $LBROOT &> $build_dir/config.log
	if [ $? -eq 0 ]; then
		echo "ok"
	else
		echo "FAILED! Log excerpt:"
		tail -n $CONTEXT $build_dir/config.log
		return 1
	fi
}

function create_buildenv
{
	VENDOR=$1
	MAINBOARD=$2
	create_config $VENDOR $MAINBOARD
	create_builddir $VENDOR $MAINBOARD
}

function compile_target
{	
	VENDOR=$1
	MAINBOARD=$2

	echo -n "  Compiling image .."
	CURR=$( pwd )
	cd $TARGET/${VENDOR}_${MAINBOARD}
	eval $MAKE &> make.log
	if [ $? -eq 0 ]; then
		echo "ok" > compile.status
		echo "ok."
		cd $CURR
		return 0
	else
		echo "FAILED! Log excerpt:"
		tail -n $CONTEXT make.log
		cd $CURR
		return 1
	fi
}

function built_successfully
{
	CURR=`pwd`
	status="fail"
	if [ -d "$TARGET/${VENDOR}_${MAINBOARD}" ]; then
		cd $TARGET/${VENDOR}_${MAINBOARD}
		if [ -r compile.status ] ; then
			status=`cat compile.status`
		fi
		cd $CURR
	fi
	[ "$buildall" != "true" -a "$status" == "ok" ]
}
function build_target
{
	VENDOR=$1
	MAINBOARD=$2
	TARCH=$( architecture $VENDOR $MAINBOARD )

	# default setting
	CC="gcc"
	LD="ld"
	
	echo -n "Processing mainboard/$VENDOR/$MAINBOARD"
	
	if [ "$ARCH" == "$TARCH" ]; then
		echo " ($TARCH: ok)"
	else
		found_crosscompiler=false
		if [ "$ARCH" == amd64 -a "$TARCH" == i386 ]; then
			CC="gcc -m32"
			found_crosscompiler=true
			echo " ($TARCH: subset of $ARCH)"
		fi
		if [ "$ARCH" == ppc64 -a "$TARCH" == ppc ]; then
			CC="gcc -m32"
			found_crosscompiler=true
			echo " ($TARCH: subset of $ARCH)"
		fi

		# TBD: look for suitable cross compiler suite
		# cross-$TARCH-gcc and cross-$TARCH-ld
		
		# Check result:
		if [ $found_crosscompiler == "false" ]; then
			echo " ($TARCH: skipped, we're $ARCH)"
			return 0
		fi
	fi
	
	if ! built_successfully $VENDOR $MAINBOARD  ; then
		create_buildenv $VENDOR $MAINBOARD
		if [ $? -eq 0 ]; then
			compile_target $VENDOR $MAINBOARD
		fi
	else
		echo " ( mainboard/$VENDOR/$MAINBOARD previously ok )"
	fi
	echo
}

function myhelp
{
	echo "Usage: $0 [-v|--verbose] [-a|--all] [-t|--target vendor/board] [lbroot]"
	echo "       $0 [-V|--version]"
	echo "       $0 [-h|--help]"
	exit 0
}

function myversion 
{
	cat << EOF

LinuxBIOS autobuild: V0.1.

Copyright (C) 2004 by Stefan Reinauer, <stepan@openbios.org>
This program is free software; you may redistribute it under the terms
of the GNU General Public License. This program has absolutely no
warranty.

EOF
	myhelp
	exit 0
}

# default options
target=""
buildall=false

# parse parameters
args=`getopt -l version,verbose,help,all,target: Vvhat: -- "$@"`

if [ $? != 0 ]; then
	myhelp
	exit 1
fi

eval set -- "$args"
while true ; do
	case $1 in
		-t|--target)	shift; target=$1; shift;;
		-a|--all)	shift; buildall=true;;
		-v|--verbose)	shift; verbose=true;;
		-V|--version)	shift; myversion;;
		-h|--help)	shift; myhelp;;
		--)		shift; break;;
		*)		echo "Unrecognized argument" ; exit 1 ;;
	esac
done

LBROOT=$1

# /path/to/freebios2/
if [ -z "$LBROOT" ] ; then
	LBROOT=$( cd ../..; pwd )
fi
echo "LBROOT=$LBROOT"

if [ "$target" != "" ]; then
  # build a single board
  VENDOR=`echo $target|tr -d \'|cut -f1 -d/`
  MAINBOARD=`echo $target|tr -d \'|cut -f2 -d/`
  build_target $VENDOR $MAINBOARD
else
  # build all boards per default
  for VENDOR in $( vendors ); do
    for MAINBOARD in $( mainboards $VENDOR ); do
  	build_target $VENDOR $MAINBOARD
    done
  done
fi

