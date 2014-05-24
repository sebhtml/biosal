#!/bin/bash

make clean
grep "if("  * -R|grep -v git
grep "for(" * -R|grep -v git
grep " $" * -R|grep -v git
grep "	" * -R|grep -v Makefile|grep -v git
