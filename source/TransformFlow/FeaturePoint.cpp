//
//  FeaturePoint.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 27/04/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "FeaturePoint.h"

namespace TransformFlow
{
	float FeaturePoint::similarity(const FeaturePoint & other)
	{
		Vec3 da = (a - other.a);
		Vec3 db = (b - other.b);
		
		return da.product() * db.product();
	}
}