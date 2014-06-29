#!/bin/bash

git commit -as

echo "Push energy"
git push origin energy &> /dev/null
