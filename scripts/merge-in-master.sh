#!/bin/bash

echo "Warning, this will merge in master !!!"
git push origin energy
git checkout master
git merge energy
git push origin master
git push geneassembly master
git checkout energy
