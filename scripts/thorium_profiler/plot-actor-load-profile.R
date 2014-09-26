#!/usr/bin/env Rscript

arguments = commandArgs(trailingOnly = TRUE)
file = arguments[1]
data = read.table(file, header= TRUE)

threshold = (500 * 1000)
bad_color = 'red'
default_color = 'black'

lines = length(data[,1])

#print("Lines")
#print(lines)

minimum = -1
maximum = -1

minimum_y = -1
maximum_y = -1

i = 1

used_duration = 0
window_start = -1
window_end = -1
last_index = -1

# in nanoseconds (10 ms)
window_maximum_size = (10 * 1000 * 1000)

utilizations = 1:lines

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

    # create window too.

    if (last_index == -1) {
        last_index = i;
    }

    # add the item to the window
    used_duration = used_duration + (end - start)

    if (window_start == -1) {
        window_start = i
    }

    window_end = i

    # Verify if we must reduce the window
    while (window_end >= window_start &&
                    (data[,2][window_end] - data[,1][window_start] > window_maximum_size)) {
        difference = data[,2][window_start] - data[,1][window_start]

        if (used_duration - difference <= window_maximum_size) {
            break;
        }
        used_duration = used_duration - difference
        window_start = window_start + 1
    }

    total = data[,2][window_end] - data[,1][window_start]
    utilization = used_duration / total
    #print(used_duration)
    #print(total)
    utilizations[i] = utilization
    i = i + 1
}

#png(paste(file, ".png", sep=""), width=2000, height=1200)
pdf(paste(file, ".pdf", sep=""))
par(mfrow=c(3,1))

# panel A
plot(data[,1], utilizations, col='black', type='l', ylab='Processor core utilization', xlab='Time (nanoseconds)',
                main=paste('Processor core utilization profile\n(Profile data file: ', file, ')', sep=''))

# panel B
plot(c(-1), c(-1), xlim = c(minimum, maximum), ylim = c(0, 0), type='l', col='red',
                xlab='Time (nanoseconds)', ylab='Timeline', main= paste("Timeline of actor execution (collapsed)\n(any granularity >= ", threshold / 1000, " µs is shown in red)", sep=""))

i = 1

while (i <= lines) {
    start = data[,1][i]
    end = data[,2][i]

    selected_color = default_color

    if ((end - start) >= threshold)
        selected_color = bad_color

    lines(c(start, end), c(0, 0), col=selected_color)
    i = i + 1
}

real_minimum_y = minimum_y
minimum_y = minimum_y - real_minimum_y
maximum_y = maximum_y - real_minimum_y

# panel C
plot(c(0), c(0), xlim = c(minimum, maximum), ylim = c(minimum_y, maximum_y), type='l', col='red',
                xlab='Time (nanoseconds)', ylab='Actor index', main=paste("Timeline of actor execution",
#"\n(any granularity >= ", threshold / 1000, " µs is shown in red)",
                        sep=""))

i = 1

while (i <= lines) {
    start = data[,1][i]
    end = data[,2][i]
    actor = data[,3][i]
    index = actor - real_minimum_y
    selected_color = default_color

    if ((end - start) >= threshold)
        selected_color = bad_color

    lines(c(start, end), c(index, index), col=selected_color)
    i = i + 1
}


dev.off()
