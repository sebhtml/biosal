#!/bin/bash

echo "Warning, this will merge in master !!!"

echo "Push energy"
(
git push origin energy
git push --tags origin energy
) &> energy-git-1.log

echo "Merge energy into master"

(
git checkout master
git merge energy
) &> master-git-1.log


echo "Push to master"

(
git push origin master
git push --tags origin master
) &> master-git-2.log

echo "Push to mirror"

(
git checkout mirror
git merge master
# push mirror
git push geneassembly mirror
git push --tags geneassembly mirror

git checkout energy
) &> mirror-git-1.log

