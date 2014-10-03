#!/bin/bash

echo "Push to mirror"

(
git checkout master
git pull origin master
git checkout mirror
git merge master
git push geneassembly mirror
git push --tags geneassembly

git checkout master
) &> mirror-git-1.log

