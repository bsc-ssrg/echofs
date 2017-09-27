# -----------------------------------------------------------------------
# (C) Copyright 2016 Barcelona Supercomputing Center
#                    Centro Nacional de Supercomputacion
# 
# This file is part of the Echo Filesystem NG.
# 
# See AUTHORS file in the top level directory for information
# regarding developers and contributors.
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# 
# The Echo Filesystem NG is distributed in the hope that it will 
# be useful, but WITHOUT ANY WARRANTY; without even the implied 
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU Lesser General Public License for more
# details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with Echo Filesystem NG; if not, write to the Free 
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# -----------------------------------------------------------------------
#
# Common functions for the efs-ng testing infrastructure
#

# directory currently being explored by prove -r
PROVE_CWD=`dirname $0`
echo ${PROVE_CWD} | egrep '^/' >/dev/null 2>&1

if [[ $? -eq 0 ]]; then
    TESTS_BASE_DIR="${PROVE_CWD}/.."
else
    TESTS_BASE_DIR="`pwd`/${PROVE_CWD}/.."
fi

# read config variables
if ! . ${TESTS_BASE_DIR}/config.sh; then
    echo "failed to source ${TESTS_BASE_DIR}/config.sh"
    exit 1
fi

if [[ ! -z $TOP_BUILD_DIR ]]; then
    EFSNG_BIN="$TOP_BUILD_DIR/src/efs-ng"
elif [[ ! -z $EFSNG ]]; then
    EFSNG_BIN="$EFSNG"
else
    echo "[FATAL] Unable to find 'efs-ng' binary"
    exit 1
fi

# we need the filesystem to mirror $TEST_ROOT and 
# be mounted on $TEST_MNT
_require_efsng_mounted()
{

    #_require_scratch_root
    #_require_scratch_mnt
    _require_efsng_unmounted

    # now it's safe to mount the efs-ng filesystem for the test
    _scratch_mnt_mount
}

# force 
_require_efsng_unmounted()
{
    # check if mounted
    mount_rec=`mount | grep fuse.efs-ng`

    if [[ "$mount_rec" != "" ]]; then
        echo "** detected efs-ng running instance"
        # if it's mounted, make sure it's on $TEST_MNT
        if echo $mount_rec | grep -q $TEST_MNT; then
            # and unmount it
            echo "*** stale instance: unmount"
            _scratch_mnt_unmount
        else
            echo "*** independent instance: ignore"
        fi
    else
        echo "** no efs-ng running instances"
    fi
}

# we need a $TEST_ROOT directory to put test data on it 
# so that efs-ng can mirror it
_require_test_root()
{
    if [[ -z $TEST_ROOT ]]; then
        echo "'$TEST_ROOT' not set"
        exit 1
    fi

    # check if the directory exists, and create it if not
    if [ ! -d "$TEST_ROOT" ]; then
        echo "* creating $TEST_ROOT"
        mkdir "$TEST_ROOT"
    else
        echo "* reusing $TEST_ROOT"
    fi
}

# we need a $TEST_MNT directory to mount efs-ng on
_require_test_mnt()
{
    if [[ -z $TEST_MNT ]]; then
        echo "'$TEST_MNT' not set"
        exit 1
    fi

    # check if the directory exists, and create it if not
    if [ ! -d "$TEST_MNT" ]; then
        echo "* creating $TEST_MNT"
        mkdir "$TEST_MNT"
    else
        echo "* reusing $TEST_MNT"
    fi
}

# we may need a $TEST_TMP directory for various tasks
_require_test_tmp()
{
    if [[ -z $TEST_TMP ]]; then
        echo "'$TEST_TMP' not set"
        exit 1
    fi

    # check if the directory exists, and create it if not
    if [ ! -d "$TEST_TMP" ]; then
        echo "* creating $TEST_TMP"
        mkdir "$TEST_TMP"
    else
        echo "* reusing $TEST_TMP"
    fi
}

