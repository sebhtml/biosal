#!/bin/bash

echo "Warning, this will merge in master !!!"

echo "Push energy"
git push origin energy &> /dev/null

echo "Merge energy into master"
(

# merge in the master branch
git checkout master
git merge energy
) &> /dev/null


echo "Push to master"
(
git push origin master
) &> /dev/null

echo "Push to mirror"
(
# push mirror
git push geneassembly master

git checkout energy
) &> /dev/null

