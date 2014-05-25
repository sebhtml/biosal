#!/bin/bash

wc -l $((find .|grep '\.c'$;find .|grep '\.h'$)|grep -v '\.git')|sort -r -n
