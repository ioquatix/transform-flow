//
//  BasicSensorMotionModel.h
//  Transform Flow
//
//  Created by Samuel Williams on 20/06/13.
//  Copyright (c) 2013 Orion Transfer Ltd. All rights reserved.
//

#ifndef __Transform_Flow__BasicSensorMotionModel__
#define __Transform_Flow__BasicSensorMotionModel__

#include "MotionModel.h"

namespace TransformFlow
{
	// If blend = 0, result is a, if blend = 1, result is b.
	double interpolateAnglesRadians(double a, double b, double blend);
	double interpolateAnglesDegrees(double a, double b, double blend);

	class BasicSensorMotionModel : public MotionModel
	{
		protected:
			Vec3 _gravity, _position;

			// Measured in degrees from north, the _bearing is a fusion between the compass and the gyro, while _normalized_bearing is computed purely from the compass and represents the bearing relative to the _camera_axis.
			RealT _bearing, _normalized_bearing;

			// State relating to heading updates:
			bool _heading_primed, _bearing_primed;
			HeadingUpdate _heading_update;

			// State relating to gyro/motion updates:
			bool _motion_primed;
			MotionUpdate _motion_update;

			// The relative rotation as measured from motion update to motion update.
			Radians<> _relative_rotation;

			// Used for tracking the quality of position updates:
			RealT _best_horizontal_accuracy;

			// This function computes the normalized bearing from the current heading and motion updates:
			void normalize_bearing();

		public:
			BasicSensorMotionModel();
			virtual ~BasicSensorMotionModel();

			virtual bool localization_valid() const;

			virtual void update(const LocationUpdate & location_update);
			virtual void update(const HeadingUpdate & heading_update);
			virtual void update(const MotionUpdate & motion_update);
			virtual void update(const ImageUpdate & image_update);
			
			virtual const Vec3 & gravity() const;
			virtual const Vec3 & position() const;
			virtual Radians<> bearing() const;
	};
}

#endif /* defined(__Transform_Flow__BasicSensorMotionModel__) */
