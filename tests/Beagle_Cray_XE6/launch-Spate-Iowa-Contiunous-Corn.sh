#!/bin/bash

root=/lustre/beagle/CompBIO/automated-tests
repository=git://github.com/sebhtml/biosal.git
branch=master
dataset=/lustre/beagle/stevens/Great_Prairie/Iowa_Continuous_Corn

__APP__=spate
__JOB__=$__APP__-$(date +%Y-%m-%d-%H-%m)
__SAMPLE__=$(basename $dataset)

if ! test -e $root
then
    mkdir -p $root
fi

cd $root

if ! test -e $__SAMPLE__
then
    ln -s $dataset $__SAMPLE__
fi

if ! test -e biosal
then
    git clone $repository
fi

cd biosal
git checkout $branch
git pull origin $branch

scripts/Cray_XE6/build-gnu.sh


cd ..
if ! test -e spate
then
    ln -s biosal/applications/spate_metagenome_assembler/spate
fi

cp biosal/tests/Beagle_Cray_XE6/Template.sh $__JOB__.pbs
