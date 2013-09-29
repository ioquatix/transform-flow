//
//  BasicSensorMotionModel.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 20/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "BasicSensorMotionModel.h"

#include <Dream/Events/Logger.h>

namespace TransformFlow
{
	using namespace Dream::Events::Logging;
	
	double interpolateAnglesRadians(double a, double b, double blend)
	{
		double ix = sin(a), iy = cos(a);
		double jx = sin(b), jy = cos(b);
		
		return atan2(ix-(ix-jx)*blend, iy-(iy-jy)*blend);
	}

	double interpolateAnglesDegrees(double a, double b, double blend)
	{
		return interpolateAnglesRadians(a * D2R, b * D2R, blend) * R2D;
	}

	BasicSensorMotionModel::BasicSensorMotionModel() : _gravity(0), _position(0), _bearing(0), _heading_primed(false), _motion_primed(false), _best_horizontal_accuracy(100), _relative_rotation(0)
	{
	}

	BasicSensorMotionModel::~BasicSensorMotionModel()
	{
	}

	void BasicSensorMotionModel::update(const LocationUpdate & location_update)
	{
		// We bias this a wee bit so that updates with a similar accuracy +/- 50% will be accepted.
		if (location_update.horizontal_accuracy < _best_horizontal_accuracy * 1.5)
		{
			_position[X] = location_update.latitude;
			_position[Y] = location_update.longitude;
			_position[Z] = location_update.altitude;
			
			_best_horizontal_accuracy = std::max<RealT>(location_update.horizontal_accuracy, 20);
		}
	}

	// This function reprojects the rotation of device north onto the camera axis.
	RealT normalized_bearing(RealT bearing, const Vec3 & device_north, const Vec3 & gravity, const Vec3 & camera_axis)
	{
		return bearing;
	}

	void BasicSensorMotionModel::update(const HeadingUpdate & heading_update)
	{
		// We compute the bearing around -Z axis. The bearing is the angle between "north" and "device north". For most phones I've worked with, "device north" is <0, 1, 0>. However, this isn't usually pointing down the camera axis <0, 0, -1> which causes problems for rotations around that axis. We fix this by computing the rotation of the -Z axis.
		
		_normalized_bearing = normalized_bearing(heading_update.true_bearing, heading_update.device_north, _gravity, _camera_axis);
		
		if (_heading_primed == false)
		{
			_heading_primed = true;

			_bearing = _normalized_bearing;
		}
		
		_heading_update = heading_update;
	}

	void BasicSensorMotionModel::update(const MotionUpdate & motion_update)
	{
		_gravity = motion_update.gravity;

		if (_motion_primed) {
			RealT dt = motion_update.time_offset - _motion_update.time_offset;

			// Calculate the rotation around gravity, rotation rate is in radians/second
			auto rotation = motion_update.rotation_rate * dt;

			auto rotation_about_gravity = _gravity.dot(rotation);

			// Relative rotation by the gyroscope is saved in radians:
			_relative_rotation += radians(rotation_about_gravity);

			if (_heading_primed) {
				_bearing = interpolateAnglesDegrees(_bearing + (rotation_about_gravity * R2D), _normalized_bearing, 0.1);
			}
		} else {
			_motion_primed = true;
		}
		
		_motion_update = motion_update;
	}

	void BasicSensorMotionModel::update(const ImageUpdate & image_update)
	{
	}
	
	const Vec3 & BasicSensorMotionModel::gravity() const
	{
		return _gravity;
	}
	
	const Vec3 & BasicSensorMotionModel::position() const
	{
		return _position;
	}
	
	Radians<> BasicSensorMotionModel::bearing() const
	{
		return degrees(_bearing);
	}
}
