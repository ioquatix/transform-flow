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
	class BasicSensorMotionModel : public MotionModel
	{
		protected:
			bool _heading_primed;
			HeadingUpdate _heading_update;

			bool _motion_primed;
			MotionUpdate _motion_update;

		public:
			BasicSensorMotionModel();
			virtual ~BasicSensorMotionModel();

			virtual void update(const LocationUpdate & location_update);
			virtual void update(const HeadingUpdate & heading_update);
			virtual void update(const MotionUpdate & motion_update);
			virtual void update(const ImageUpdate & image_update);
	};
}

#endif /* defined(__Transform_Flow__BasicSensorMotionModel__) */
