//
//  FeaturePoint.h
//  Transform Flow
//
//  Created by Samuel Williams on 27/04/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#ifndef _TRANSFORM_FLOW_FEATURE_POINT_H
#define _TRANSFORM_FLOW_FEATURE_POINT_H

#include <Euclid/Numerics/Vector.h>

namespace TransformFlow
{
	using namespace Euclid::Numerics;

	struct FeaturePoint
	{
		Vec2u offset;
		
		// The colour identity along the gradient:
		Vec3 a, b;

		// The gradient of the feature point:
		Vec3 gradient;
		
		RealT difference(const FeaturePoint & other);
	};
}

#endif
