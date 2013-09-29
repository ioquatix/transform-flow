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

			/// Measured in degrees from north.
			RealT _bearing, _normalized_bearing;
			
			bool _heading_primed;
			HeadingUpdate _heading_update;

			bool _motion_primed;
			MotionUpdate _motion_update;

			Radians<> _relative_rotation;

			RealT _best_horizontal_accuracy;

		public:
			BasicSensorMotionModel();
			virtual ~BasicSensorMotionModel();

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
