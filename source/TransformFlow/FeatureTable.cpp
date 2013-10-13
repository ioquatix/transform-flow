//
//  FeatureTable.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 23/04/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "FeatureTable.h"
#include "FeaturePoints.h"
#include "FastAlignment.h"

#include <Euclid/Numerics/Average.h>
#include <Euclid/Numerics/Interpolate.h>

namespace TransformFlow {

	using namespace Dream::Events::Logging;

	FeatureTable::FeatureTable(RealT dy, RealT pixels_per_bin, const AlignedBox2 & bounds, const Radians<> & rotation) : _dy(dy), _bounds(ZERO), _pixels_per_bin(pixels_per_bin)
	{
		// The bounds provided are the bounds of the image. Points are image coordinates, but this isn't suitable for binning. We want to bin along the axis perpendicular to gravity, so we make a rotation which does this. We also want to center the table at the origin, so we apply a translation.
		_transform = rotate<Z>(rotation) << translate(-bounds.size() / 2.0);

		// Calculate a new rotated bounding box:
		_bounds.union_with_point(_transform * bounds.min());
		_bounds.union_with_point(_transform * bounds.max());
		_bounds.union_with_point(_transform * bounds.corner({false, true}));
		_bounds.union_with_point(_transform * bounds.corner({true, false}));

		auto half_width = _bounds.size()[WIDTH] / 2.0;
		auto bins_per_half_width = (half_width / _pixels_per_bin) + 1;

		std::size_t number_of_bins = bins_per_half_width * 2;

		// Allocate the bins:
		_bins.resize(number_of_bins);
	}

	FeatureTable::~FeatureTable()
	{
	}

	std::size_t FeatureTable::bin_index_for_offset(RealT x)
	{
		// Adventure Time:
		auto b = x / _pixels_per_bin;
		auto m = _bins.size() / 2;
		auto o = b + m;

		if (o == _bins.size())
			o -= 1;

		assert(o >= 0 && o < _bins.size());

		return o;
	}

	FeatureTable::Index FeatureTable::add_feature(const Vec2 &offset)
	{
		// The aligned offset is the coordinate relative to the origin, with -Z = gravity.
		auto aligned_offset = _transform * offset;

		auto index = bin_index_for_offset(aligned_offset[X]);
		auto & bin = _bins.at(index);

		// Add the chain link into the correct bin:
		bin.features.push_back({aligned_offset, offset});

		return {index, bin.features.size() - 1};
	}

	void FeatureTable::update(const std::vector<Vec2> &offsets)
	{
		for (auto & offset : offsets)
			add_feature(offset);
	}

	Average<RealT> FeatureTable::average_feature_position(std::size_t bin) const
	{
		Average<RealT> distribution;
		
		for (auto & feature : _bins[bin].features)
		{
			//log_debug("Adding chain, aligned_offset =", chain.aligned_offset, "offset =", chain.offset);
			distribution.add_sample(feature.aligned_offset[X]);
		}

		return distribution;
	}

	Average<RealT> FeatureTable::bin_alignment_sequential(const FeatureTable & other, std::size_t i, std::size_t j) const
	{
		std::size_t m = 0, n = 0;

		auto & a = _bins[i].features;
		auto & b = other.bins()[j].features;

		Average<RealT> alignment;

		const RealT dy = 0.9 * _dy;

		while (m < a.size() && n < b.size()) {
			auto & pa = a[m].aligned_offset;
			auto & pb = b[n].aligned_offset;

			auto d = pb - pa;

			if (d[Y] <= -dy) {
				n += 1;
				continue;
			}

			if (d[Y] >= dy) {
				m += 1;
				continue;
			}

			alignment.add_sample(d[X]);

			m += 1; n += 1;
		}

		return alignment;
	}

	Average<RealT> FeatureTable::bin_alignment_average(const FeatureTable & other, std::size_t i, std::size_t j) const
	{
		auto pa = average_feature_position(i);
		auto pb = other.average_feature_position(j);
		
		Average<RealT> average;
		
		auto t = _pixels_per_bin;
		
		if (pa.number_of_samples() > t && pb.number_of_samples() > t)
			average.add_sample(pb.value() - pa.value());
		
		return average;
	}

	Average<RealT> FeatureTable::calculate_offset(const FeatureTable & other, int estimate) const
	{
		// No bins -> no data, cannot align.
		if (_bins.size() == 0 || other.bins().size() == 0)
			return {};

		assert(this->_pixels_per_bin == other._pixels_per_bin);

		// Convert pixel estimate to bin estimate:
		if (estimate != 0) {
			estimate /= _pixels_per_bin;
		}

		return align_tables(*this, other, estimate);
	}
};
