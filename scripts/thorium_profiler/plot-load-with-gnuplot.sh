#!/bin/bash

file=$1

# http://lowrank.net/gnuplot/plot3d2-e.html

echo "
reset
set term gif animate delay 30
#set pm3d
set output '$file.gif'
set ylabel 'Time (seconds'
set xlabel 'Node'
set zlab 'Load'

do for [angle=0:350:5] {
    set view 50, angle
    splot '$file' with lines lc palette
    #splot '$file'
}

" | gnuplot
