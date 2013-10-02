//
//  Alignment.cpp
//  IntegerArrayAlignment
//
//  Created by Samuel Williams on 20/09/13.
//  Copyright (c) 2013 Orion Transfer. All rights reserved.
//

#include "FastAlignment.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <queue>
#include <random>
#include <cassert>
#include <limits>

#include <Dream/Events/Logger.h>

namespace TransformFlow
{
	using namespace Dream::Events::Logging;

	struct Cost
	{
		int offset;
		float error;//, actual_error;

		std::size_t count;

		Cost(int _offset, float _error = 0) : offset(_offset), error(_error), count(0) {}

		bool operator>(const Cost & other) const
		{
			return error > other.error;
		}

		bool operator<(const Cost & other) const
		{
			return error < other.error;
		}

		bool operator==(const Cost & other) const
		{
			return this->offset == other.offset && this->error == other.error;
		}

		bool operator!=(const Cost & other) const
		{
			return !((*this) == other);
		}

		void add_error(float amount)
		{
			error += amount;
			//actual_error += amount;
		}
	};

	static float error_bias(std::size_t difference)
	{
		// std::powf(offset - estimate, bias);
		return (difference * difference) / 2.0;
	}

	template <typename SequenceT>
	static Cost calculate_alignment_cost(const SequenceT & u, const SequenceT & v, int offset, int estimate, float best_error)
	{
		Cost cost = {offset};

		std::size_t i = 0, j = 0;

		if (offset < 0)
			j = -offset;
		else
			i = offset;

		cost.error += error_bias(offset - estimate);

		while (i < u.size() && j < v.size())
		{
			auto d = float(u[i]) - float(v[j]);

			cost.add_error(d*d);

			i += 1, j += 1;

			cost.count += 1;

			if (cost.error > best_error) break;
		}

		return cost;
	}

	template <typename SequenceT>
	Cost align_small(const SequenceT & u, const SequenceT & v, int estimate)
	{
		Cost minimum_cost = calculate_alignment_cost(u, v, estimate, estimate, std::numeric_limits<float>::max());

		// Used to control expansion of the search space:
		int left = estimate - 1;
		int right = estimate + 1;

		// The bounds of the offset search:
		int right_bound = (int)u.size() / 2;
		int left_bound = -right_bound;

		while (left > left_bound || right < right_bound) {
			if (left > left_bound) {
				Cost cost = calculate_alignment_cost(u, v, left, estimate, minimum_cost.error);

				if (cost.error <= minimum_cost.error)
					minimum_cost = cost;

				left -= 1;
			}

			if (right < right_bound) {
				Cost cost = calculate_alignment_cost(u, v, right, estimate, minimum_cost.error);

				if (cost.error <= minimum_cost.error)
					minimum_cost = cost;

				right += 1;
			}
		}

		return minimum_cost;
	}

	// Given a list, return the permutation which if applied would sort the list.
	template <class SequenceT, typename CompareT = std::less<typename SequenceT::value_type>>
	std::vector<std::size_t> permutation(const SequenceT & values, const CompareT & compare = CompareT()) {
		std::vector<std::size_t> indices;
		indices.reserve(values.size());

		for (std::size_t i = 0; i < values.size(); i += 1)
			indices.push_back(i);

		std::sort(std::begin(indices), std::end(indices), [&](std::size_t a, std::size_t b) -> bool {
			return compare(values[a], values[b]);
		});

		return indices;
	}

	template <typename HeapT, typename CompareT = std::less<typename HeapT::value_type>>
	void siftdown(HeapT & heap, const std::size_t & top_index, const CompareT & compare = CompareT())
	{
		std::size_t parent_index = top_index;
		std::size_t left_child_index = 2*parent_index + 1;

		auto item = heap.at(parent_index);

		while (left_child_index < heap.size()) {
			auto smallest_child_index = left_child_index;

			auto right_child_index = left_child_index + 1;
			if (right_child_index < heap.size()) {
				// right_child < left_child
				if (compare(heap.at(right_child_index), heap.at(left_child_index))) {
					smallest_child_index = right_child_index;
				}
			}

			// smallest_child < item
			if (compare(heap.at(smallest_child_index), item)) {
				heap.at(parent_index) = heap.at(smallest_child_index);
				parent_index = smallest_child_index;
				left_child_index = 2*parent_index + 1;
			} else {
				break;
			}
		}

		if (parent_index != top_index) {
			heap.at(parent_index) = item;
		}
	}

