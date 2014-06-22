#!/usr/bin/env ruby

# This script is used to parse LOAD values from
# a biosal computation. The biosal option -print-load
# is required to generate LOAD data

i = 0
content = Hash.new

def print_content content
    if content.nil?
        return
    end

    tokens = content.split
    i = 3
    j = 0
    while j < 4
        print " " + tokens[i].gsub("]", "").strip
        i += 2
        j += 1
    end
end

second = 0
period = 2

File.open(ARGV[0]).each do |line|

    content[line.split[1]] = line.strip
    i += 1

    if i == 4
        j = 0
        print second.to_s
        while j < 4
            print_content content["node/" + j.to_s]
            j += 1
        end
        puts ""
        i = 0
        second += period
        content = Hash.new
    end
end
