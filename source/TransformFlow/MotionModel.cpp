//
//  MotionModel.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 16/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "MotionModel.h"

#include <Euclid/Geometry/Plane.h>

namespace TransformFlow
{
	using namespace Dream::Events::Logging;
	using namespace Euclid::Geometry;

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

	HeadingUpdate::HeadingUpdate()
	{
		// This seems to be the default for iPhone bearing calculations:
		device_north = Vec3(0, 1, 0).normalize();
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
	
	RealT ImageUpdate::pixels_of(Radians<> angle) const
	{
		return angle / (field_of_view / image_buffer->size()[WIDTH]);
	}

#pragma mark -

	using namespace Dream::Events::Logging;
	
	const char * GYROSCOPE = "Gyroscope";
	const char * ACCELEROMETER = "Accelerometer";
	const char * GRAVITY = "Gravity";
	const char * MOTION = "Motion";
	const char * LOCATION = "Location";
	const char * HEADING = "Heading";
	const char * FRAME = "Frame";

	SensorData::SensorData(const Path & path) : _loader(new Resources::Loader(path)) {
		_loader->add_loader(new Image::Loader);
		
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

	MotionModel::MotionModel() : _camera_axis(0, 0, -1)
	{
		
	}

	MotionModel::~MotionModel()
	{
	}

	void MotionModel::update(SensorUpdate * sensor_update)
	{
		sensor_update->apply(this);
	}

	bool MotionModel::localization_valid() const
	{
		return !gravity().equivalent({0, 0, 0});
	}

	Radians<> MotionModel::tilt() const
	{
		// This code calculates the right vector in device centric coordinates.
		// If gravity is naturally -Y, then forward is +Z, then right is +X.
		auto right = cross_product({0, 0, 1}, gravity().normalize()).normalize();

		// The image Y axis in device space points towards the right when gravity is device space -Y and forward is +Z.
		//auto angle = right.angle_between({0, 1, 0});
		Quat q = rotate(right, {0, 1, 0}, {0, 0, 1});
		
		// We want the signed rotation, e.g. from -180deg to 180deg, around {0, 0, 1}.
		return q.angle() * q.axis().dot({0, 0, 1});
	}
	
	// Return the vector component of u orthogonal to v:
	static Vec3 project(const Vec3 & u, const Vec3 & v)
	{
		return u - (v * (u.dot(v) / v.dot(v)));
	}
	
	Quat local_camera_transform(const Vec3 & gravity, const Radians<> & bearing)
	{
		float sz = acos(Vec3(0, 0, -1).dot(gravity));
		
		if (sz <= 0.01) {
			return rotate<Z>(bearing);
		} else {
			Vec3 pyz = gravity, pxy = gravity;

			pyz[X] = 0;
			pxy[Z] = 0;

			// Calculate the rotational components of the gravity vector, so we can decompose into a rotation around Z and a rotation around X. Gravity doesn't cause rotations around Y.
			Vec3 rz = pxy.normalize();
			Quat qz = rotate(rz, {0, -1, 0}, {0, 0, 1});
		
			Vec3 rx = project(qz * gravity, {1, 0, 0}).normalize();
			Quat qx = rotate(rx, {0, -1, 0}, {1, 0, 0});

			//NSLog(@"qx=%0.3f qz=%0.3f b=%0.3f", R2D * (float)qx.angle(), R2D * (float)qz.angle(), origin.rotation);

			Quat q = rotate<Y>(-bearing) << qx << qz;

			q = q.conjugate().normalize();

			return q << rotate<X>(-R90);
		}
	}
}
