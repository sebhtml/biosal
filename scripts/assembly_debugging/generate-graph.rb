#!/usr/bin/env ruby

require 'set'

if ARGV.length != 2
    puts "Usage"
    puts "generate-graph.rb file.fasta k"
    puts "Example: generate-graph.rb file.fasta 51"
    exit
end

file = ARGV[0]
k = ARGV[1].to_i

sequence = ""

File.open(file).each do |line|
    if line.length >0 and line[0] != '>'
        sequence += line.strip()
    end
end

vertices = Hash.new
i = 0
total_length = sequence.length
#puts "total_length= " + total_length.to_s

while i < total_length - k + 1

    vertex = sequence[i..(i + k - 1)]

    #puts vertex.length

    if !vertices.has_key?(vertex)
        vertices[vertex] = {:parents => Set.new, :children => Set.new}
    end
    i += 1
end

previous = nil

i = 0
while i < total_length - k + 1

    vertex = sequence[i..(i + k - 1)]

    if previous != nil
        vertices[previous][:children].add(vertex)
        vertices[vertex][:parents].add(previous)
    end

    previous = vertex
    i += 1
end

i = 0
first = sequence[i..(i + k - 2)]
i = total_length - k + 1
last = sequence[i..(i + k - 2)]

if first == last
    i = 0
    vertex = sequence[i..(i + k - 1)]
    i = total_length - k
    previous = sequence[i..(i + k - 1)]

    #puts previous + " -> " + vertex
    vertices[previous][:children].add(vertex)
    vertices[vertex][:parents].add(previous)
end

#exit

puts "digraph G {"

cycle = 1

vertices.each do |vertex, links|

    puts vertex

    if links[:parents].size != 1 or links[:children].size != 1
        cycle = 0
    end

    links[:parents].each do |other|
        puts other + " -> " + vertex
    end
    links[:children].each do |other|
        puts vertex + " -> " + other
    end
end

puts "/* cycle " + cycle.to_s + " */ "
puts "}"
