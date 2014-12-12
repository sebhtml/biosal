#!/bin/bash

# environment variables:
#
# BUILD_DEBUG: enable assertions (CONFIG_DEBUG)
# BUILD_COMMIT: build a specific commit, if none, use HEAD

root=/project/projectdirs/m1523/Jobs-Hopper
repository=git://github.com/sebhtml/biosal.git
branch=master
dataset=/scratch/scratchdirs/boisvert/Iowa_Native_Prairie_Soil

__APP__=spate
__SAMPLE__=$(basename $dataset)
__JOB__=$__APP__-$__SAMPLE__-$(date +%Y-%m-%d-%H-%M-%S)

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

__COMMIT__=$(git log | head -n1 | awk '{print $2}')

if test -n "$BUILD_COMMIT"
then
    __COMMIT__=$BUILD_COMMIT

    git checkout $__COMMIT__
fi

module swap PrgEnv-pgi PrgEnv-gnu
make CC=cc -j 4

cd ..

# copy build artifact
cp biosal/applications/spate_metagenome_assembler/spate $__JOB__.$__APP__

# create submission script
cp biosal/tests/Hopper_Cray_XE6/Template.$__APP__.pbs $__JOB__.pbs

template="s/__JOB__/$__JOB__/g"
sed -i "$template" $__JOB__.pbs

template="s/__APP__/$__APP__/g"
sed -i "$template" $__JOB__.pbs

template="s/__SAMPLE__/$__SAMPLE__/g"
sed -i "$template" $__JOB__.pbs

template="s/__COMMIT__/$__COMMIT__/g"
sed -i "$template" $__JOB__.pbs

qsub $__JOB__.pbs > $__JOB__.qsub

echo "Submitted build $__JOB__ ($__COMMIT__)"
cat $__JOB__.qsub