	template <typename HeapT, typename CompareT = std::less<typename HeapT::value_type>>
	void siftup(HeapT & heap, const std::size_t & bottom_index, const CompareT & compare = CompareT())
	{
		std::size_t child_index = bottom_index;
		std::size_t parent_index = (child_index - 1) / 2;

		auto item = heap.at(child_index);

		while (child_index > 0) {
			// child < parent
			if (compare(item, heap.at(parent_index))) {
				heap.at(child_index) = heap.at(parent_index);

				child_index = parent_index;
				parent_index = (child_index - 1) / 2;
			} else {
				break;
			}
		}

		if (child_index != bottom_index)
			heap.at(child_index) = item;
	}

	template <typename SequenceT>
	Cost align_large(const SequenceT & u, const SequenceT & v, int estimate)
	{
		auto peaks = permutation(u, std::greater<std::size_t>());

		std::vector<Cost> costs;

		// Used to control expansion of the search space:
		int left = estimate - 1;
		int right = estimate + 1;

		// The bounds of the offset search:
		int right_bound = int(u.size() / 4) * 3;
		int left_bound = -right_bound;

		// Avoid reallocation:
		costs.reserve(std::max(u.size(), v.size()) + std::min(u.size(), v.size()) * 2);
		costs.push_back({estimate});

		while (costs.size() > 0) {
			auto & best = costs.front();

			if (best.count == 0) {
				// Add adjacent costs if required.
				if ((left == best.offset-1) && (left > left_bound)) {
					costs.push_back({left, error_bias(left - estimate)});
					siftup(costs, costs.size() - 1);
					left -= 1;
				}

				if ((right == best.offset+1) && (right < right_bound)) {
					costs.push_back({right, error_bias(right - estimate)});
					siftup(costs, costs.size() - 1);
					right += 1;
				}
			} else {
				// If we have evaluated all items and this is still the lowest cost, we are done:
				if (best.count >= u.size())
					return best;
			}

			auto i = best.count;
			auto j = peaks.at(i);

			auto k = int(j) - best.offset;

			best.count += 1;

			if (k >= 0 && k < v.size()) {
				auto d = float(u[j]) - float(v[k]);

				if (d != 0) {
					best.add_error(d*d);
					siftdown(costs, 0);
				}
			}
		}

		// Never get here:
		return {estimate};
	}

	typedef std::vector<std::size_t> UnsignedSequenceT;

	Average<RealT> align_tables(const FeatureTable & a, const FeatureTable & b, int estimate)
	{
		UnsignedSequenceT sa, sb;

		//std::cerr << "a.bins: ";
		for (auto & bin : a.bins()) {
		//	std::cerr << bin.links.size() << " ";
			sa.push_back(bin.links.size());
		}
		//std::cerr << std::endl;

		//std::cerr << "b.bins: ";
		for (auto & bin : b.bins()) {
		//	std::cerr << bin.links.size() << " ";
			sb.push_back(bin.links.size());
		}
		//std::cerr << std::endl;

		// The origin of sa, sb is in the middle. So, we need to bias the search to take this into account:
		int bias = (int(sa.size()) - int(sb.size())) / 2;

		// Most likely bin offset:
		Cost cost = align_small(sa, sb, bias + estimate);

		auto offset = cost.offset;

#ifdef DEBUG_ALIGNMENT
		if (bias != cost.offset) {
			log_debug("align", "sa", sa.size(), "sb", sb.size(), "estimate", estimate, "bias", bias, "cost offset", cost.offset, "error", cost.error);

			std::cerr << "a.bins: ";
			for (auto & bin : a.bins()) {
				std::cerr << bin.links.size() << " ";
			}
			std::cerr << std::endl;

			std::cerr << "b.bins: ";
			for (auto & bin : b.bins()) {
				std::cerr << bin.links.size() << " ";
			}
			std::cerr << std::endl;

			cost = align_small(sa, sb, bias + estimate);
		}
#endif

		Average<RealT> distribution;

		std::size_t i = 0, j = 0;

		if (offset < 0)
			j = -offset;
		else
			i = offset;

		while (i < a.bins().size() && j < b.bins().size())
		{
			auto a_distribution = a.average_chain_position(i);
			auto b_distribution = b.average_chain_position(j);

			// Increment counters:
			i += 1, j += 1;

			const std::size_t T = 0;

			// If we don't have enough confidence in our distribution of vertical feature points, we skip this bin.
			if (a_distribution.number_of_samples() <= T || b_distribution.number_of_samples() <= T)
				continue;

			auto difference = b_distribution.value() - a_distribution.value();
			//log_debug("A", a_distribution.value(), "#", a_distribution.number_of_samples(), "B", b_distribution.value(), "#", b_distribution.number_of_samples(), "Difference", difference);
			
			distribution.add_sample(difference);
		}

		//log_debug("Calculated offset", distribution.value(), "#", distribution.number_of_samples());

		return distribution;
	}
};
