//
//  FeatureTable.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 23/04/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "FeatureTable.h"
#include "FeaturePoints.h"

namespace TransformFlow {

	using namespace Dream::Events::Logging;

	FeatureTable::FeatureTable(std::size_t bins, const AlignedBox2 & bounds, const Radians<> & rotation) : _bounds(ZERO)
	{
		_rotation = rotate<Z>(rotation);

		_bins.resize(bins);

		// Calculate a new rotated bounding box:
		_bounds.union_with_point(_rotation * bounds.min());
		_bounds.union_with_point(_rotation * bounds.max());
		_bounds.union_with_point(_rotation * bounds.corner({false, true}));
		_bounds.union_with_point(_rotation * bounds.corner({true, false}));

		_size = _bounds.size();
	}

	void FeatureTable::print_table(std::ostream & output)
	{
		std::size_t index = 0;

		for (auto & bin : _bins) {
			output << "Bin " << index << ": ";

			for (auto & chain : bin.links) {
				output << chain.offset << "; ";
			}

			output << std::endl;

			index += 1;
		}
	}

	FeatureTable::Chain * FeatureTable::find_previous_similar(Vec2 aligned_offset, std::size_t index)
	{
		const Vec2 MAX_DISPLACEMENT = {4, 25};

		Chain * best_chain = nullptr;
		Vec2 best_displacement;

		// We scan one bin to the left, and one bin to the right, and find the closest chain.
		auto begin = _bins.begin() + index;
		auto end = _bins.begin() + (index + 1);

		if (begin != _bins.begin()) {
			--begin;
			//log_debug("Looking to left, size =", begin->links->size());
		}

		if (end != _bins.end()) {
			++end;
			//log_debug("Looking to right, size =", (end - 1)->links->size());
		}

		//log_debug("find_previous_similar", offset, " @ ", index);

		for (auto bin = begin; bin != end; ++bin) {
			if (bin->links.size() == 0) continue;

			Chain * previous_chain = &bin->links.back();
			Vec2 displacement = (aligned_offset - previous_chain->aligned_offset).absolute();

			//DREAM_ASSERT(previous_chain->offset[Y] < offset[Y]);

			//log_debug("*", previous_chain->offset, "displacement", displacement);

			// Displacement constants
			if (displacement[X] > MAX_DISPLACEMENT[X] || displacement[Y] > MAX_DISPLACEMENT[Y]) continue;

			if (best_chain == nullptr || displacement.length() < best_displacement.length()) {
				best_chain = previous_chain;
				best_displacement = displacement;
			}
		}

		//if (best_chain)
		//	log_debug("->", best_chain->offset, "displacement", best_displacement);

		return best_chain;
	}

	void FeatureTable::update(const std::vector<Vec2> & offsets)
	{
		for (auto offset : offsets) {
			auto aligned_offset = (_rotation * offset) - _bounds.min();

			// We are just concerned with horizontal offset:
			auto f = aligned_offset[X] / _size[X];
			DREAM_ASSERT(f >= 0 && f < 1);
			
			std::size_t index = f * _bins.size();

			auto & bin = _bins.at(index);

			Chain * previous_link = find_previous_similar(aligned_offset, index);

			// Add the chain link into the correct bin:
			bin.links.push_back({aligned_offset, offset, nullptr});

			// Add the chain into the list of all chains if it is a new chain:
			if (previous_link != nullptr) {
				previous_link->next = &bin.links.back();
			} else {
				_chains.push_back(&bin.links.back());
			}

			//print_table(std::cout);
		}

		//std::size_t i = 0;
		//for (auto bin : _bins) {
		//	std::cerr << "Bin " << i++ << ": " << bin.links.size() << std::endl;
		//}

		//std::cerr << "Found " << _chains.size() << " chains." << std::endl;
	}

	/*
		Scan through middle of image.
		
		find gradients
		
		based on r radius, scan at approximate gradient edge, and chain together.
		
		build chains of edges.
		
		almagamte into one data structure, chain, with approximate centre, size, and colours. Primarily left or right edge, e.g. tree trunk against white building, trunk is the main part of the edge.
		
		track from one frame to the next by scanning algorithm, adjust size based on edges, perhaps use binary search.
	*/

	struct AlignmentCost
	{
		int offset;
		float error;

		bool operator<(const AlignmentCost & other) const
		{
			return error < other.error;
		}
	};

	template <typename SequenceT>
	AlignmentCost calculate_alignment_cost(const SequenceT & a, const SequenceT & b, int offset)
	{
		AlignmentCost cost = {offset, 0};

		std::size_t i = 0, j = 0;

		if (offset > 0)
			i = offset;
		else
			j = -offset;

		while (i < a.size() && j < b.size())
		{
			auto d = number(int(a[i].links.size()) - int(b[j].links.size())).absolute();

			cost.error += (d*d);

			i += 1, j += 1;
		}

		return cost;
	}

	std::size_t FeatureTable::calculate_alignment(const FeatureTable & other)
	{
		// We try to find the alignment between two sequences:
		std::priority_queue<AlignmentCost> ordered_costs;
		std::vector<AlignmentCost> costs;

		auto & a = _bins;
		auto & b = other.bins();

		// The size of the search, left or right of the origin:
		const int F = 8;

		int min = -(int(b.size()) / F);
		int max = int(b.size()) / F;

		for (int offset = min; offset < max; offset += 1)
		{
			auto cost = calculate_alignment_cost(a, b, offset);

			log_debug("-- Calculated error at offset:", offset, "=", cost.error);

			ordered_costs.push(cost);
		}

		return ordered_costs.top().offset;
	}
};
