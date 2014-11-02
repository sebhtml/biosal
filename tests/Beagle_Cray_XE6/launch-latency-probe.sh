#!/bin/bash

root=/lustre/beagle/CompBIO/automated-tests
repository=git://github.com/sebhtml/biosal.git
branch=master

__APP__=latency_probe
__JOB__=$__APP__-$(date +%Y-%m-%d-%H-%M-%S)

if ! test -e $root
then
    mkdir -p $root
fi

cd $root

if ! test -e biosal
then
    git clone $repository
fi

cd biosal
git checkout $branch
git pull origin $branch

__COMMIT__=$(git log | head -n1 | awk '{print $2}')

scripts/Cray_XE6/build-gnu.sh

cp biosal/performance/latency_probe/latency_probe $__JOB__.$__APP__

cp biosal/tests/Beagle_Cray_XE6/Template.latency_probe.pbs $__JOB__.pbs

template="s/__JOB__/$__JOB__/g"
sed -i "$template" $__JOB__.pbs

template="s/__APP__/$__APP__/g"
sed -i "$template" $__JOB__.pbs

template="s/__COMMIT__/$__COMMIT__/g"
sed -i "$template" $__JOB__.pbs

. /opt/modules/3.2.6.7/init/bash
module purge
module load moab/6.1.1
module load torque/2.5.7

qsub $__JOB__.pbs > $__JOB__.qsub

echo "Submitted build $__JOB__ ($__COMMIT__)"
cat $__JOB__.qsub
