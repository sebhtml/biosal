#!/bin/bash

echo "Warning, this will merge in master !!!"
git push sebhtml energy
git checkout master
git merge energy
git push sebhtml master
git push geneassembly master
git checkout energy
