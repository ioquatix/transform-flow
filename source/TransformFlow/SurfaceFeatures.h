//
//  SurfaceFeatures.h
//  Transform Flow
//
//  Created by Samuel Williams on 13/08/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef __TRANSFORM_FLOW_SURFACE_FEATURES_H
#define __TRANSFORM_FLOW_SURFACE_FEATURES_H

#include "FeaturePoints.h"

namespace TransformFlow {

	std::vector<Vec2> find_key_points(Ptr<Image> pixel_buffer);

}

#endif
