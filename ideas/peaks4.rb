#!/usr/bin/env ruby

require 'algorithms'
require 'terminal-table'

class Array
	def rjust(n, x); Array.new([0, n-length].max, x)+self end
	def ljust(n, x); dup.fill(x, length...n) end
	def pad(n, x = nil)
		n < 0 ? ljust(size - n, x) : rjust(size + n, x)
	end
end

class Alignment
	def initialize(a, b)
		@a = a
		@b = b
		
		@indices = {}
		@cost = {}
		@errors = {}
		
		calculate_peaks
	end
	
	attr :cost
	attr :indices
	attr :peaks
	
	# Calculates the range of possible offsets for overlapping a, b.
	def offsets
		# This ensures that at least half the items are overlapping, otherwise our confidence is too small:
		(-@b.length / 2 + 1)...(@a.length / 2)
	end
	
	def update_cost(offset)
		i = @indices[offset] ||= 0
		j = @peaks[i]
		k = offset + j
		
		# Increment as we've now included this cost:
		@indices[offset] += 1
		
		# The array wasn't overlapping at this index, cost = 0
		return if k < 0 or k >= @b.length
		
		# ...otherwise calculate error:
		error = (@a[j] - @b[k]) ** 2
		
		#puts "error = #{error} @ offset #{offset}"
		@errors[offset] ||= []
		@errors[offset] << error
		
		@cost[offset] ||= 0
		@cost[offset] += error
		
		# How many times can we evaluate this offset? = how much is overlapping?
	end
	
	def calculate_peaks
		# Find peaks in reverse order, O(NlogN)
		ordered_peaks = @a.each.with_index.to_a.sort{|a,b| b[0] <=> a[0]}
		
		# We only want the indices:
		@peaks ||= ordered_peaks.collect{|i| i[1]}
	end
	
	def completed? offset
		i = @indices[offset] ||= 0
		
		puts "#{i} >= #{@peaks.size}"
		return i >= @peaks.size
	end
	
	def calculate_overlap(estimate_index)
		# A priority queue for finding the next best node, lowest cost:
		lowest_cost_queue = Containers::PriorityQueue.new { |x, y| (y <=> x) == 1 }
		
		lowest_cost_queue.push(estimate_index, 0)
		
		while lowest_cost_queue.size > 0
			index = lowest_cost_queue.pop
			
			# If the lowest cost has already evaluated the entire possible array, we are done:
			if completed? index
				return index
			end
			
			debug_costs(index)
			
			unless @cost.key?(index - 1) or not offsets.include? (index - 1)
				lowest_cost_queue.push(index - 1, 0)
			end
			
			unless @cost.key? (index + 1) or not offsets.include? (index + 1)
				lowest_cost_queue.push(index + 1, 0)
			end
			
			update_cost(index)
			
			# Insert the item back with new cost:
			lowest_cost_queue.push(index, @cost[index])
			
			gets
		end
	end
	
	def debug_costs(offset)
		rows = []
		p = [nil] * @a.size
		p[@peaks[@indices[offset] || 0]] = '*'
		
		if offset < 0
			rows << p.pad(offset)
			rows << @a.pad(offset)
			rows << @b.pad(-offset)
		else
			rows << p.pad(-offset)
			rows << @a.pad(-offset)
			rows << @b.pad(offset)
		end
		
		table = Terminal::Table.new :rows => rows
		puts table
		
		puts "   cost: #{@cost.inspect}"
		puts "indices: #{@indices.inspect}"
		offsets.each do |offset|
			puts "#{offset.to_s.rjust(4)} : #{(@errors[offset] || []).join(', ')}"
		end
	end
end

# Visually aligned t1, t2, e = -3:
t1 = [0, 0, 0, 5, 0, 0, 9, 0, 0, 6, 4]
t2 =          [4, 0, 0, 8, 0, 0, 6, 4, 0, 0, 0]

# Estimate offset:
e = -2


alignment = Alignment.new(t1, t2)
offset = alignment.calculate_overlap(e)
puts "Found best match at offset #{offset}"

