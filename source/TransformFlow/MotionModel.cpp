//
//  MotionModel.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 16/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "MotionModel.h"

namespace TransformFlow
{
	using namespace Dream::Events::Logging;

	SensorUpdate::~SensorUpdate()
	{
	}

	LocationUpdate::~LocationUpdate()
	{
	}

	void LocationUpdate::apply(MotionModel * model)
	{
		model->update(*this);
	}

	HeadingUpdate::~HeadingUpdate()
	{
	}

	void HeadingUpdate::apply(MotionModel * model)
	{
		model->update(*this);
	}

	MotionUpdate::~MotionUpdate()
	{
	}

	void MotionUpdate::apply(MotionModel * model)
	{
		model->update(*this);
	}

	ImageUpdate::~ImageUpdate()
	{
	}

	void ImageUpdate::apply(MotionModel * model)
	{
		model->update(*this);
	}

	RealT ImageUpdate::distance_from_origin(RealT width) const
	{
		// opposite = width, adjacent = distance_from_origin
		// tan(angle) = opposite / adjacent
		return (width / 2.0) / (field_of_view / 2.0).tan();
	}

	RealT ImageUpdate::distance_from_origin()
	{
		return distance_from_origin(image_buffer->size()[WIDTH]);
	}

	Radians<> ImageUpdate::angle_of(RealT pixels) const
	{
		return (field_of_view / image_buffer->size()[WIDTH]) * pixels;
	}

#pragma mark -

	using namespace Dream::Events::Logging;
	
	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * MOTION = "Motion";
	const char * LOCATION = "Location";
	const char * HEADING = "Heading";
	const char * FRAME = "Frame Captured";

	SensorData::SensorData(const Path & path) : _loader(new Resources::Loader(path)) {
		_loader->add_loader(new Image::Loader);
		
		parse_log();
	}
	
	SensorData::~SensorData() {
		
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
		Ref<IData> data = _loader->data_for_resource("log.txt");
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
				
				image_update.time_offset = to<RealT>(parts.at(3));
				image_update.image_buffer = frame_for_index(to<std::size_t>(parts.at(2)));

				// http://www.boinx.com/chronicles/2013/3/22/field-of-view-fov-of-cameras-in-ios-devices/
				if (parts.size() >= 5)
					image_update.field_of_view = radians(to<RealT>(parts.at(4)));
				else
					image_update.field_of_view = 56.3_deg; // iPhone 5

				_sensor_updates.push_back(new ImageUpdate(image_update));
			}
			else
			{
				//logger()->log(LOG_WARN, LogBuffer() << "Unexpected line: " << line);
			}
		}
	}

	MotionModel::MotionModel() : _gravity(0), _position(0), _bearing(-1)
	{
	}

	MotionModel::~MotionModel()
	{
	}

	void MotionModel::update(SensorUpdate * sensor_update)
	{
		sensor_update->apply(this);
	}
}
