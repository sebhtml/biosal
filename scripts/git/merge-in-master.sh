#!/bin/bash

echo "Warning, this will merge in master !!!"

echo "Push energy"
(
git push origin energy
git push --tags origin
) &> energy-git-1.log

echo "Merge energy into master"

(
git checkout master
git merge energy
) &> master-git-1.log


echo "Push to master"

(
git push origin master
git push --tags origin
) &> master-git-2.log

