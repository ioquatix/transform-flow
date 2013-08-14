//
//  OpticalFlowMotionModel.h
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 12/8/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#ifndef TRANSFORMFLOW_OPTICALFLOWMOTIONMODEL_H
#define TRANSFORMFLOW_OPTICALFLOWMOTIONMODEL_H

#include "BasicSensorMotionModel.h"
#include "FeatureAlgorithm.h"

namespace TransformFlow
{
	std::vector<Vec2> find_key_points(Ptr<Image> pixel_buffer);

	class OpticalFlowMotionModel : public BasicSensorMotionModel
	{
	public:
		OpticalFlowMotionModel();
		virtual ~OpticalFlowMotionModel();

		virtual void update(const ImageUpdate & image_update);

	private:
		bool _image_primed;
		ImageUpdate _image_update;

		Ref<MatchingAlgorithm> _matching_algorithm;
	};
}

#endif
