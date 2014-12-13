#!/bin/bash

root=/projects/CompBIO/Projects/automated-tests
repository=git://github.com/sebhtml/biosal.git
branch=master

__APP__=latency_probe
__JOB__=$__APP__-alcf-cetus-1024x16-$(date +%Y-%m-%d-%H-%M-%S)

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

scripts/IBM_Blue_Gene_Q/build-xl.sh

cd ..
cp biosal/performance/latency_probe/latency_probe $__JOB__.$__APP__

cp biosal/tests/Cetus_IBM_Blue_Gene_Q/Template.$__APP__.sh $__JOB__.sh

template="s/__JOB__/$__JOB__/g"
sed -i "$template" $__JOB__.sh

template="s/__APP__/$__APP__/g"
sed -i "$template" $__JOB__.sh

template="s/__COMMIT__/$__COMMIT__/g"
sed -i "$template" $__JOB__.sh

./$__JOB__.sh > $__JOB__.job

echo "Submitted build $__JOB__ ($__COMMIT__)"
cat $__JOB__.job
