#!/bin/bash

echo "Warning, this will merge in master !!!"
git push origin energy

# merge in the master branch
git checkout master
git merge energy
git push origin master

# push mirror
git push geneassembly master

git checkout energy
