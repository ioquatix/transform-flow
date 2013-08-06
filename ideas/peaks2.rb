#!/usr/bin/env ruby

LCSNode = Struct.new(:value, :previous)

# Find the Longest Common Subsequence in the given sequences x, y.
def peaks(s)
	p = []
	
	s.each.with_index do |v, i|
		p << [v, i] unless v == 0
	end
	
	p.sort.reverse
end

def peak_tree(s)
	nodes = []
	
	s.each
	
	return nodes
end

t1 = [0, 0, 0, 5, 0, 0, 9, 0, 0, 6, 4]
t2 = [4, 0, 0, 8, 0, 0, 6, 4, 0, 0, 0]
e = -2

def error(t1, t2, k)
	cost = 0
	
	t1.each.with_index do |v, i|
		j = i + k
		
		next if j < 0 || j >= t2.size
		puts " ** Comparing items #{i} #{j} = #{(v - t2[j]).abs ** 2}"
		
		cost += (v - t2[j]).abs ** 2
	end
	
	return cost
end

def global_errors(t1, t2, k = 0)
	min = -t2.size() / 2
	max = t2.size() / 2
	
	confidence = {}
	cost = {}
	
	
	.each do |s|
		puts "Calculating error with offset #{s}..."
		
		e = error(t1, t2, s)
		
		puts "Error = #{e}"
		
		errors << [s, e]
	end
	
	return errors
end

puts global_errors(t1, t2).inspect