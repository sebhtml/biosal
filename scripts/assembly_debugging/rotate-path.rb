#!/usr/bin/env ruby

if ARGV.length != 3
    puts "Usage"
    puts "rotate-path.rb file.fasta k r"
    puts "Example: rotate-path.rb file.fasta 51 300"
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

def rotate_path(sequence, l, k, r)
# TODO
end

puts "Before " + sequence
rotate_path(sequence, sequence.length, k, rotation)
puts "After" + sequence
