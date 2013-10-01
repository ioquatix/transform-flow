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
	
	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * MOTION = "Motion";
	const char * LOCATION = "Location";
	const char * HEADING = "Heading";
	const char * FRAME = "Frame";

	SensorData::SensorData(Ptr<ILoader> loader) : _loader(loader)
	{
		parse_log();
	}

	SensorData::~SensorData() noexcept
	{
		
	}
	
	Ref<Image> SensorData::frame_for_index(std::size_t index) {
		if (index < _frames.size()) {
			if (_frames[index]) {
				return _frames[index];
			}
		} else {
			_frames.resize(index+1);
		}
				
		return (_frames[index] = _loader->load<Image>(to_string(index)));
	}
	
	void SensorData::parse_log()
	{
		Ref<IData> data = _loader->data_for_resource("log");
		Shared<std::istream> stream = data->input_stream();
		
		std::string line;

		MotionUpdate motion_update;

		while (stream->good())
		{
			std::getline(*stream, line, '\n');
			
			std::vector<std::string> parts;
			split(line, ',', std::back_inserter(parts));
			
			for (auto & part : parts)
				part = trimmed(part, " ");
			
			if (parts.size() < 2)
				continue;

			if (parts.at(1) == GYROSCOPE)
			{
				motion_update.rotation_rate = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			}
			else if (parts.at(1) == ACCELEROMETER)
			{
				motion_update.acceleration = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			}
			else if (parts.at(1) == GRAVITY)
			{
				motion_update.gravity = Vec3(to<RealT>(parts.at(3)), to<RealT>(parts.at(4)), to<RealT>(parts.at(5)));
			}
			else if (parts.at(1) == MOTION)
			{
				motion_update.time_offset = to<TimeT>(parts.at(2));

				_sensor_updates.push_back(new MotionUpdate(motion_update));
			}
			else if (parts.at(1) == LOCATION)
			{
				LocationUpdate location_update;

				location_update.time_offset = to<TimeT>(parts.at(2));
				location_update.latitude = to<double>(parts.at(3));
				location_update.longitude = to<double>(parts.at(4));
				location_update.altitude = to<double>(parts.at(5));
				
				location_update.horizontal_accuracy = to<double>(parts.at(6));
				location_update.vertical_accuracy = to<double>(parts.at(7));

				_sensor_updates.push_back(new LocationUpdate(location_update));
			}
			else if (parts.at(1) == HEADING)
			{
				HeadingUpdate heading_update;
				
				heading_update.time_offset = to<TimeT>(parts.at(2));
				heading_update.magnetic_bearing = to<double>(parts.at(3));
				heading_update.true_bearing = to<double>(parts.at(4));

				_sensor_updates.push_back(new HeadingUpdate(heading_update));
			}
			else if (parts.at(1) == FRAME)
			{
				ImageUpdate image_update;
				
				image_update.time_offset = to<RealT>(parts.at(2));
				image_update.image_buffer = frame_for_index(to<std::size_t>(parts.at(3)));

				// http://www.boinx.com/chronicles/2013/3/22/field-of-view-fov-of-cameras-in-ios-devices/
				if (parts.size() >= 5)
					image_update.field_of_view = radians(to<RealT>(parts.at(4)));
				else
					image_update.field_of_view = 55.0_deg; // Typical mobile device FOV.

				_sensor_updates.push_back(new ImageUpdate(image_update));
			}
			else
			{
				//logger()->log(LOG_WARN, LogBuffer() << "Unexpected line: " << line);
			}
		}
	}

	void VideoStream::VideoFrame::calculate_feature_points()
	{
		Dream::Core::Stopwatch watch;
		watch.start();

		feature_points = new FeaturePoints;
		feature_points->scan(image_update->image_buffer, tilt);

		watch.pause();
	}

	VideoStream::VideoStream(Ptr<ILoader> loader, Ref<MotionModel> motion_model) : _loader(loader), _motion_model(motion_model)
	{
		load_frames();
		load_tracking_points();
	}
	
	void VideoStream::load_frames()
	{
		_sensor_data = new SensorData(_loader);
		
		// This frame index is relating to the actual index of the frame data.
		std::size_t frame_index = 0;
		
		for (auto & update : _sensor_data->sensor_updates()) {
			// We process all updates in order, to calculate the information at specific video frames:
			_motion_model->update(update.get());

			if (Shared<ImageUpdate> image_update = update) {
				VideoFrame video_frame;

				video_frame.index = frame_index;
				video_frame.image_update = image_update;
				video_frame.valid = _motion_model->localization_valid();

				if (video_frame.valid) {
					video_frame.gravity = _motion_model->gravity().normalize();
					video_frame.bearing = _motion_model->bearing();
					video_frame.tilt = _motion_model->tilt();

					// Global coordinate system:
					Vec3 down(0, -1, 0), north(0, 0, -1);
				
					Vec3 heading = Quat(rotate(video_frame.bearing, down)) * north;
					video_frame.heading = heading.normalize();

					video_frame.calculate_feature_points();
				}

				_frames.push_back(video_frame);
				frame_index += 1;
			}
		}
	}
	
	static void parse_rows(std::istream & input, std::function<void(const std::vector<std::string> &)> row_callback)
	{
		std::string line;

		while (input.good())
		{
			std::getline(input, line, '\n');
			
			std::vector<std::string> parts;
			split(line, ',', std::back_inserter(parts));
			
			for (auto & part : parts)
				part = trimmed(part, " ");
			
			row_callback(parts);
		}
	}
	
	void VideoStream::load_tracking_points()
	{
		Ref<IData> data = _loader->data_for_resource("tracking-points");
		
		if (!data) {
			log_debug("Could not find tracking points data!");
			
			return;
		}
		
		Shared<std::istream> stream = data->input_stream();
		
		//image_frame,tracking_index,x,y[,z]
		parse_rows(*stream, [&](const std::vector<std::string> & parts) {
			if (parts.size() < 4) return;
			
			TrackingPoint tracking_point;
			
			tracking_point.frame_index = to<std::size_t>(parts.at(0));
			tracking_point.tracking_index = to<std::size_t>(parts.at(1));
			
			tracking_point.coordinate[X] = to<RealT>(parts.at(2));
			tracking_point.coordinate[Y] = to<RealT>(parts.at(3));
			
			if (parts.size() >= 5)
				tracking_point.coordinate[Z] = to<RealT>(parts.at(4));
			else
				tracking_point.coordinate[Z] = 0;
			
			_tracking_points.push_back(tracking_point);
			
			// Per video frame tracking points:
			_frames.at(tracking_point.frame_index).tracking_points[tracking_point.tracking_index] = tracking_point;
		});
		
		log_debug("Loaded", _tracking_points.size(), "tracking points.");
	}
	
	VideoStream::~VideoStream() noexcept
	{
	}
}
