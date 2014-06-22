#!/usr/bin/env Rscript

data = read.table('data.txt')

png('load.png', width=4000, height=400)
plot(data[,1], data[,2], type='l')

columns = ncol(data)

i = 3
while (i <= columns) {
    lines(data[,1], data[,i])
    i = i + 1
}
dev.off()