# generate an ID for this test and store it in $TESTID
_generate_test_id(){
    TMP=${0##./}
    TMP=${TMP/\//_}
    TESTID=${TMP%%.test}
}

# generate a configuration file for efs-ng based on the values
# stored in TEST_ROOT, TEST_MNT, and ???
_generate_efsng_config(){

    if [[ ! -z "$TEST_SUBDIR" ]]; then
        EFSNG_CFG="${TEST_SUBDIR}/${TESTID}.cfg"
        EFSNG_LOG="${TEST_SUBDIR}/${TESTID}.log"
    else
        EFSNG_CFG="${TESTID}.cfg"
        EFSNG_LOG="${TESTID}.log"
    fi

    output=""
    
    # generate header
    printf -v header \
"efs-ng:
{
    root-dir = \"%s\";
    mount-point = \"%s\";
    log-file = \"%s\";

" \
    "$TEST_ROOT" \
    "$TEST_MNT" \
    "$EFSNG_LOG"

    # generate backends
    backends="    backend-stores:\n    (\n"

    if [ ${#DRAM_BACKEND[@]} -ne 0 ]; then

        backends+="        {\n"

        for k in "${!DRAM_BACKEND[@]}"
        do
            backends+="            $k = \"${DRAM_BACKEND[$k]}\";\n"
        done

        if [ ${#nvram_backend[@]} -ne 0 ]; then
            backends+="        },\n"
        else
            backends+="        }\n"
        fi
    fi

    if [ ${#NVRAM_BACKEND[@]} -ne 0 ]; then

        backends+="        {\n"

        for k in "${!NVRAM_BACKEND[@]}"
        do
            backends+="            $k = \"${NVRAM_BACKEND[$k]}\";\n"
        done

        backends+="        }\n"
    fi

    backends+="    )\n"

    # generate preloads
    preloads="    preload:\n    (\n"

    if [ ${#DRAM_PRELOAD_FILES[@]} -ne 0 ]; then
        preloads+="        {\n"

        for f in "${DRAM_PRELOAD_FILES[@]}"
        do
            preloads+="            path = \"$f\";\n"
        done

        preloads+="            backend = \"DRAM\";\n"

        if [ ${#NVRAM_PRELOAD_FILES[@]} -ne 0 ]; then
            preloads+="        },\n"
        else
            preloads+="        }\n"
        fi
    fi

    if [ ${#NVRAM_PRELOAD_FILES[@]} -ne 0 ]; then
        preloads+="        {\n"

        for f in "${NVRAM_PRELOAD_FILES[@]}"
        do
            preloads+="            path = \"$f\";\n"
        done

        preloads+="            backend = \"NVRAM-NVML\";\n"
        preloads+="        }\n"
    fi

    preloads+="    )\n"

    # generate trailer
    trailer="}"

    # assemble and output
    output+="$header"
    output+="$backends"
    output+="$preloads"
    output+="$trailer"

    echo -e "$output" > "$EFSNG_CFG"

    echo -e "$output"
}

_scratch_mnt_mount()
{
    # generate appropriate configuration file
    _generate_efsng_config

    echo "mounting testing instance of efs-ng"

    #../../../build/src/efs-ng -c "$EFSNG_CFG" -o splice_read,splice_write,splice_move
    ${EFSNG_BIN} -c "$EFSNG_CFG" -o splice_read,splice_write,splice_move

    if [[ $? -eq 0 ]]; then
        echo "mount ok"
    fi
}

_scratch_mnt_unmount()
{
    fusermount -u $TEST_MNT
}

# generate random name
_namegen()
{
	echo "fstest_`dd if=/dev/urandom bs=1k count=1 2>/dev/null | md5sum  | cut -f1 -d' '`"
}

_require_test_subdir()
{
    if [[ -z $TESTID ]]; then
        echo "'$TESTID' not set"
        exit 1
    fi

    TEST_SUBDIR="${PWD}/run/test__${TESTID}__"

    if [[ ! -e $TEST_SUBDIR ]]; then
        echo "creating $TEST_SUBDIR"
        if ! mkdir -p $TEST_SUBDIR; then
            echo "failed!"
        fi
    else
        echo "reusing $TEST_SUBDIR"
    fi

    TEST_ROOT="${TEST_SUBDIR}/test_root"
    TEST_MNT="${TEST_SUBDIR}/test_mnt"
    TEST_TMP="${TEST_SUBDIR}/test_tmp"
}

_remove_test_subdir()
{
    if [[ -e "$TEST_SUBDIR" ]]; then
        rm -rf "$TEST_SUBDIR"
    fi
}

_start_test()
{
    if [[ -z $* ]]; then
        echo "1..1"
    else
        echo "1..$1"
    fi
}

# check that 
_expect()
{
    received_res="${1}"
    expected_res="${2}"
    message="${3}"

    if [[ $received_res -ne $expected_res ]]; then
        _abort_test "$message"
    fi
}

_abort_test()
{
    if [[ -z $* ]]; then
        echo "not ok"
    else
        echo "not ok -- $1"
    fi
    exit 0
}

_test_success()
{
    echo "ok"
}

_compare()
{
    file1="$1"
    file2="$2"

    if ! diff -qb "$file1" "$file2"; then
        return 1
    else
        return 0
    fi
}

