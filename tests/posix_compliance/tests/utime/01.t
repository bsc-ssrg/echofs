#!/bin/sh

desc="utime returns ENOENT if the filename does not exist"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..4"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
expect ENOENT utime ${n0}/${n1}
expect ENOENT utime ${n0}/${n1} 1234 5678
expect 0 rmdir ${n0}
