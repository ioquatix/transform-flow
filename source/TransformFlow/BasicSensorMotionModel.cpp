//
//  BasicSensorMotionModel.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 20/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#include "BasicSensorMotionModel.h"

#include <Euclid/Geometry/Plane.h>

namespace TransformFlow
{
	using namespace Euclid::Geometry;

	static double interpolateAnglesRadians(double a, double b, double blend)
	{
		double ix = sin(a), iy = cos(a);
		double jx = sin(b), jy = cos(b);
		
		return atan2(ix-(ix-jx)*blend, iy-(iy-jy)*blend);
	}

	static double interpolateAnglesDegrees(double a, double b, double blend)
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
				_bearing = interpolateAnglesDegrees(_bearing + rotation_about_gravity, _heading_update.true_bearing, 0.001);
			}
		} else {
			_motion_primed = true;
		}
		
		_motion_update = motion_update;
	}

	void BasicSensorMotionModel::update(const ImageUpdate & image_update)
	{
	}

	Radians<> BasicSensorMotionModel::tilt() const
	{
		// This is in device-space/image-space.
		Vec3 north{0, 0, -1};

		auto heading = Quat(rotate<Z>(-bearing())) * north;

		Plane3 flat_plane(Vec3{0}, heading);
		Vec3 planar_gravity = flat_plane.closest_point(gravity());

		return planar_gravity.angle_between({1, 0, 0});
	}
}