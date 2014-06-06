
-----------------
Design notes, while commuting with the Metra Train, Pace Bus, and Shuttle Bus

to put in intput/input_controller.c

4 sequence stores per node
4 graph stores per node

----------------------
64 sequence stores
10000001 sequences

per sequence store (default)
156250, 156250, ..., 156251

-------------
better (quadratic split, constant number of blocks):

64 * 64 blocks = 4096 blocks

10000001 / 4096
2441, 2441, 2441, ... 4106

block i goes to sequence store (i % stores)

irb(main):023:0> 2441 * 4095 + 1 * 4106
=> 10000001


256 nodes
1024 graph stores

-----
best
256 nodes
4 sequence stores per node
1024 sequence stores
block size 4096
3000000000 sequences
blocksize 4096 sequences



