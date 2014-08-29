#!/bin/bash

file=$1

# http://lowrank.net/gnuplot/plot3d2-e.html

echo "
reset
set term png
set output '$file.map.png'
set ylabel 'Time (seconds)'
set xlabel 'Node'
set zlab 'Load'

set view 0, 90
splot '$file' with pm3d

" | gnuplot

echo "
reset
set term gif animate delay 15
set output '$file.animation.gif'
set ylabel 'Time (seconds)'
set xlabel 'Node'
set zlab 'Load'

do for [angle=0:350:2] {
    set view 50, angle
    splot '$file' with pm3d
}

" | gnuplot


