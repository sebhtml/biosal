#!/bin/bash

root=/projects/CompBIO/Projects/automated-tests
repository=git://github.com/sebhtml/biosal.git
branch=master
dataset=/gpfs/mira-fs1/projects/CompBIO/Datasets/JGI/Great_Prairie_Soil_Metagenome_Grand_Challenge/Datasets/Iowa_Native_Prairie_Soil

__APP__=spate
__SAMPLE__=$(basename $dataset)
__JOB__=$__APP__-$__SAMPLE__-1024x16-$(date +%Y-%m-%d-%H-%M-%S)

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

scripts/IBM_Blue_Gene_Q/build-spate-xl.sh

cd ..
cp biosal/applications/spate_metagenome_assembler/spate $__JOB__.$__APP__

cp biosal/tests/Cetus_IBM_Blue_Gene_Q/Template.$__APP__.sh $__JOB__.sh

template="s/__JOB__/$__JOB__/g"
sed -i "$template" $__JOB__.sh

template="s/__APP__/$__APP__/g"
sed -i "$template" $__JOB__.sh

template="s/__SAMPLE__/$__SAMPLE__/g"
sed -i "$template" $__JOB__.sh

template="s/__COMMIT__/$__COMMIT__/g"
sed -i "$template" $__JOB__.sh

./$__JOB__.sh > $__JOB__.job

echo "Submitted build $__JOB__ ($__COMMIT__)"
cat $__JOB__.job
