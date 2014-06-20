#!/usr/bin/env ruby

if ARGV.size < 2
    puts "Usage"
    puts "Program file.fastq kmerlength"
    exit
end

file = ARGV[0]

kmer_length = ARGV[1].to_i

table = Hash.new

def get_reverse_complement sequence

    new_sequence = sequence.clone
    i = 0
    length = new_sequence.length

    while i < length
        if new_sequence[i] == 'A'
            new_sequence[i] = 'T'
        elsif new_sequence[i] == 'T'
            new_sequence[i] = 'A'
        elsif new_sequence[i] == 'G'
            new_sequence[i] = 'C'
        elsif new_sequence[i] == 'C'
            new_sequence[i] = 'G'
        end
        i += 1
    end

    i = 0

    middle = length / 2

    while i < middle
        other = new_sequence[length - 1 - i]
        new_sequence[length - 1 - i] = new_sequence[i]
        new_sequence[i] = other

        i += 1
    end

    new_sequence
end

def get_canonical_kmer kmer
    reverse_complement = get_reverse_complement kmer

    if reverse_complement < kmer
        reverse_complement
    else
        kmer
    end
end

line_number = 0
File.open(file).each do |line|

    if line_number % 1000 == 0
#puts line_number
    end

    if line_number % 4 == 1
        sequence = line.strip
        length = sequence.length

        position = 0
        kmers = length - kmer_length + 1

        while kmers > 0
            kmer = sequence[position..(position + kmer_length - 1)]

            canonical_kmer = get_canonical_kmer kmer

            unless table.key? canonical_kmer
                table[canonical_kmer] = 0
            end

            table[canonical_kmer] += 1

            position += 1
            kmers -= 1
        end
    end

    line_number += 1
end

distribution = Hash.new

table.each do |key, value|

    unless distribution.key? value
        distribution[value] = 0
    end

    distribution[value] += 1
end

distribution.keys.sort.each do |coverage|
    puts coverage.to_s + " " + distribution[coverage].to_s
end
