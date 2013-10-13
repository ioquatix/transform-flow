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

#include <list>

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
		struct History
		{
			Ref<FeaturePoints> feature_points;
			Radians<> relative_rotation;
			RealT corrected_bearing;
			
			RealT calculate_estimate(const ImageUpdate & image_update, Radians<> current_relative_bearing) const;
			Average<RealT> calculate_offset(Ptr<FeaturePoints> current_feature_points, const RealT & estimate) const;
			RealT calculate_bearing(const ImageUpdate & image_update, const Average<RealT> & offset) const;
		};
		
		History _history;
		
		bool _corrected_bearing_primed;
		
		RealT _corrected_bearing;
		
		// Measured in degrees from north:
		RealT _previous_bearing;
	};
}

#endif
