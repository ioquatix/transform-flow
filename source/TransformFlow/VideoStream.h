//
//  VideoStream.h
//  Transform Flow
//
//  Created by Samuel Williams on 8/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef _TRANSFORM_FLOW_VIDEO_STREAM_H
#define _TRANSFORM_FLOW_VIDEO_STREAM_H

#include "MotionModel.h"
#include "FeaturePoints.h"

namespace TransformFlow
{
	class VideoStream : public Object
	{
		public:
			struct VideoFrame
			{
				Shared<ImageUpdate> image_update;

				// In device coordinate space:
				Vec3 gravity, heading;

				Radians<> bearing;
				Radians<> tilt;

				Ref<FeaturePoints> feature_points;

				void calculate_feature_points();
			};

		protected:
			Ref<SensorData> _sensor_data;
			Ref<MotionModel> _motion_model;

			std::vector<VideoFrame> _frames;

		public:
			VideoStream(const Path & path, Ref<MotionModel> motion_model);
			virtual ~VideoStream() noexcept;

			const std::vector<VideoFrame> & frames() const { return _frames; }
	};
}

#endif
