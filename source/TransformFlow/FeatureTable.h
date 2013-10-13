//
//  FeatureTable.h
//  Transform Flow
//
//  Created by Samuel Williams on 23/04/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#ifndef __Transform_Flow__FeatureTable__
#define __Transform_Flow__FeatureTable__

#include <Dream/Class.h>

#include <Euclid/Geometry/AlignedBox.h>
#include <Euclid/Numerics/Average.h>

#include <vector>

namespace TransformFlow
{
	using namespace Dream;
	using namespace Euclid::Geometry;

	class FeatureTable : public Object
	{
	public:
		struct Feature {
			// Aligned offset is the offset in world coordinates. The offset itself is in pixel coordinates.
			Vec2 aligned_offset, offset;
		};

		struct Bin {
			std::vector<Feature> features;
		};

		struct Index {
			std::size_t bin_index;
			std::size_t feature_index;
		};

	protected:
		Mat33 _transform;
		AlignedBox2 _bounds;
		const RealT _pixels_per_bin, _dy;

		std::vector<Bin> _bins;

		Index add_feature(const Vec2 & offset);

	public:
		FeatureTable(RealT dy, RealT pixels_per_bin, const AlignedBox2 & bounds, const Radians<> & rotation);
		virtual ~FeatureTable();

		RealT dy() const { return _dy; };
		RealT pixels_per_bin() const { return _pixels_per_bin; }

		const AlignedBox2 & bounds() const { return _bounds; }

		virtual void update(const std::vector<Vec2> & offsets);

		std::size_t bin_index_for_offset(RealT x);

		const std::vector<Bin> & bins() const { return _bins; }

		Average<RealT> average_feature_position(std::size_t bin) const;

		Average<RealT> bin_alignment_sequential(const FeatureTable & other, std::size_t i, std::size_t j) const;
		Average<RealT> bin_alignment_average(const FeatureTable & other, std::size_t i, std::size_t j) const;

		// The estimate is in pixels, the default is usually sufficient.
		Average<RealT> calculate_offset(const FeatureTable & other, int estimate = 0) const;
	};
}

#endif /* defined(__Transform_Flow__FeatureTable__) */
