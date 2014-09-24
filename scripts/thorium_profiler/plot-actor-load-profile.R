#!/usr/bin/env Rscript

arguments = commandArgs(trailingOnly = TRUE)
file = arguments[1]
data = read.table(file)

lines = length(data[,1])

#print("Lines")
#print(lines)

minimum = -1
maximum = -1

minimum_y = -1
maximum_y = -1

i = 1

while (i <= lines) {
    start = data[,1][i]
    end = data[,2][i]
    actor = data[,3][i]

    if (minimum == -1 || start < minimum) {
        minimum = start
    }

    if (maximum == -1 || end > maximum) {
        maximum = end
    }

    if (minimum_y == -1 || actor < minimum_y) {
        minimum_y = actor
    }

    if (maximum_y == -1 || actor > maximum_y) {
        maximum_y = actor
    }

    i = i + 1
}

png("file.png")
#, width=4000, height=1000)
plot(c(0), c(0), xlim = c(minimum, maximum), ylim = c(minimum_y, maximum_y), type='l', col='red',
                xlab='Time (nanoseconds)', ylab='Actor', main="Actor load profiles")

i = 1

while (i <= lines) {
    start = data[,1][i]
    end = data[,2][i]
    actor = data[,3][i]

    lines(c(start, end), c(actor, actor), col='black')
    i = i + 1
}

dev.off()
