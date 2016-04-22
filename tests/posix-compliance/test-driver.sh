#!/bin/bash -x

do_retry()
{
	cmd="$1"
	retry_times=$2
	retry_wait=$3

	c=0
	while [ $c -lt $((retry_times+1)) ]; do
		c=$((c+1))
		echo "Executing \"$cmd\", try $c"
		$1 && return $?
		if [ ! $c -eq $retry_times ]; then
			echo "Command failed, will retry in $retry_wait secs"
			sleep $retry_wait
		else
			echo "Command failed, giving up."
			return 1
		fi
	done
}

tests_root=$PWD
root_dir="$PWD/root-dir"
mnt_dir="$PWD/mnt-dir"

#echo "---> '" $srcdir "'"
#echo "---> '" $builddir "'"

# create the root dir for the test
if [[ ! -e ${root_dir} ]]; then
    echo "Creating root dir for tests: ${root_dir}"
    mkdir ${root_dir}
else
    echo "Reusing old root dir for tests: ${root_dir}"
fi

# create the mountpoint for the test
if [[ ! -e ${mnt_dir} ]]; then
    echo "Creating mount point for tests: ${mnt_dir}"
    mkdir ${mnt_dir}
else
    echo "Reusing old mount point for tests: ${mnt_dir}"
fi

# mount the efs-ng filesystem on top
#sudo ../src/efs-ng -r ${mnt_dir} -m ${mnt_dir} -o allow_other,default_permissions,auto_unmount
../../src/efs-ng -r ${root_dir} -m ${mnt_dir} -o default_permissions,auto_unmount

if [[ $? -ne 0 ]];
then
    exit 1
fi

# enter mount point and run the actual testsuite
echo "Running test suite"
(cd ${mnt_dir} && prove -r "${tests_root}")

ret=$?

echo "Unmounting filesystem"
if ! do_retry "fusermount -u ${mnt_dir}" 5 1;
then
    echo "Unable to unmount filesystem. Please do it manually."
fi

if [[ $ret -ne 0 ]];
then
    exit 1
else
    exit 0
fi
