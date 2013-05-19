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

#include <vector>
#include <list>

namespace TransformFlow
{
	using namespace Dream;

	class FeaturePoints;

	class FeatureTable : public Object
	{
	public:
		struct Chain {
			Vec2 offset;
			Chain * next;
		};

		struct Bin {
			std::list<Chain> links;
		};

	protected:
		Vec2u _size;

		std::vector<Chain *> _chains;
		std::vector<Bin> _bins;

		void print_table(std::ostream & output);

		Chain * find_previous_similar(Vec2 offset, std::size_t index);

	public:
		FeatureTable(std::size_t bins, Vec2u size);

		void update(FeaturePoints * feature_points);

		const std::vector<Chain *> & chains() { return _chains; }
	};
}

#endif /* defined(__Transform_Flow__FeatureTable__) */
