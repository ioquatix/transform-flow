//
//  HybridMotionModel.h
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 12/8/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#ifndef TRANSFORMFLOW_HYBRIDMOTIONMODEL_H
#define TRANSFORMFLOW_HYBRIDMOTIONMODEL_H

#include "BasicSensorMotionModel.h"
#include "FeaturePoints.h"

namespace TransformFlow
{
	class HybridMotionModel : public BasicSensorMotionModel
	{
	public:
		HybridMotionModel();
		virtual ~HybridMotionModel();

		virtual void update(const ImageUpdate & image_update);

		virtual Radians<> bearing() const;

	protected:
		bool _corrected_bearing_primed;
		
		RealT _corrected_bearing;
		
		// Measured in degrees from north:
		RealT _previous_bearing;
		
		Radians<> _previous_relative_rotation;
		
		Ref<FeaturePoints> _previous_feature_points;
	};
}

#endif
