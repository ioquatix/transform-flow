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

#include <Dream/Resources/Loader.h>

namespace TransformFlow
{
	using Resources::ILoader;
	
	class SensorData : public Object
	{
		protected:
			Ref<ILoader> _loader;
			
			std::vector<Ref<Image>> _frames;
			Ref<Image> frame_for_index(std::size_t index);

			std::vector<Shared<SensorUpdate>> _sensor_updates;

			void parse_log();

		public:
			SensorData(Ptr<ILoader> loader);
			virtual ~SensorData() noexcept;

			const std::vector<Shared<SensorUpdate>> & sensor_updates() const { return _sensor_updates; }
	};
	
	class VideoStream : public Object
	{
		public:
			struct TrackingPoint
			{
				std::size_t frame_index;
				
				// Tracking points with the same tracking index in different frames are the same "physical location".
				std::size_t tracking_index;
				
				Vec3 coordinate;
			};
			
			struct VideoFrame
			{
				std::size_t index;

				Shared<ImageUpdate> image_update;

				// Whether the localization was valid for this frame:
				bool valid;

				// In device coordinate space:
				Vec3 gravity, heading;

				Radians<> bearing;
				Radians<> tilt;

				Ref<FeaturePoints> feature_points;

				void calculate_feature_points();
				
				std::unordered_map<std::size_t, TrackingPoint> tracking_points;
			};

		protected:
			Ref<ILoader> _loader;
			
			Ref<SensorData> _sensor_data;
			Ref<MotionModel> _motion_model;

			std::vector<VideoFrame> _frames;
			std::vector<TrackingPoint> _tracking_points;

			void load_frames();
			void load_tracking_points();

		public:
			VideoStream(Ptr<ILoader> loader, Ref<MotionModel> motion_model);
			virtual ~VideoStream() noexcept;

			const std::vector<VideoFrame> & frames() const { return _frames; }
			
			const std::vector<TrackingPoint> & tracking_points() const { return _tracking_points; }
	};
}

#endif
