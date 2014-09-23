#!/usr/bin/env Rscript

arguments = commandArgs(trailingOnly = TRUE)
file = arguments[1]
data = read.table(file)

lines = length(data[,1])

#print("Lines")
#print(lines)

minimum = -1
maximum = -1

i = 1

while (i <= lines) {
    start = data[,2][i]
    end = data[,3][i]

    if (minimum == -1 || start < minimum) {
        minimum = start
    }

    if (maximum == -1 || end > maximum) {
        maximum = end
    }

    i = i + 1
}

png("file.png", width=1000, height=1000)
plot(c(0), c(0), xlim = c(minimum, maximum), ylim = c(0, 1), type='l', col='red')

i = 1

while (i <= lines) {
    start = data[,2][i]
    end = data[,3][i]

    lines(c(start, end), c(1, 1), col='blue')
    i = i + 1
}

dev.off()
