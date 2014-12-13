#!/bin/bash

repository=git://github.com/sebhtml/biosal.git
branch=master
app_path=biosal/$app_path
job_template=biosal/$job_template

app=$(basename $app_path)
sample=$(basename $dataset)
job=$app-$sample-$center-$system-$nodes-$threads-$(date +%Y-%m-%d-%H-%M-%S)

if ! test -e $root
then
    mkdir -p $root
fi

cd $root

if ! test -e $sample
then
    ln -s $dataset $sample
fi

if ! test -e biosal
then
    git clone $repository
fi

cd biosal
git checkout $branch
git pull origin $branch

commit=$(git log | head -n1 | awk '{print $2}')

$build_script

cd ..
cp $app_path $job.$app

cp $job_template $job.sh

template="s/__JOB__/$job/g"
sed -i "$template" $job.sh

template="s/__APP__/$app/g"
sed -i "$template" $job.sh

template="s/__SAMPLE__/$sample/g"
sed -i "$template" $job.sh

template="s/__COMMIT__/$commit/g"
sed -i "$template" $job.sh

$submit_command $job.sh > $job.job

echo "Submitted build $job ($commit)"
cat $job.job
