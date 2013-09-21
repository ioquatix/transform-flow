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
#include "FeaturePoint.h"

#include <Euclid/Geometry/AlignedBox.h>
#include <Euclid/Numerics/Average.h>

#include <vector>
#include <list>

namespace TransformFlow
{
	using namespace Dream;
	using namespace Euclid::Geometry;

	class FeaturePoints;

	class FeatureTable : public Object
	{
	public:
		struct Chain {
			// Aligned offset is the offset in world coordinates. The offset itself is in pixel coordinates.
			Vec2 aligned_offset, offset;
			Chain * next;
		};

		struct Bin {
			std::list<Chain> links;
		};

	protected:
		Mat33 _transform;
		AlignedBox2 _bounds;

		std::vector<Chain *> _chains;
		std::vector<Bin> _bins;

		void print_table(std::ostream & output);

		Chain * find_previous_similar(Vec2 offset, std::size_t index);

	public:
		FeatureTable(std::size_t bins, const AlignedBox2 & bounds, const Radians<> & rotation);

		void update(const std::vector<Vec2> & offsets);

		const std::vector<Chain *> & chains() const { return _chains; }
		const std::vector<Bin> bins() const { return _bins; }

		int calculate_bin_offset(const FeatureTable & other) const;

		Average<RealT> average_chain_position(std::size_t bin) const;
		Average<RealT> calculate_offset(const FeatureTable & other) const;
	};
}

#endif /* defined(__Transform_Flow__FeatureTable__) */
