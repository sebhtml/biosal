#!/usr/bin/env Rscript

arguments = commandArgs(trailingOnly = TRUE)
file = arguments[1]
data = read.table(file, header= TRUE)

threshold = (500 * 1000)

bad_color = 'red'
default_color = 'black'
bad_color = default_color

lines = length(data[,1])

#print("Lines")
#print(lines)

minimum = -1
maximum = -1

minimum_y = -1
maximum_y = -1

i = 1

utilizations = 1:lines
window_minimum_size = ( 1000 * 1000 )

used_duration = 0
total = 0
start_item = -1
end_item = -1

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

    start_item = i
    end_item = i
    used_duration = end - start
    total = end - start
    j = 1

    while (((total < window_minimum_size) || (end_item - start_item + 1) <= 1)
 && (((i - j) >= 1) || ((i + j) <= lines))) {

        #   [ new_start_item]  [start_item]
        new_start_item = i - j
        if (new_start_item >= 1) {
            used = data[,2][new_start_item] - data[,1][new_start_item]
            used_duration = used_duration + used
            distance = data[,1][start_item] - data[,1][new_start_item]
            total = total + distance
            start_item = new_start_item
        }

        #   [end_item] [new_end_item]
        new_end_item = i + j
        if (new_end_item <= lines) {
            used = data[,2][new_end_item] - data[,1][new_end_item]
            used_duration = used_duration + used
            distance = data[,2][new_end_item] - data[,2][end_item]
            total = total + distance
            end_item = new_end_item
        }

        j = j + 1
    }

#if (i % 1000 == 0)
#print(paste(used_duration, "/", total, " ", start_item, i, end_item))
    utilization = used_duration / total

    items = end_item - start_item + 1

    if (items <= 1)
        utilization = 0

    #print(used_duration)
    #print(total)
    utilizations[i] = utilization
    i = i + 1
}

real_minimum_y = minimum_y
minimum_y = minimum_y - real_minimum_y
maximum_y = maximum_y - real_minimum_y



#png(paste(file, ".png", sep=""), width=2000, height=1200)
pdf(paste(file, ".pdf", sep=""))
par(mfrow=c(3,1))

# panel A
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

# panel B
plot(c(-1), c(-1), xlim = c(minimum, maximum), ylim = c(0, 0), type='l', col='red',
                xlab='Time (nanoseconds)', ylab='Timeline', main= paste("Timeline of actor execution (collapsed)",
# "\n(any granularity >= ", threshold / 1000, " µs is shown in red)",
                        sep=""))

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




# panel A
plot(data[,1], utilizations, col='black', type='l',
#ylim=c(0, 1), 
                ylab='Processor core utilization', xlab='Time (nanoseconds)',
                main=paste('Processor core utilization profile\n(Profile data file: ', file, ')', sep=''))



dev.off()
