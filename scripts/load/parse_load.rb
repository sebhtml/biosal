#!/usr/bin/env ruby

# This script is used to parse LOAD values from
# a biosal computation. The biosal option -print-load
# is required to generate LOAD data

if ARGV.size == 0
    puts "Usage"
    puts "parse_load.rb load.txt 4"
    exit
end

content = Hash.new

nodes = ARGV[1].to_i
# LOAD EPOCH 10 s node/2 0.39 0.00 0.29 0.00 0.29 0.00
File.open(ARGV[0]).each do |line|

    tokens = line.strip.split

    time = "mock"
    real_time = tokens[2]
    node = tokens[4]

    if content[time].nil?
        content[time] = Hash.new
    end

    content[time][node] = line.strip

    if content[time].size == nodes
        node = 0
        name = "node/" + node.to_s

        print real_time
        while node < nodes
            data = content[time][name]

            tokens = data.split

            i = 0
            for token in tokens
                if i >= 5
                    print " " + token
                end
                i += 1
            end
            node += 1
        end
        puts ""

        content[time] = Hash.new
    end
end
