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
#include <Euclid/Geometry/Plane.h>

namespace TransformFlow {
	using namespace Dream::Events::Logging;

	void VideoStream::VideoFrame::calculate_feature_points()
	{
		Dream::Core::Stopwatch watch;
		watch.start();

		feature_points = new FeaturePoints;
		feature_points->scan(image_update->image_buffer, tilt);

		watch.pause();
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

				// This is in device-space/image-space.
				{
					Vec3 north{0, 0, -1};

					video_frame.heading = Quat(rotate<Z>(-video_frame.bearing)) * north;

					Plane3 flat_plane(Vec3{0}, video_frame.heading);
					Vec3 planar_gravity = flat_plane.closest_point(video_frame.gravity);

					video_frame.tilt = planar_gravity.angle_between({1, 0, 0});
					//log_debug("Calculated tilt:", video_frame.tilt.value * R2D);
				}

				video_frame.calculate_feature_points();

				//log_debug("Video Frame", "Gravity", video_frame.gravity, "Bearing", video_frame.bearing);

				_images.push_back(video_frame);
			}
		}
	}
	
	VideoStream::~VideoStream()
	{
	}
}
