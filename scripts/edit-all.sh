#!/bin/bash

vim $((find .|grep '\.c'$;find .|grep '\.h'$)|grep -v '\.git')
