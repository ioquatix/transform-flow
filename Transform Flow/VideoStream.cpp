//
//  VideoStream.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 8/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "VideoStream.h"
#include <Dream/Core/Data.h>

#include <Euclid/Numerics/Interpolate.h>
#include <Euclid/Numerics/Transforms.h>

namespace TransformFlow {
	using namespace Dream::Events::Logging;

	void VideoStream::VideoFrame::calculate_feature_points()
	{
		Dream::Core::Stopwatch watch;
		watch.start();

		feature_points = new FeaturePoints;
		//feature_points->scan(image_update->image_buffer);
		feature_points->scan(image_update->image_buffer, tilt);

		watch.pause();
		log_debug("Feature Point Scan:", watch.time());
	}

	VideoStream::VideoStream(const Path & path, Ref<MotionModel> motion_model) : _sensor_data(new SensorData(path)), _motion_model(motion_model)
	{
		for (auto & update : _sensor_data->sensor_updates()) {
			// We process all updates in order, to calculate the information at specific video frames:
			_motion_model->update(update.get());

			if (Shared<ImageUpdate> image_update = update) {
				VideoFrame video_frame;

				video_frame.image_update = image_update;
				video_frame.gravity = _motion_model->gravity();
				video_frame.bearing = _motion_model->bearing();
				video_frame.tilt = R90;

				video_frame.calculate_feature_points();

				log_debug("Video Frame", "Gravity", video_frame.gravity, "Bearing", video_frame.bearing);

				_images.push_back(video_frame);
			}
		}
	}
	
	VideoStream::~VideoStream()
	{
	}
}
