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
		
		if (sz <= 0.05) {
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
	
	Quat world_rotation(const Vec3 & gravity, Radians<> bearing)
	{
		Quat q = IDENTITY;
		
		// F defines the negative normal for the plain
		// x -> latitude (horizontal, red marker points east)
		// y -> longitude (vertical, green marker points north)
		// z -> altitude (altitude, blue marker points up)
		Vec3 f = gravity.normalize();
		Vec3 down(0, 0, -1);

		float sz = acos(down.dot(f));
	
		if (sz > 0.01) {
			Vec3 s = cross_product(down, f);

			q *= (Quat)rotate(radians(sz), s);
		}

		q *= (Quat)rotate<Z>(bearing);

		return q;
	}
	
}
