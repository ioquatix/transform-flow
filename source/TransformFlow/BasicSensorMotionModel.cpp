//
//  BasicSensorMotionModel.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 20/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "BasicSensorMotionModel.h"

namespace TransformFlow
{
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

	BasicSensorMotionModel::BasicSensorMotionModel() : _heading_primed(false), _motion_primed(false)
	{
	}

	BasicSensorMotionModel::~BasicSensorMotionModel()
	{
	}

	void BasicSensorMotionModel::update(const LocationUpdate & location_update)
	{
	}

	void BasicSensorMotionModel::update(const HeadingUpdate & heading_update)
	{
		if (_heading_primed == false)
		{
			_heading_primed = true;

			_bearing = heading_update.true_bearing;
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

			auto rotation_about_gravity = _gravity.dot(rotation) * R2D;

			if (_heading_primed) {
				_bearing = interpolateAnglesDegrees(_bearing + rotation_about_gravity, _heading_update.true_bearing, 0.01);
			}
		} else {
			_motion_primed = true;
		}
		
		_motion_update = motion_update;
	}

	void BasicSensorMotionModel::update(const ImageUpdate & image_update)
	{
	}
}
