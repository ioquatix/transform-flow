#!/usr/bin/env ruby

t1 = [0, 0, 0, 5, 0, 0, 9, 0, 0, 6, 4]
t2 = [4, 0, 0, 8, 0, 0, 6, 4, 0, 0, 0]
e = -2

tc = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
ti = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

class Alignment
	def initialize(a, b, offset, cost)
		@a = a
		@b = b
		
		@indices = {}
		@cost = {}
		
		@estimated_offset = 0
		
		calculate_peaks
	end
	
	attr :estimated_offset, true
	
	attr :cost
	attr :offset
	attr :peaks
	
	# Calculates the range of possible offsets for overlapping a, b.
	def offsets
		(-@b.length + 1)...(@a.length)
	end
	
	def update_cost(offset)
		i = @indices[offset] ||= 0
		
		value, j = @peaks[i]
		
		error = (@a[j] - @b[offset + j]).abs ** 2
		
		@cost[offset] ||= 0
		@cost[offset] += error
		
		# Increment as we've now included this cost:
		@indices[offset] += 1
	end
	
	def calculate_peaks
		# Find peaks in reverse order, O(NlogN)
		@peaks ||= @a.collect.with_index.to_a.sort{|a,b| b[0] <=> a[0]}
	end
	
	def calculate_overlap
		
	end
end

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

def incrementally_update_error(offset)
	i = ti[offset]
	j = ti[offset] + offset
	
	ti[offset] += 1
	
	# The cost of the given offset is updated based in the next index:
	tc[offset] += (t1[i] - t2[j]).abs ** 2
	
	va = t1[]
	
	j = tc[i] + k
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